/* -*- mode: C++; c-file-style: "gnu" -*-
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "kmfolderindex.h"
#include "kmfolder.h"
#include <config.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <kdebug.h>


#define HAVE_MMAP //need to get this into autoconf FIXME  --Sam
#include <unistd.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

// Current version of the table of contents (index) files
#define INDEX_VERSION 1506

#ifndef MAX_LINE
#define MAX_LINE 4096
#endif

#ifndef INIT_MSGS
#define INIT_MSGS 8
#endif

#include <errno.h>
#include <assert.h>
#include <utime.h>
#include <fcntl.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif
#include <kapplication.h>
#include <kcursor.h>
#include <kmessagebox.h>
#include <klocale.h>
#include "kmmsgdict.h"

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

KMFolderIndex::KMFolderIndex(KMFolder* folder, const char* name)
  : FolderStorage(folder, name), mMsgList(INIT_MSGS)
{
    mIndexStream = 0;
    mIndexStreamPtr = 0;
    mIndexStreamPtrLength = 0;
    mIndexSwapByteOrder = false;
    mIndexSizeOfLong = sizeof(long);
    mIndexId = 0;
    mHeaderOffset   = 0;
}


KMFolderIndex::~KMFolderIndex()
{
}


QString KMFolderIndex::indexLocation() const
{
  QString sLocation(folder()->path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += dotEscape(fileName());
  sLocation += ".index";

  return sLocation;
}

int KMFolderIndex::updateIndex()
{
  if (!mAutoCreateIndex)
    return 0;
  bool dirty = mDirty;
  mDirtyTimer->stop();
  for (unsigned int i=0; !dirty && i<mMsgList.high(); i++)
    if (mMsgList.at(i))
      dirty = !mMsgList.at(i)->syncIndexString();
  if (!dirty) { // Update successful
      touchMsgDict();
      return 0;
  }
  return writeIndex();
}

int KMFolderIndex::writeIndex( bool createEmptyIndex )
{
  QString tempName;
  QString indexName;
  mode_t old_umask;
  int len;
  const uchar *buffer = 0;

  indexName = indexLocation();
  tempName = indexName + ".temp";
  unlink(QFile::encodeName(tempName));

  // We touch the folder, otherwise the index is regenerated, if KMail is
  // running, while the clock switches from daylight savings time to normal time
  utime(QFile::encodeName(location()), 0);

  old_umask = umask(077);
  FILE *tmpIndexStream = fopen(QFile::encodeName(tempName), "w");
  umask(old_umask);
  if (!tmpIndexStream)
    return errno;

  fprintf(tmpIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);

  // Header
  Q_UINT32 byteOrder = 0x12345678;
  Q_UINT32 sizeOfLong = sizeof(long);

  Q_UINT32 header_length = sizeof(byteOrder)+sizeof(sizeOfLong);
  char pad_char = '\0';
  fwrite(&pad_char, sizeof(pad_char), 1, tmpIndexStream);
  fwrite(&header_length, sizeof(header_length), 1, tmpIndexStream);

  // Write header
  fwrite(&byteOrder, sizeof(byteOrder), 1, tmpIndexStream);
  fwrite(&sizeOfLong, sizeof(sizeOfLong), 1, tmpIndexStream);

  off_t nho = ftell(tmpIndexStream);

  if ( !createEmptyIndex ) {
    KMMsgBase* msgBase;
    for (unsigned int i=0; i<mMsgList.high(); i++)
    {
      if (!(msgBase = mMsgList.at(i))) continue;
      buffer = msgBase->asIndexString(len);
      fwrite(&len,sizeof(len), 1, tmpIndexStream);

      off_t tmp = ftell(tmpIndexStream);
      msgBase->setIndexOffset(tmp);
      msgBase->setIndexLength(len);
      if(fwrite(buffer, len, 1, tmpIndexStream) != 1)
	kdDebug(5006) << "Whoa! " << __FILE__ << ":" << __LINE__ << endl;
    }
  }

  int fError = ferror( tmpIndexStream );
  if( fError != 0 ) {
    fclose( tmpIndexStream );
    return fError;
  }
  if(    ( fflush( tmpIndexStream ) != 0 )
      || ( fsync( fileno( tmpIndexStream ) ) != 0 ) ) {
    int errNo = errno;
    fclose( tmpIndexStream );
    return errNo;
  }
  if( fclose( tmpIndexStream ) != 0 )
    return errno;

  ::rename(QFile::encodeName(tempName), QFile::encodeName(indexName));
  mHeaderOffset = nho;
  if (mIndexStream)
      fclose(mIndexStream);

  if ( createEmptyIndex )
    return 0;

  mIndexStream = fopen(QFile::encodeName(indexName), "r+"); // index file
  assert( mIndexStream );
  fcntl(fileno(mIndexStream), F_SETFD, FD_CLOEXEC);

  updateIndexStreamPtr();

  writeMsgDict();

  setDirty( false );
  return 0;
}


bool KMFolderIndex::readIndex()
{
  Q_INT32 len;
  KMMsgInfo* mi;

  assert(mIndexStream != 0);
  rewind(mIndexStream);

  clearIndex();
  int version;

  setDirty( false );

  if (!readIndexHeader(&version)) return false;

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
  mHeaderOffset = ftell(mIndexStream);

  clearIndex();
  while (!feof(mIndexStream))
  {
    mi = 0;
    if(version >= 1505) {
      if(!fread(&len, sizeof(len), 1, mIndexStream))
        break;

      if (mIndexSwapByteOrder)
        len = kmail_swap_32(len);

      off_t offs = ftell(mIndexStream);
      if(fseek(mIndexStream, len, SEEK_CUR))
        break;
      mi = new KMMsgInfo(folder(), offs, len);
    }
    else
    {
      QCString line(MAX_LINE);
      fgets(line.data(), MAX_LINE, mIndexStream);
      if (feof(mIndexStream)) break;
      if (*line.data() == '\0') {
	  fclose(mIndexStream);
	  mIndexStream = 0;
	  clearIndex();
	  return false;
      }
      mi = new KMMsgInfo(folder());
      mi->compat_fromOldIndexString(line, mConvertToUtf8);
    }
    if(!mi)
      break;

    if (mi->isDeleted())
    {
      delete mi;  // skip messages that are marked as deleted
      setDirty( true );
      needsCompact = true;  //We have deleted messages - needs to be compacted
      continue;
    }
#ifdef OBSOLETE
    else if (mi->isNew())
    {
      mi->setStatus(KMMsgStatusUnread);
      mi->setDirty(FALSE);
    }
#endif
    if ((mi->isNew()) || (mi->isUnread()) ||
        (folder() == kmkernel->outboxFolder()))
    {
      ++mUnreadMsgs;
      if (mUnreadMsgs == 0) ++mUnreadMsgs;
    }
    mMsgList.append(mi, false);
  }
  if( version < 1505)
  {
    mConvertToUtf8 = FALSE;
    setDirty( true );
    writeIndex();
  }
  mTotalMsgs = mMsgList.count();
  return true;
}


int KMFolderIndex::count(bool cache) const
{
  int res = FolderStorage::count(cache);
  if (res == -1)
    res = mMsgList.count();
  return res;
}


bool KMFolderIndex::readIndexHeader(int *gv)
{
  int indexVersion;
  assert(mIndexStream != 0);
  mIndexSwapByteOrder = false;
  mIndexSizeOfLong = sizeof(long);

  int ret = fscanf(mIndexStream, "# KMail-Index V%d\n", &indexVersion);
  if ( ret == EOF || ret == 0 )
      return false; // index file has invalid header
  if(gv)
      *gv = indexVersion;
  if (indexVersion < 1505 ) {
      if(indexVersion == 1503) {
	  kdDebug(5006) << "Converting old index file " << indexLocation() << " to utf-8" << endl;
	  mConvertToUtf8 = TRUE;
      }
      return TRUE;
  } else if (indexVersion == 1505) {
  } else if (indexVersion < INDEX_VERSION) {
      kdDebug(5006) << "Index file " << indexLocation() << " is out of date. Re-creating it." << endl;
      createIndexFromContents();
      return FALSE;
  } else if(indexVersion > INDEX_VERSION) {
      kapp->setOverrideCursor(KCursor::arrowCursor());
      int r = KMessageBox::questionYesNo(0,
					 i18n(
					    "The mail index for '%1' is from an unknown version of KMail (%2).\n"
					    "This index can be regenerated from your mail folder, but some "
					    "information, including status flags, may be lost. Do you wish "
					    "to downgrade your index file?") .arg(name()) .arg(indexVersion) );
      kapp->restoreOverrideCursor();
      if (r == KMessageBox::Yes)
	  createIndexFromContents();
      return FALSE;
  }
  else {
      // Header
      Q_UINT32 byteOrder = 0;
      Q_UINT32 sizeOfLong = sizeof(long); // default

      Q_UINT32 header_length = 0;
      fseek(mIndexStream, sizeof(char), SEEK_CUR );
      fread(&header_length, sizeof(header_length), 1, mIndexStream);
      if (header_length > 0xFFFF)
         header_length = kmail_swap_32(header_length);

      off_t endOfHeader = ftell(mIndexStream) + header_length;

      bool needs_update = true;
      // Process available header parts
      if (header_length >= sizeof(byteOrder))
      {
         fread(&byteOrder, sizeof(byteOrder), 1, mIndexStream);
         mIndexSwapByteOrder = (byteOrder == 0x78563412);
         header_length -= sizeof(byteOrder);

         if (header_length >= sizeof(sizeOfLong))
         {
            fread(&sizeOfLong, sizeof(sizeOfLong), 1, mIndexStream);
            if (mIndexSwapByteOrder)
               sizeOfLong = kmail_swap_32(sizeOfLong);
            mIndexSizeOfLong = sizeOfLong;
            header_length -= sizeof(sizeOfLong);
            needs_update = false;
         }
      }
      if (needs_update || mIndexSwapByteOrder || (mIndexSizeOfLong != sizeof(long)))
	setDirty( true );
      // Seek to end of header
      fseek(mIndexStream, endOfHeader, SEEK_SET );

      if (mIndexSwapByteOrder)
         kdDebug(5006) << "Index File has byte order swapped!" << endl;
      if (mIndexSizeOfLong != sizeof(long))
         kdDebug(5006) << "Index File sizeOfLong is " << mIndexSizeOfLong << " while sizeof(long) is " << sizeof(long) << " !" << endl;

  }
  return TRUE;
}


#ifdef HAVE_MMAP
bool KMFolderIndex::updateIndexStreamPtr(bool just_close)
#else
bool KMFolderIndex::updateIndexStreamPtr(bool)
#endif
{
    // We touch the folder, otherwise the index is regenerated, if KMail is
    // running, while the clock switches from daylight savings time to normal time
    utime(QFile::encodeName(location()), 0);
    utime(QFile::encodeName(indexLocation()), 0);
    utime(QFile::encodeName(KMMsgDict::getFolderIdsLocation( folder() )), 0);

  mIndexSwapByteOrder = false;
#ifdef HAVE_MMAP
    if(just_close) {
	if(mIndexStreamPtr)
	    munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
	mIndexStreamPtr = 0;
	mIndexStreamPtrLength = 0;
	return TRUE;
    }

    assert(mIndexStream);
    struct stat stat_buf;
    if(fstat(fileno(mIndexStream), &stat_buf) == -1) {
	if(mIndexStreamPtr)
	    munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
	mIndexStreamPtr = 0;
	mIndexStreamPtrLength = 0;
	return FALSE;
    }
    if(mIndexStreamPtr)
	munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
    mIndexStreamPtrLength = stat_buf.st_size;
    mIndexStreamPtr = (uchar *)mmap(0, mIndexStreamPtrLength, PROT_READ, MAP_SHARED,
				    fileno(mIndexStream), 0);
    if(mIndexStreamPtr == MAP_FAILED) {
	mIndexStreamPtr = 0;
	mIndexStreamPtrLength = 0;
	return FALSE;
    }
#endif
    return TRUE;
}


KMFolderIndex::IndexStatus KMFolderIndex::indexStatus()
{
    QFileInfo contInfo(location());
    QFileInfo indInfo(indexLocation());

    if (!contInfo.exists()) return KMFolderIndex::IndexOk;
    if (!indInfo.exists()) return KMFolderIndex::IndexMissing;

    return ( contInfo.lastModified() > indInfo.lastModified() )
        ? KMFolderIndex::IndexTooOld
        : KMFolderIndex::IndexOk;
}

void KMFolderIndex::clearIndex(bool autoDelete, bool syncDict)
{
    mMsgList.clear(autoDelete, syncDict);
}


void KMFolderIndex::truncateIndex()
{
  if ( mHeaderOffset )
    truncate(QFile::encodeName(indexLocation()), mHeaderOffset);
  else
    // The index file wasn't opened, so we don't know the header offset.
    // So let's just create a new empty index.
    writeIndex( true );
}


void KMFolderIndex::fillDictFromIndex(KMMsgDict *dict)
{
  open();
  mMsgList.fillMsgDict(dict);
  close();
}


KMMsgInfo* KMFolderIndex::setIndexEntry( int idx, KMMessage *msg )
{
  KMMsgInfo *msgInfo = new KMMsgInfo( folder() );
  *msgInfo = *msg;
  mMsgList.set( idx, msgInfo );
  return msgInfo;
}
#include "kmfolderindex.moc"
