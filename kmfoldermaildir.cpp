// kmfoldermaildir.cpp
// Author: Kurt Granroth <granroth@kde.org>

#include <qdir.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qtextstream.h>

#include "kmglobal.h"
#include "kfileio.h"
#include "kmfoldermaildir.h"
#include "kmmessage.h"
#include "kmundostack.h"

#include <kapp.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#ifndef isblank
#  define isblank(x) ((x)==' '||(x)=='\t')
#endif

#ifndef MAX_LINE
#define MAX_LINE 4096
#endif
#ifndef INIT_MSGS
#define INIT_MSGS 8
#endif

static QRegExp SUFFIX_REGEX(":2,?R?S?$");

//-----------------------------------------------------------------------------
KMFolderMaildir::KMFolderMaildir(KMFolderDir* aParent, const QString& aName)
  : KMFolderMaildirInherited(aParent, aName)
{
}


//-----------------------------------------------------------------------------
KMFolderMaildir::~KMFolderMaildir()
{
  if (mOpenCount>0) close(TRUE);
  if (kernel->undoStack()) kernel->undoStack()->folderDestroyed(this);
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::open()
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(name() != "");

  if (!path().isEmpty())
  {
    if (isIndexOutdated()) // test if contents file has changed
    {
      QString str;
      mIndexStream = NULL;
      str = i18n("Folder `%1' changed. Recreating index.")
		  .arg(name());
      emit statusMsg(str);
    } else {
      mIndexStream = fopen(indexLocation().local8Bit(), "r+"); // index file
      updateIndexStreamPtr();
    }	

    if (!mIndexStream)
      rc = createIndexFromContents();
    else
      readIndex();
  }
  else
  {
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

  mQuiet = 0;
  mChanged = FALSE;

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolderMaildir::create(bool imap)
{
  int rc;
  int old_umask;

  assert(name() != "");
  assert(mOpenCount == 0);

  // create the maildir directory structure
  if (::mkdir(QFile::encodeName(location()), S_IRWXU) > 0)
  {
    kdDebug(5006) << "Could not create " << location() << " maildir" << endl;
    return errno;
  }
  if (::mkdir(QFile::encodeName(location() + "/new"), S_IRWXU) > 0)
  {
    kdDebug(5006) << "Could not create " << location() << "/new" << endl;
    return errno;
  }
  if (::mkdir(QFile::encodeName(location() + "/cur"), S_IRWXU) > 0)
  {
    kdDebug(5006) << "Could not create " << location() << "/cur" << endl;
    return errno;
  }
  if (::mkdir(QFile::encodeName(location() + "/tmp"), S_IRWXU) > 0)
  {
    kdDebug(5006) << "Could not create " << location() << "/new" << endl;
    return errno;
  }

  if (!path().isEmpty())
  {
    old_umask = umask(077);
    mIndexStream = fopen(QFile::encodeName(indexLocation()), "w+"); //sven; open RW
	updateIndexStreamPtr(TRUE);
    umask(old_umask);

    if (!mIndexStream) return errno;
  }
  else
  {
    mAutoCreateIndex = FALSE;
  }

  mOpenCount++;
  mQuiet = 0;
  mChanged = FALSE;
  if (imap) {
    readConfig();
    mUnreadMsgs = -1;
  }

  rc = writeIndex();
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolderMaildir::close(bool aForced)
{
  if (mOpenCount <= 0) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  if (mAutoCreateIndex)
  {
      bool dirty = mDirty;
      for (int i=0; !dirty && i<mMsgList.high(); i++)
	  if (mMsgList[i])
	      dirty = !mMsgList[i]->syncIndexString();
      if(dirty)
	  writeIndex();
      writeConfig();
  }

  mMsgList.clear(TRUE);

    if (mIndexStream) {
	fclose(mIndexStream);
	updateIndexStreamPtr(TRUE);
    }

  mOpenCount   = 0;
  mIndexStream = NULL;
  mUnreadMsgs  = -1;

  mMsgList.reset(INIT_MSGS);
}

//-----------------------------------------------------------------------------
void KMFolderMaildir::sync()
{
  if (mOpenCount > 0)
    if (!mIndexStream || fsync(fileno(mIndexStream))) {
	kdDebug(5006) << "Error: Could not sync folder" << endl;
	kdDebug(5006) << "Abnormally terminating to prevent data loss, now." << endl;
	exit(1);
    }
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::expunge()
{
  int openCount = mOpenCount;

  assert(name() != "");

  close(TRUE);

  if (mAutoCreateIndex) truncate(QFile::encodeName(indexLocation()), mHeaderOffset);
  else unlink(QFile::encodeName(indexLocation()));

  // nuke all messages in this folder now
  QDir d(location() + "/new");
  d.setFilter(QDir::Files);
  QStringList files(d.entryList());
  QStringList::ConstIterator it(files.begin());
  for ( ; it != files.end(); ++it)
    QFile::remove(d.filePath(*it));

  d.setPath(location() + "/cur");
  files = d.entryList();
  for (it = files.begin(); it != files.end(); ++it)
    QFile::remove(d.filePath(*it));

  mDirty = FALSE;

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we're cleared and truncated no need to compact

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  mUnreadMsgs = 0;
  emit numUnreadMsgsChanged( this );
  if (mAutoCreateIndex)
    writeConfig();
  if (!mQuiet)
    emit changed();
  else
    mChanged = TRUE;
  return 0;
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::compact()
{
  if (needsCompact == false)
    return 0;

  open();

  QString subdirNew(location() + "/new/");
  QString subdirCur(location() + "/cur/");

  for (int i = 0; i < count(); i++)
  {
    KMMsgInfo *mi = (KMMsgInfo*)mMsgList[i];
    if (!mi)
      continue;

    QString filename(mi->fileName());
    if (filename.isNull() || filename.isEmpty())
      continue;

    // first, make sure this isn't in the 'new' subdir
    QString newFile(subdirNew + filename);
    if (QFile::exists(newFile))
      moveInternal(subdirNew + filename, subdirCur + filename, mi);

    // construct a valid filename.  if it's already valid, then
    // nothing happens
    constructValidFileName(filename, mi->status());

    // if the name changed, then we need to update the actual filename
    if (filename != mi->fileName())
    {
      moveInternal(subdirCur + mi->fileName(), subdirCur + filename, mi);
      mi->setFileName(filename);
      mDirty = TRUE;
    }

    // we can't have any New messages at this point
    if (mi->status() == KMMsgStatusNew)
    {
      mi->setStatus(KMMsgStatusUnread);
      mDirty = TRUE;
    }
  }
  close(true);

  needsCompact = false;

  return 0;
}

int KMFolderMaildir::addMsg(KMMessage* aMsg, int* index_return, bool)
{
  long len;
  unsigned long size;
  KMFolder* msgParent;
  QCString msgText;
  int idx(-1);

  // take message out of the folder it is currently in, if any
  msgParent = aMsg->parent();
  if (msgParent)
  {
    if (msgParent==this && name() != "outbox" && name() != "drafts")
        return 0;

    idx = msgParent->find(aMsg);
    msgParent->getMsg( idx );
  }

  aMsg->setStatusFields();
  if (aMsg->headerField("Content-Type").isEmpty())  // This might be added by
    aMsg->removeHeaderField("Content-Type");        // the line above
  msgText = aMsg->asString();
  len = msgText.length();

  if (len <= 0)
  {
    kdDebug(5006) << "Message added to folder `" << name() << "' contains no data. Ignoring it." << endl;
    return 0;
  }

  // "trash" is a special folder
  if (name() == "trash")
  {
    if ((aMsg->status() == KMMsgStatusNew) ||
        (aMsg->status() == KMMsgStatusUnread))
    {
      aMsg->setStatus(KMMsgStatusRead);
    }
  }

  // make sure the filename has the correct extension
  QString filename(aMsg->fileName());
  constructValidFileName(filename, aMsg->status());

  QString tmp_file(location() + "/tmp/");
  tmp_file += filename;

#if 0
  if (name() == "trash")
  {
    filename += ".lzo";
    tmp_file += filename;
kdDebug(0) << "Writing to " << tmp_file << endl;
    if (!kCStringToFile(msgText, tmp_file, false, false, true, true))
     return 0;
    else
kdDebug(0) << "Write succeeded" << endl;
  }
  else
  {
  tmp_file += filename;
#endif
  if (!kCStringToFile(msgText, tmp_file, false, false, false))
    return 0;
#if 0
  }
#endif

  QFile file(tmp_file);
  size = msgText.length();

  // now move the file to the correct location
  QString new_loc(location() + "/cur/");
  new_loc += filename;
  if (moveInternal(tmp_file, new_loc, filename, aMsg->status()) == QString::null)
  {
    file.remove();
    return 0;
  }

  if (aMsg->status() == KMMsgStatusNew)
    aMsg->setStatus(KMMsgStatusUnread);

  if (msgParent)
    if (idx >= 0) msgParent->take(idx);

  if (filename != aMsg->fileName())
    aMsg->setFileName(filename);

  if (aMsg->status()==KMMsgStatusUnread)
  {
    if (mUnreadMsgs == -1)
      mUnreadMsgs = 1;
    else
      ++mUnreadMsgs;
    emit numUnreadMsgsChanged( this );
  }

  // store information about the position in the folder file in the message
  aMsg->setParent(this);
  aMsg->setMsgSize(size);
  idx = mMsgList.append(aMsg);

  // write index entry if desired
  if (mAutoCreateIndex)
  {
    assert(mIndexStream != NULL);
    clearerr(mIndexStream);
    fseek(mIndexStream, 0, SEEK_END);
    long revert = ftell(mIndexStream);
	
	int len;
	const uchar *buffer = aMsg->asIndexString(len);
	fwrite(&len,sizeof(len), 1, mIndexStream);
	aMsg->setIndexOffset( ftell(mIndexStream) );
	aMsg->setIndexLength( len );
	if(fwrite(buffer, len, 1, mIndexStream) != 1)
	    kdDebug(5006) << "Whoa! " << __FILE__ << ":" << __LINE__ << endl;

    fflush(mIndexStream);
    int error = ferror(mIndexStream);
    if (error) {
      kdDebug(5006) << "Error: Could not add message to folder (No space left on device?)" << endl;
      if (ftell(mIndexStream) > revert) {
	kdDebug(5006) << "Undoing changes" << endl;
	truncate( QFile::encodeName(indexLocation()), revert );
      }
      kdDebug(5006) << "Abnormally terminating to prevent data loss, now." << endl;
      exit(1);
      /* This code may not be 100% reliable
      bool busy = kernel->kbp()->isBusy();
      if (busy) kernel->kbp()->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
	     "(No space left on device or insufficient quota?)\n\n"
	     "Free space and sufficient quota are required to continue safely."));
      if (busy) kernel->kbp()->busy();
      if (opened) close();
      */
      return error;
    }
  }

  // some "paper work"
  if (index_return)
    *index_return = idx;

  if (!mQuiet)
    emit msgAdded(idx);
  else
    mChanged = TRUE;

  needsCompact = true;

  return 0;
}

KMMessage* KMFolderMaildir::readMsg(int idx)
{
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  QCString msgText;
  getMsgString(idx, msgText);

  KMMessage *msg = new KMMessage(*mi);
  msg->fromString(msgText);

  mMsgList.set(idx,msg);

  return msg;
}

QCString& KMFolderMaildir::getMsgString(int idx, QCString& mDest)
{
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=NULL && !mi->isMessage());

  QString abs_file(location() + "/cur/");
  abs_file += mi->fileName();

  if (QFile::exists(abs_file) == false)
  {
    kdDebug(5006) << "The " << abs_file << " file doesn't exist!" << endl;
    return mDest;
  }

  mDest.resize(mi->msgSize()+2);
  mDest = kFileToString(abs_file, false, false);
  return mDest;
}

void KMFolderMaildir::readFileHeaderIntern(const QString& dir, const QString& file, KMMsgStatus status)
{
  // we keep our current directory to restore it later
  char path_buffer[PATH_MAX];
  ::getcwd(path_buffer, PATH_MAX - 1);
  ::chdir(dir.local8Bit());

  // messages in the 'cur' directory are Read by default.. but may
  // actually be some other state (but not New)
  if (status == KMMsgStatusRead)
  {
    if (file.find(":2,") == -1)
      status = KMMsgStatusUnread;
    else if (file.right(5) == ":2,RS")
      status = KMMsgStatusReplied;
  }

  // open the file and get a pointer to it
  QFile f(file);
  if (f.open(IO_ReadOnly) == false) return;

  char line[MAX_LINE];
  bool atEof    = false;
  bool inHeader = true;
  QCString *lastStr = NULL;

  QCString dateStr, fromStr, toStr, subjStr;
  QCString xmarkStr, replyToIdStr, msgIdStr, referencesStr;
  QCString statusStr;

  // iterate through this file until done
  while (!atEof)
  {
    if (!f.readLine(line, MAX_LINE))
      atEof = true;
  
    // are we done with this file?  if so, compile our info and store
    // it in a KMMsgInfo object
    if (atEof || !inHeader)
    {
      if ((replyToIdStr.isEmpty() || (replyToIdStr[0] != '<'))  &&
          !referencesStr.isEmpty() && referencesStr[0] == '<')
      {
        replyToIdStr = referencesStr;
      }

      if (!statusStr.isEmpty())
      {
        // only handle those states not determined by the file suffix
        if (statusStr[0] == 'S')
          status = KMMsgStatusSent;
        else if (statusStr[0] == 'F')
          status = KMMsgStatusForwarded;
        else if (statusStr[0] == 'D')
          status = KMMsgStatusDeleted;
        else if (statusStr[0] == 'Q')
          status = KMMsgStatusQueued;
        else if (statusStr[0] == 'G')
          status = KMMsgStatusFlag;
      }

      KMMsgInfo *mi = new KMMsgInfo(this);
      mi->init(subjStr, fromStr, toStr, 0, status, xmarkStr, replyToIdStr, msgIdStr, file.local8Bit(), f.size());
      if (!dateStr.isEmpty())
        mi->setDate(dateStr);
      mi->setDirty(false);
      mMsgList.append(mi);

      // if this is a New file and is in 'new', we move it to 'cur'
      if (status == KMMsgStatusNew)
      {
        QString newDir(location() + "/new/");
        QString curDir(location() + "/cur/");
        moveInternal(newDir + file, curDir + file, mi);
      }

      break;
    }

    // Is this a long header line?
    if (inHeader && line[0] == '\t' || line[0] == ' ')
    {
      int i = 0;
      while (line[i] == '\t' || line[i] == ' ')
        i++;
      if (line[i] < ' ' && line[i] > 0)
        inHeader = false;
      else
        if (lastStr)
          *lastStr += line + i;
    }
    else
      lastStr = NULL;

    if (inHeader && (line[0] == '\n' || line[0] == '\r'))
      inHeader = false;
    if (!inHeader)
      continue;

    if (strncasecmp(line, "Date:", 5) == 0 && isblank(line[5]))
    {
      dateStr = QCString(line+6);
      lastStr = &dateStr;
    }
    else if (strncasecmp(line, "From:", 5) == 0 && isblank(line[5]))
    {
      fromStr = QCString(line+6);
      lastStr = &fromStr;
    }
    else if (strncasecmp(line, "To:", 3) == 0 && isblank(line[3]))
    {
      toStr = QCString(line+4);
      lastStr = &toStr;
    }
    else if (strncasecmp(line, "Subject:", 8) == 0 && isblank(line[8]))
    {
      subjStr = QCString(line+9);
      lastStr = &subjStr;
    }
    else if (strncasecmp(line, "References:", 11) == 0 && isblank(line[11]))
    {
      int leftAngle, rightAngle;
      referencesStr = QCString(line+12);

      leftAngle = referencesStr.findRev('<');
      if (leftAngle != -1)
        referencesStr = referencesStr.mid(leftAngle);

      rightAngle = referencesStr.find('>');
      if (rightAngle != -1)
        referencesStr.truncate(rightAngle + 1);
    }
    else if (strncasecmp(line, "Message-Id:", 11) == 0 && isblank(line[11]))
    {
      int rightAngle;
      msgIdStr = QCString(line+12);

      rightAngle = msgIdStr.find( '>' );
      if (rightAngle != -1)
        msgIdStr.truncate(rightAngle + 1);
    }
    else if (strncasecmp(line, "X-KMail-Mark:", 13) == 0 && isblank(line[13]))
    {
      xmarkStr = QCString(line+14);
    }
    else if (strncasecmp(line, "X-Status:", 9) == 0 && isblank(line[9]))
    {
      statusStr = QCString(line+10);
    }
    else if (strncasecmp(line, "In-Reply-To:", 12) == 0 && isblank(line[12]))
    {
      int rightAngle;
      replyToIdStr = QCString(line+13);

      rightAngle = replyToIdStr.find( '>' );
      if (rightAngle != -1)
	replyToIdStr.truncate( rightAngle + 1 );
    }
  }

  if (status == KMMsgStatusNew || status == KMMsgStatusUnread)
  {
    mUnreadMsgs++;
   if (mUnreadMsgs == 0) ++mUnreadMsgs;
  }

  ::chdir(path_buffer);
}

int KMFolderMaildir::createIndexFromContents()
{
  mUnreadMsgs = 0;

  mMsgList.clear(true);
  mMsgList.reset(INIT_MSGS);

  mChanged = false;

  // first, we make sure that all the directories are here as they
  // should be
  QFileInfo dirinfo;

  dirinfo.setFile(location() + "/new");
  if (!dirinfo.exists() || !dirinfo.isDir())
  {
    kdDebug(5006) << "Directory " << location() << "/new doesn't exist or is a file"<< endl;
    return 1;
  }
  QDir newDir(location() + "/new");
  newDir.setFilter(QDir::Files);

  dirinfo.setFile(location() + "/cur");
  if (!dirinfo.exists() || !dirinfo.isDir())
  {
    kdDebug(5006) << "Directory " << location() << "/cur doesn't exist or is a file"<< endl;
    return 1;
  }
  QDir curDir(location() + "/cur");
  curDir.setFilter(QDir::Files);

  // then, we look for all the 'cur' files
  const QFileInfoList *list = curDir.entryInfoList();
  QFileInfoListIterator it(*list);
  QFileInfo *fi;

  while ((fi = it.current()))
  {
    readFileHeaderIntern(curDir.path(), fi->fileName(), KMMsgStatusRead);
    ++it;
  }

  // then, we look for all the 'new' files
  list = newDir.entryInfoList();
  it = *list;

  while ((fi=it.current()))
  {
    readFileHeaderIntern(newDir.path(), fi->fileName(), KMMsgStatusNew);
    ++it;
  }

  if (autoCreateIndex())
  {
    emit statusMsg(i18n("Writing index file"));
    writeIndex();
  }
  else mHeaderOffset = 0;

  correctUnreadMsgsCount();

  if (kernel->outboxFolder() == this && count() > 0)
    KMessageBox::information(0, i18n("Your outbox contains messages which were "
    "most likely not created by KMail.\nPlease remove them from there, if you "
    "don't want KMail to send them."));

  needsCompact = true;

  return 0;
}

bool KMFolderMaildir::isIndexOutdated()
{
  QFileInfo new_info(location() + "/new");
  QFileInfo cur_info(location() + "/cur");
  QFileInfo index_info(indexLocation());

  if (!index_info.exists())
    return TRUE;

  return ((new_info.lastModified() > index_info.lastModified()) || 
          (cur_info.lastModified() > index_info.lastModified()));
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderMaildir::take(int idx)
{
  // first, we do the high-level stuff.. then delete later
  KMMessage *msg = KMFolderMaildirInherited::take(idx);

  if (!msg) return NULL;

  if (!msg->fileName())
    return NULL;

  // we need to look in both 'new' and 'cur' since it's possible to
  // delete a message before the folder is compacted.  since the file
  // naming and moving is done in ::compact, we can't assume any
  // location at this point
  QString abs_file(location() + "/cur/");
  abs_file += msg->fileName();

  if (QFile::exists(abs_file) == false)
  {
    abs_file = location() + "/new/";
    abs_file += msg->fileName();

    if (QFile::exists(abs_file) == false)
    {
      kdDebug(5006) << "Can't delete " << abs_file << " if it doesn't exist!" << endl;
      return msg;
    }
  }

  if(QFile::remove(abs_file) == false)
    return msg;

  return msg;
}

//-----------------------------------------------------------------------------
int KMFolderMaildir::remove()
{
  assert(name() != "");

  close(TRUE);
  unlink(QFile::encodeName(indexLocation()));

  // it would be nice if QDir could delete recursively.. but since it
  // doesn't, we have to do this hack
  QString cmd;
  cmd.sprintf("rm -rf '%s'", QFile::encodeName(location()).data());
  system(cmd.ascii());

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we are dead - no need to compact us
  return 0;
}

//-----------------------------------------------------------------------------
QString KMFolderMaildir::constructValidFileName(QString& aFileName, KMMsgStatus status)
{
  if (aFileName.isEmpty() || aFileName.isNull())
  {
    aFileName.sprintf("%ld.%d.", (long)time(NULL), getpid());
    aFileName += KApplication::randomString(5);
  }

  aFileName.truncate(aFileName.findRev(SUFFIX_REGEX));

  QString suffix;
  if ((status != KMMsgStatusNew) && (status != KMMsgStatusUnread))
  {
    suffix += ":2,";
    if (status == KMMsgStatusReplied)
      suffix += "RS";
    else
      suffix += "S";
  }

  aFileName += suffix;

  return aFileName;
}

//-----------------------------------------------------------------------------
QString KMFolderMaildir::moveInternal(const QString& oldLoc, const QString& newLoc, KMMsgInfo *mi)
{
  QString filename(mi->fileName());
  QString ret(moveInternal(oldLoc, newLoc, filename, mi->status()));

  if (filename != mi->fileName())
    mi->setFileName(filename);

  return ret;
}

//-----------------------------------------------------------------------------
QString KMFolderMaildir::moveInternal(const QString& oldLoc, const QString& newLoc, QString& aFileName, KMMsgStatus status)
{
  QString dest(newLoc);
  // make sure that our destination filename doesn't already exist
  while (QFile::exists(dest))
  {
    aFileName = "";
    constructValidFileName(aFileName, status);

    QFileInfo fi(dest);
    dest = fi.dirPath(true) + "/" + aFileName;
    mDirty = TRUE;
  }

  QDir d;
  if (d.rename(oldLoc, dest) == false)
    return QString::null;
  else
    return dest;
}

//-----------------------------------------------------------------------------
void KMFolderMaildir::msgStatusChanged(const KMMsgStatus oldStatus,
  const KMMsgStatus newStatus)
{
  // if the status of any message changes, then we need to compact
  needsCompact = true;

  KMFolderMaildirInherited::msgStatusChanged(oldStatus, newStatus);
}

#include "kmfoldermaildir.moc"
