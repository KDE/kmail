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

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmfolderindex_common.cpp"

//---------

#ifdef KMAIL_SQLITE_INDEX
# include "kmfolderindex_sqlite.cpp"
#else

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

int KMFolderIndex::updateIndex()
{
  if (!mAutoCreateIndex)
    return 0;
  bool dirty = mDirty;
  mDirtyTimer->stop();
  const uint high = mMsgList.high();
  for (uint i=0; !dirty && i<high; i++) {
    KMMsgBase *msg = mMsgList.at(i);
    if (msg)
      dirty = !msg->syncIndexString();
  }
  if (!dirty) { // Update successful
      touchFolderIdsFile();
      return 0;
  }
  return writeIndex();
}

int KMFolderIndex::writeIndex( bool createEmptyIndex )
{
  QString tempName;
  QString indexName;
  mode_t old_umask;

  indexName = indexLocation();
  tempName = indexName + ".temp";
  {
    int result = unlink( QFile::encodeName( tempName ) );
    if ( ! ( result == 0 || (result == -1 && errno == ENOENT ) ) )
      return errno;
  }

  // We touch the folder, otherwise the index is regenerated, if KMail is
  // running, while the clock switches from daylight savings time to normal time
  utime( QFile::encodeName( QDir::toNativeSeparators(location()) ), 0 );

  old_umask = umask( 077 );
  FILE *tmpIndexStream = KDE_fopen( QFile::encodeName( tempName ), "w" );
  kDebug( StorageDebug ) << "KDE_fopen(tempName=" << tempName << ", \"w\") == tmpIndexStream == " << tmpIndexStream;
  umask( old_umask );
  if ( !tmpIndexStream ) {
    return errno;
  }

  fprintf(tmpIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);

  // Header
  quint32 byteOrder = 0x12345678;
  quint32 sizeOfLong = sizeof(long);

  quint32 header_length = sizeof(byteOrder)+sizeof(sizeOfLong);
  char pad_char = '\0';
  fwrite(&pad_char, sizeof(pad_char), 1, tmpIndexStream);
  fwrite(&header_length, sizeof(header_length), 1, tmpIndexStream);

  // Write header
  fwrite(&byteOrder, sizeof(byteOrder), 1, tmpIndexStream);
  fwrite(&sizeOfLong, sizeof(sizeOfLong), 1, tmpIndexStream);

  off_t nho = KDE_ftell(tmpIndexStream);

  int fError = 0;
  if ( !createEmptyIndex ) {
    FILE *oldIndexStream = mIndexStream; //save
    mIndexStream = tmpIndexStream; // needed because writeMessages() operates on mIndexStream
    fError = writeMessages( 0/*all*/, false /* !flush */ );
    mIndexStream = oldIndexStream; //restore
/* moved to writeMessages()
    KMMsgBase* msgBase;
    int len;
    const uchar *buffer = 0;
    for (unsigned int i=0; i<mMsgList.high(); i++)
    {
      if (!(msgBase = mMsgList.at(i))) continue;
      buffer = msgBase->asIndexString(len);
      fwrite(&len,sizeof(len), 1, tmpIndexStream);

      off_t tmp = KDE_ftell(tmpIndexStream);
      msgBase->setIndexOffset(tmp);
      msgBase->setIndexLength(len);
      if(fwrite(buffer, len, 1, tmpIndexStream) != 1)
        kDebug(5006) <<"Whoa!";
    }*/
  }

  fError |= ferror( tmpIndexStream );
  if( fError != 0 ) {
    fclose( tmpIndexStream );
    kDebug( StorageDebug ) << "fclose(tmpIndexStream = " << tmpIndexStream << ")";
    return fError;
  }
  if(    ( fflush( tmpIndexStream ) != 0 )
      || ( fsync( fileno( tmpIndexStream ) ) != 0 ) ) {
    int errNo = errno;
    fclose( tmpIndexStream );
    kWarning() << "fflush() or fsync() failed; fclose(tmpIndexStream = " << tmpIndexStream << ")";
    return errNo;
  }
  kDebug( StorageDebug ) << "fclose(tmpIndexStream = " << tmpIndexStream << ")";
  if( fclose( tmpIndexStream ) != 0 ) {
    kWarning() << "fclose() failed";
    return errno;
  }

#ifdef Q_WS_WIN
  if (mIndexStream) { // close before renaming
    // neither this fixes windows port: 
    // if ( !updateIndexStreamPtr() )
    //  return 1;
    bool ok = fclose( mIndexStream ) == 0;
    kDebug( StorageDebug ) << "fclose(mIndexStream = " << mIndexStream << ")";
    mIndexStream = 0;
    if ( !ok ) {
      kWarning() << "fclose() failed";
      return errno;
    }
  }
#endif

  if ( KDE_rename(QFile::encodeName(tempName), QFile::encodeName(indexName)) != 0 )
    return errno;
  mHeaderOffset = nho;

#ifndef Q_WS_WIN
  if (mIndexStream) {
      fclose(mIndexStream);
      kDebug( StorageDebug ) << "fclose(mIndexStream = " << mIndexStream << ")";
  }
#endif

  if ( createEmptyIndex )
    return 0;

  mIndexStream = KDE_fopen(QFile::encodeName(indexName), "r+"); // index file
  kDebug( StorageDebug ) << "KDE_fopen(indexName=" << indexName << ", \"r+\") == mIndexStream == " << mIndexStream;
  assert( mIndexStream );
#ifndef Q_WS_WIN
  fcntl(fileno(mIndexStream), F_SETFD, FD_CLOEXEC);
#endif

  if ( !updateIndexStreamPtr() )
    return 1;
  if ( 0 != writeFolderIdsFile() )
    return 1;

  setDirty( false );
  return 0;
}


bool KMFolderIndex::readIndex()
{
  qint32 len;
  KMMsgInfo* mi;

  assert(mIndexStream != 0);
  rewind(mIndexStream);

  clearIndex();
  int version;

  setDirty( false );

  if (!readIndexHeader(&version)) return false;

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
  mHeaderOffset = KDE_ftell(mIndexStream);

  clearIndex();
  while (!feof(mIndexStream))
  {
    mi = 0;
    if(version >= 1505) {
      if(!fread(&len, sizeof(len), 1, mIndexStream))
        break;

      if (mIndexSwapByteOrder)
        len = kmail_swap_32(len);

      off_t offs = KDE_ftell(mIndexStream);
      if(KDE_fseek(mIndexStream, len, SEEK_CUR))
        break;
      mi = new KMMsgInfo(folder(), offs, len);
    }
    else
    {
      QByteArray line( MAX_LINE, '\0' );
      fgets(line.data(), MAX_LINE, mIndexStream);
      if (feof(mIndexStream)) break;
      if (*line.data() == '\0') {
        fclose(mIndexStream);
        kDebug( StorageDebug ) << "fclose(mIndexStream = " << mIndexStream << ")";
        mIndexStream = 0;
        clearIndex();
        return false;
      }
      mi = new KMMsgInfo(folder());
      mi->compat_fromOldIndexString(line, mConvertToUtf8);
    }
    if(!mi)
      break;

    if (mi->status().isDeleted())
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
      mi->setDirty(false);
    }
#endif
    if ((mi->status().isNew()) || (mi->status().isUnread()) ||
        (folder() == kmkernel->outboxFolder()))
    {
      ++mUnreadMsgs;
      if (mUnreadMsgs == 0) ++mUnreadMsgs;
    }
    mMsgList.append(mi, false);
  }
  if( version < 1505)
  {
    mConvertToUtf8 = false;
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
        kDebug(5006) <<"Converting old index file" << indexLocation() <<" to utf-8";
        mConvertToUtf8 = true;
      }
      return true;
  } else if (indexVersion == 1505) {
  } else if (indexVersion < INDEX_VERSION) {
      kDebug(5006) <<"Index file" << indexLocation() <<" is out of date. Re-creating it.";
      createIndexFromContents();
      return false;
  } else if(indexVersion > INDEX_VERSION) {
      QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
      int r = KMessageBox::questionYesNo(0,
        i18n(
          "The mail index for '%1' is from an unknown version of KMail (%2).\n"
          "This index can be regenerated from your mail folder, but some "
          "information, including status flags, may be lost. Do you wish "
          "to downgrade your index file?" , objectName() , indexVersion), QString(), KGuiItem(i18n("Downgrade")), KGuiItem(i18n("Do Not Downgrade")) );
      QApplication::restoreOverrideCursor();
      if (r == KMessageBox::Yes)
        createIndexFromContents();
      return false;
  }
  else {
      // Header
      quint32 byteOrder = 0;
      quint32 sizeOfLong = sizeof(long); // default

      quint32 header_length = 0;
      KDE_fseek(mIndexStream, sizeof(char), SEEK_CUR );
      fread(&header_length, sizeof(header_length), 1, mIndexStream);
      if (header_length > 0xFFFF)
         header_length = kmail_swap_32(header_length);

      off_t endOfHeader = KDE_ftell(mIndexStream) + header_length;

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
      KDE_fseek(mIndexStream, endOfHeader, SEEK_SET );

      if (mIndexSwapByteOrder)
         kDebug(5006) <<"Index File has byte order swapped!";
      if (mIndexSizeOfLong != sizeof(long))
         kDebug(5006) <<"Index File sizeOfLong is" << mIndexSizeOfLong <<" while sizeof(long) is" << sizeof(long) <<" !";

  }
  return true;
}


#ifdef HAVE_MMAP
bool KMFolderIndex::updateIndexStreamPtr(bool just_close)
#else
bool KMFolderIndex::updateIndexStreamPtr(bool)
#endif
{
    // We touch the folder, otherwise the index is regenerated, if KMail is
    // running, while the clock switches from daylight savings time to normal time
    if (0 != utime(QFile::encodeName( QDir::toNativeSeparators(location()) ), 0))
      kWarning() << "utime(" << QDir::toNativeSeparators(location()) << ", 0) failed (location())";
    if (0 != utime(QFile::encodeName( QDir::toNativeSeparators(indexLocation()) ), 0))
      kWarning() << "utime(" << QDir::toNativeSeparators(indexLocation()) << ", 0) failed (indexLocation())";
    if (0 != utime(QFile::encodeName( QDir::toNativeSeparators(KMMsgDict::getFolderIdsLocation( *this )) ), 0))
      kWarning() << "utime(" << QDir::toNativeSeparators(KMMsgDict::getFolderIdsLocation( *this )) << ", 0) failed (KMMsgDict::getFolderIdsLocation( *this ))";

    mIndexSwapByteOrder = false;
#ifdef HAVE_MMAP
    if(just_close) {
      bool munmapResult = true;
      if(mIndexStreamPtr)
        munmapResult = 0 == munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
      mIndexStreamPtr = 0;
      mIndexStreamPtrLength = 0;
      return munmapResult;
    }

    assert(mIndexStream);
    KDE_struct_stat stat_buf;
    if(KDE_fstat(fileno(mIndexStream), &stat_buf) == -1) {
      if(mIndexStreamPtr)
        munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
      mIndexStreamPtr = 0;
      mIndexStreamPtrLength = 0;
      return false;
    }
    if(mIndexStreamPtr)
      if(0 != munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength))
        return false;
    mIndexStreamPtrLength = stat_buf.st_size;
    mIndexStreamPtr = (uchar *)mmap(0, mIndexStreamPtrLength, PROT_READ, MAP_SHARED,
    fileno(mIndexStream), 0);
    if(mIndexStreamPtr == MAP_FAILED) {
      mIndexStreamPtr = 0;
      mIndexStreamPtrLength = 0;
      return false;
    }
#endif
    return true;
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

void KMFolderIndex::fillMessageDict()
{
  open( "fillDict" );
  for ( unsigned int idx = 0; idx < mMsgList.high(); idx++ ) {
    KMMsgBase* msg = mMsgList.at( idx );
    if ( msg ) {
      KMMsgDict::mutableInstance()->insert( 0, msg, idx );
    }
  }
  close( "fillDict" );
}


KMMsgInfo* KMFolderIndex::setIndexEntry( int idx, KMMessage *msg )
{
  KMMsgInfo *msgInfo = new KMMsgInfo( folder() );
  *msgInfo = *msg;
  mMsgList.set( idx, msgInfo );
  return msgInfo;
}

bool KMFolderIndex::recreateIndex()
{
  QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
  KMessageBox::error(0,
       i18n("The mail index for '%1' is corrupted and will be regenerated now, "
            "but some information, including status flags, will be lost.", label()));
  QApplication::restoreOverrideCursor();
  if ( createIndexFromContents() != 0 )
    return false;
  return readIndex();
}

int KMFolderIndex::writeMessages( KMMsgBase* msg, bool flush )
{
  // ### This only works if the size of mMsgList is > 0, otherwise msg will
  //     not be written at all, because the loop is not executed.
  Q_ASSERT( msg == 0 || mMsgList.high() > 0 );

  const uint high = mMsgList.high();
  for ( uint i = 0; i < high; i++ )
  {
    KMMsgBase* msgBase = msg ? msg : mMsgList.at(i);
    if ( !msgBase )
      continue;
    int len;
    const uchar *buffer = msgBase->asIndexString( len );
    if ( fwrite( &len, sizeof( len ), 1, mIndexStream ) != 1 )
      return 1;
    off_t offset = KDE_ftell( mIndexStream );
    if ( fwrite( buffer, len, 1, mIndexStream ) != 1 ) {
      kDebug() << "Whoa!";
      return 1;
    }
    msgBase->setIndexOffset( offset );
    msgBase->setIndexLength( len );
    if ( msg )
      break; // only one
  }
  if ( flush )
    fflush( mIndexStream );
  int error = ferror( mIndexStream );
  if ( error != 0 )
    return error;
  return 0;
}

#endif // !KMAIL_SQLITE_INDEX

#include "kmfolderindex.moc"
