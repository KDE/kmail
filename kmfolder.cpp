// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qfileinfo.h>

#include "kmglobal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmfolderdir.h"
#include "kbusyptr.h"
#include "kmundostack.h"

#include <kapp.h>
#include <kconfig.h>
#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <kmessagebox.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_FCNTL_H && !HAVE_FLOCK
#include <fcntl.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _BSD_COMPAT
#define _BSD_COMPAT // flock is under BSD compliance oat least on IRIX
#endif

#include <sys/file.h>
#include <klocale.h>

#ifndef isblank
#  define isblank(x) ((x)==' '||(x)=='\t')
#endif

#define MAX_LINE 4096
#define INIT_MSGS 8

// Current version of the table of contents (index) files
#define INDEX_VERSION 1300

// Regular expression to find the line that seperates messages in a mail
// folder:
#define MSG_SEPERATOR_START "From "
#define MSG_SEPERATOR_REGEX "^From .*..:...*$"
static short msgSepLen = strlen(MSG_SEPERATOR_START);

extern KBusyPtr *kbp;

static int _rename(const char* oldname, const char* newname)
{
  return rename(oldname, newname);
}


//-----------------------------------------------------------------------------
KMFolder :: KMFolder(KMFolderDir* aParent, const QString& aName) : 
  KMFolderInherited(aParent, aName), mMsgList(INIT_MSGS)
{
  //-- in case that the compiler has problems with the static version above:
  //msgSepLen = strlen(MSG_SEPERATOR_START);

  initMetaObject();

  mStream         = NULL;
  mIndexStream    = NULL;
  mOpenCount      = 0;
  mQuiet          = 0;
  mChanged        = FALSE;
  mHeaderOffset   = 0;
  mAutoCreateIndex= TRUE;
  mFilesLocked    = FALSE;
  mIsSystemFolder = FALSE;
  mType           = "plain";
  mAcctList       = NULL;
  mDirty          = FALSE;
  unreadMsgs      = -1;
  needsCompact    = FALSE;
  mChild          = 0;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mOpenCount>0) close(TRUE);

  //if (mAcctList) delete mAcctList;
  /* Well, this is a problem. If I add the above line then kmfolder depends
   * on kmaccount and is then not that portable. Hmm.
   */
  if (undoStack) undoStack->folderDestroyed(this);
}


//-----------------------------------------------------------------------------
const QString KMFolder::location() const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += name();

  return sLocation;
}


//-----------------------------------------------------------------------------
const QString KMFolder::indexLocation() const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name();
  sLocation += ".index";

  return sLocation;
}

//-----------------------------------------------------------------------------
const QString KMFolder::subdirLocation() const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name();
  sLocation += ".directory";

  return sLocation;  
}

//-----------------------------------------------------------------------------
KMFolderDir* KMFolder::createChildFolder()
{
  QString childName = "." + name() + ".directory";
  QString childDir = path() + "/" + childName;
  bool ok = true;

  if (mChild)
    return mChild;

  if (access(childDir, W_OK) != 0) // Not there or not writable
  {
    if (mkdir(childDir, S_IRWXU) != 0 && chmod(childDir, S_IRWXU) != 0)
      ok=false; //failed create new or chmod existing tmp/
  }

  if (!ok) {
    QString wmsg = QString(" '%1': %2").arg(childDir).arg(strerror(errno)); 
    KMessageBox::information(0,i18n("Failed to create directory") + wmsg);
    return 0;
  }

  KMFolderDir* folderDir = new KMFolderDir(parent(), childName);
  if (!folderDir)
    return 0;
  folderDir->reload();
  parent()->append(folderDir);
  mChild = folderDir;
  return folderDir;
}

//-----------------------------------------------------------------------------
int KMFolder::open()
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(name() != "");

  mFilesLocked = FALSE;
  mStream = fopen(location(), "r+"); // messages file
  if (!mStream) 
  {
    debug("Cannot open folder `%s': %s", (const char*)location(), 
	  strerror(errno));
    mOpenCount = 0;
    return errno;
  }

  if (!path().isEmpty())
  {
    if (isIndexOutdated()) // test if contents file has changed
    {
      QString str;
      mIndexStream = NULL;
      str = i18n("Folder `%1' changed. Recreating index.") 
		  .arg(name());
      emit statusMsg(str);
    }
    else mIndexStream = fopen(indexLocation(), "r+"); // index file

    if (!mIndexStream) rc = createIndexFromContents();
    else readIndex();
  }
  else
  {
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

  if (!rc) lock();
  mQuiet = 0;
  mChanged = FALSE;

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::create()
{
  int rc;
  int old_umask;

  assert(name() != "");
  assert(mOpenCount == 0);

  debug( "Creating folder " + location() );
  if (access(location(), F_OK) == 0) {
    debug("KMFolder::create call to access function failed.");
    debug("File:: " + location());
    debug("Error " + QString( strerror(errno) ));
    return EEXIST;
  }

  old_umask = umask(077);
  mStream = fopen(location(), "w+"); //sven; open RW
  umask(old_umask);

  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    old_umask = umask(077);
    mIndexStream = fopen(indexLocation(), "w+"); //sven; open RW
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

  rc = writeIndex();
  if (!rc) lock();
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolder::close(bool aForced)
{
  if (mOpenCount <= 0 || !mStream) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  if (mAutoCreateIndex) 
  {
    if (mDirty) writeIndex();
    else sync();
    writeConfig();
  }

  unlock();
  mMsgList.clear(TRUE);

  if (mStream) fclose(mStream);
  if (mIndexStream) fclose(mIndexStream);

  mOpenCount   = 0;
  mStream      = NULL;
  mIndexStream = NULL;
  mFilesLocked = FALSE;

  mMsgList.reset(INIT_MSGS);
}


//-----------------------------------------------------------------------------
int KMFolder::lock()
{
  int rc;
#if !HAVE_FLOCK
  struct flock fl;
  fl.l_type=F_WRLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
#endif

  assert(mStream != NULL);
  mFilesLocked = FALSE;

#if HAVE_FLOCK
  rc = flock(fileno(mStream), LOCK_NB|LOCK_EX);
#else
  rc = fcntl(fileno(mStream), F_SETLK, &fl);
#endif

  if (rc < 0)
  {
    debug("Cannot lock folder `%s': %s (%d)", (const char*)location(),
	  strerror(errno), errno);
    return errno;
  }

  if (mIndexStream)
  {
#if HAVE_FLOCK
    rc = flock(fileno(mIndexStream), LOCK_UN);
#else
    rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);
#endif

    if (rc < 0) 
    {
      debug("Cannot lock index of folder `%s': %s", (const char*)location(),
	    strerror(errno));
      rc = errno;
#if HAVE_FLOCK
      rc = flock(fileno(mIndexStream), LOCK_UN);
#else
      fl.l_type = F_UNLCK;
      rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);
#endif
      return rc;
    }
  }

  mFilesLocked = TRUE;
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::unlock()
{
  int rc;
#if !HAVE_FLOCK
  struct flock fl;
  fl.l_type=F_UNLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
#endif

  assert(mStream != NULL);
  mFilesLocked = FALSE;

#if HAVE_FLOCK
  if (mIndexStream) flock(fileno(mIndexStream), LOCK_UN);
  rc = flock(fileno(mStream), LOCK_UN);
#else
  if (mIndexStream) fcntl(fileno(mIndexStream), F_SETLK, &fl);
  rc = fcntl(fileno(mStream), F_SETLK, F_UNLCK);
#endif

  return errno;
}


//-----------------------------------------------------------------------------
bool KMFolder::isIndexOutdated()
{
  QFileInfo contInfo(location());
  QFileInfo indInfo(indexLocation());

  if (!contInfo.exists()) return FALSE;
  if (!indInfo.exists()) return TRUE;

  return (contInfo.lastModified() > indInfo.lastModified());
}


//-----------------------------------------------------------------------------
int KMFolder::createIndexFromContents()
{
  char line[MAX_LINE];
  char status[8], xstatus[8];
  QString subjStr, dateStr, fromStr, toStr, xmarkStr, *lastStr=NULL;
  QString whoFieldName;
  unsigned long offs, size, pos;
  bool atEof = FALSE;
  bool inHeader = TRUE;
  KMMsgInfo* mi;
  QString msgStr;
  QRegExp regexp(MSG_SEPERATOR_REGEX);
  int i, num, numStatus, whoFieldLen;
  short needStatus;

  assert(mStream != NULL);
  rewind(mStream);

  mMsgList.clear();

  num     = -1;
  numStatus= 11;
  offs    = 0;
  size    = 0;
  dateStr = "";
  fromStr = "";
  toStr = "";
  subjStr = "";
  *status = '\0';
  *xstatus = '\0';
  xmarkStr = "";
  needStatus = 3;
  whoFieldName = QString(whoField()) + ':'; //unused (sven)
  whoFieldLen = whoFieldName.length();      //unused (sven)

  //debug("***whoField: %s (%d)",
  //      (const char*)whoFieldName, whoFieldLen);

  while (!atEof)
  {
    pos = ftell(mStream);
    if (!fgets(line, MAX_LINE, mStream)) atEof = TRUE;

    if (atEof ||
	(strncmp(line,MSG_SEPERATOR_START, msgSepLen)==0 && 
	 regexp.match(line) >= 0))
    {
      size = pos - offs;
      pos = ftell(mStream);

      if (num >= 0)
      {
	if (numStatus <= 0)
	{
	  msgStr = i18n("Creating index file: %1 messages done").arg(num);
	  emit statusMsg(msgStr);
	  numStatus = 10;
	}

	if (size > 0)
	{
	  mi = new KMMsgInfo(this);
	  mi->init(subjStr, fromStr, toStr, 0, KMMsgStatusNew, xmarkStr, offs, size);
	  mi->setStatus(status,xstatus);
	  mi->setDate(dateStr);
	  mi->setDirty(FALSE);
	  mMsgList.append(mi);

	  *status = '\0';
	  *xstatus = '\0';
	  needStatus = 3;
	  xmarkStr = "";
	  dateStr = "";
	  fromStr = "";
	  subjStr = "";
	}
	else num--,numStatus++;
      }

      offs = ftell(mStream);
      num++;
      numStatus--;
      inHeader = TRUE;
      continue;
    }
    // Is this a long header line?
    if (inHeader && (line[0]=='\t' || line[0]==' '))
    {
      i = 0;
      while (line [i]=='\t' || line [i]==' ') i++;
      if (line [i] < ' ' && line [i]>0) inHeader = FALSE;
      else if (lastStr) *lastStr += line + i;
    } 
    else lastStr = NULL;

    if (inHeader && (line [0]=='\n' || line [0]=='\r'))
      inHeader = FALSE;
    if (!inHeader) continue;

    if ((needStatus & 1) && strncasecmp(line, "Status:", 7) == 0 && 
	isblank(line[7])) 
    {
      for(i=0; i<4 && line[i+8] > ' '; i++)
	status[i] = line[i+8];
      status[i] = '\0';
      needStatus &= ~1;
    }
    else if ((needStatus & 2) && strncasecmp(line, "X-Status:", 9)==0 &&
	     isblank(line[9]))
    {
      for(i=0; i<4 && line[i+10] > ' '; i++)
	xstatus[i] = line[i+10];
      xstatus[i] = '\0';
      needStatus &= ~2;
    }
    else if (strncasecmp(line,"X-KMail-Mark:",13)==0 && isblank(line[13]))
      xmarkStr = QString(line+14).copy();
    else if (strncasecmp(line,"Date:",5)==0 && isblank(line[5]))
    {
      dateStr = QString(line+6).copy();
      lastStr = &dateStr;
    }
    else if (strncasecmp(line,"From:", 5)==0 &&
	     isblank(line[5]))
    {
      fromStr = QString(line+6).copy();
      lastStr = &fromStr;
    }
    else if (strncasecmp(line,"To:", 3)==0 &&
	     isblank(line[3]))
    {
      toStr = QString(line+4).copy();
      lastStr = &toStr;
    }
    else if (strncasecmp(line,"Subject:",8)==0 && isblank(line[8]))
    {
      subjStr = QString(line+9).copy();
      lastStr = &subjStr;
    }
  }

  if (mAutoCreateIndex)
  {
    emit statusMsg(i18n("Writing index file"));
    writeIndex();
  }
  else mHeaderOffset = 0;

  correctUnreadMsgsCount();
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::writeIndex()
{
  KMMsgBase* msgBase;
  int old_umask;
  int i=0;

  if (mIndexStream) fclose(mIndexStream);
  old_umask = umask(077);
  mIndexStream = fopen(indexLocation(), "w");
  umask(old_umask);
  if (!mIndexStream) return errno;

  fprintf(mIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);

  mHeaderOffset = ftell(mIndexStream);
  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    fprintf(mIndexStream, "%s\n", (const char*)msgBase->asIndexString());
  }
  fflush(mIndexStream);

  mDirty = FALSE;
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}


//-----------------------------------------------------------------------------
bool KMFolder::readIndexHeader()
{
  int indexVersion;

  assert(mIndexStream != NULL);

  fscanf(mIndexStream, "# KMail-Index V%d\n", &indexVersion);
  if (indexVersion < INDEX_VERSION)
  {
    debug("Index file %s is out of date. Re-creating it.", 
	  (const char*)indexLocation());
    createIndexFromContents();
    return FALSE;
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolder::readIndex()
{
  char line[MAX_LINE];
  KMMsgInfo* mi;

  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgList.clear();
  if (!readIndexHeader()) return;

  mDirty = FALSE;
  mHeaderOffset = ftell(mIndexStream);

  mMsgList.clear();
  while (!feof(mIndexStream))
  {
    fgets(line, MAX_LINE, mIndexStream);
    if (feof(mIndexStream)) break;

    mi = new KMMsgInfo(this);
    mi->fromIndexString(line);
    if (mi->status() == KMMsgStatusDeleted) 
    {
      delete mi;  // skip messages that are marked as deleted
      mDirty = TRUE;
      needsCompact = true;  //We have deleted messages - needs to be compacted
      continue;
    }
#ifdef OBSOLETE
    else if (mi->status() == KMMsgStatusNew)
    {
      mi->setStatus(KMMsgStatusUnread);
      mi->setDirty(FALSE);
    }
#endif
    mMsgList.append(mi);
  }
}


//-----------------------------------------------------------------------------
void KMFolder::markNewAsUnread()
{
  KMMsgBase* msgBase;
  int i;

  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    if (msgBase->status() == KMMsgStatusNew)
    {
      msgBase->setStatus(KMMsgStatusUnread);
      msgBase->setDirty(TRUE);
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolder::quiet(bool beQuiet)
{
  if (beQuiet)
    mQuiet++;
  else {
    mQuiet--;
    if (mQuiet <= 0)
    {
      mQuiet = 0;
      if (mChanged)
	emit changed();
      mChanged = FALSE;
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(KMMsgBasePtr aMsg)
{
  removeMsg(find(aMsg));
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(int idx)
{
  //assert(idx>=0);
  if(idx < 0)
    {
      debug("KMFolder::removeMsg() : idx < 0\n");
      return;
    }
  mMsgList.take(idx);
  mDirty = TRUE;
  if (!mQuiet) 
    emit msgRemoved(idx);
  else
    mChanged = TRUE;
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::take(int idx)
{
  KMMsgBase* mb;
  KMMessage* msg;

  assert(mStream!=NULL);
  assert(idx>=0 && idx<=mMsgList.high());

  mb = mMsgList[idx];
  if (!mb) return NULL;
  if (!mb->isMessage()) readMsg(idx);

  msg = (KMMessage*)mMsgList.take(idx);
  if (msg->status()==KMMsgStatusUnread ||
      msg->status()==KMMsgStatusNew) {
    --unreadMsgs;
    emit numUnreadMsgsChanged( this );
  }
  msg->setParent(NULL);
  mDirty = TRUE;
  needsCompact=true; // message is taken from here - needs to be compacted
  if (!mQuiet)
    emit msgRemoved(idx);
  else
    mChanged = TRUE;

  return msg;
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::getMsg(int idx)
{
  KMMsgBase* mb;

  // assert(idx>=0 && idx<=mMsgList.high());
  if(!(idx >= 0 && idx <= mMsgList.high()))
    return 0L;

  mb = mMsgList[idx];
  if (!mb) return NULL;

  if (mb->isMessage()) return ((KMMessage*)mb);
  return readMsg(idx);
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::readMsg(int idx)
{
  KMMessage* msg;
  unsigned long msgSize;
  QCString msgText;
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=NULL && !mi->isMessage());
  assert(mStream != NULL);

  msgSize = mi->msgSize();
  msgText.resize(msgSize+2);

  fseek(mStream, mi->folderOffset(), SEEK_SET);
  fread(msgText.data(), msgSize, 1, mStream);
  msgText[msgSize] = '\0';

  msg = new KMMessage(*mi);
  msg->fromString(msgText);

  mMsgList.set(idx,msg);

  return msg;
}


//-----------------------------------------------------------------------------
int KMFolder::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  KMFolder* msgParent;
  int rc;

  assert(aMsg != NULL);
  msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  open();
  rc = addMsg(aMsg, aIndex_ret);
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  long offs, size, len, revert;
  bool opened = FALSE;
  QString msgText;
  char endStr[3];
  int idx = -1, rc;
  KMFolder* msgParent;
  bool editing = false;

  if (!mStream)
  {
    opened = TRUE;
    rc = open();
    debug("addMsg-open: %d", rc);
    if (rc) return rc;
  }

  // take message out of the folder it is currently in, if any
  msgParent = aMsg->parent();
  if (msgParent)
  {
    if (msgParent==this)
      {
	if (name() == "outbox") //special case for Edit message.
	  {    
	    // debug ("Editing message in outbox");
	    editing = true;
	  }
	else
	  return 0;
      }

    idx = msgParent->find(aMsg);
    msgParent->getMsg( idx );
  }

  aMsg->setStatusFields();
  msgText = aMsg->asString();
  len = msgText.length();

  assert(mStream != NULL);
  clearerr(mStream);
  if (len <= 0)
  {
    debug("Message added to folder `%s' contains no data. Ignoring it.",
	  (const char*)name());
    if (opened) close();
    return 0;
  }

  // write message to folder file
  fseek(mStream, -2, SEEK_END);
  fread(endStr, 1, 2, mStream); // ensure separating empty line
  if (ftell(mStream) > 0 && endStr[0]!='\n') 
  {
    if (endStr[1]!='\n')
    {
      //printf ("****endStr[1]=%c\n", endStr[1]);
      fwrite("\n\n", 1, 2, mStream);
    }
    else fwrite("\n", 1, 1, mStream);
  }
  fseek(mStream,0,SEEK_END); // this is needed on solaris and others
  revert = ftell(mStream);
  int error = ferror(mStream);
  if (error) {
    if (opened) close();
    return error;
  }
	     
  fwrite("From aaa@aaa Mon Jan 01 00:00:00 1997\n", 38, 1, mStream);
  offs = ftell(mStream);
  fwrite(msgText, len, 1, mStream);
  if (msgText[(int)len-1]!=QChar('\n')) fwrite("\n\n", 1, 2, mStream);
  fflush(mStream);
  size = ftell(mStream) - offs;

  error = ferror(mStream);
  if (error) {
    debug( "Error: Could not add message to folder (No space left on device?)" );
    if (ftell(mStream) > revert) {
      debug("Undoing changes");
      truncate( location(), revert );
    }
    kbp->idle();
    KMessageBox::sorry(0,
	  i18n("Unable to add message to folder.\n"
	       "(No space left on device or insufficient quota?)\n\n"
	       "Free space and sufficient quota are required to continue safely."));
    kbp->busy();
    if (opened) close();
    return error;
  }

  if (msgParent) {
    if (idx >= 0) msgParent->take(idx);
  }

  if (aMsg->status()==KMMsgStatusUnread ||
      aMsg->status()==KMMsgStatusNew) {
    ++unreadMsgs;
    emit numUnreadMsgsChanged( this );
  }

  // store information about the position in the folder file in the message
  aMsg->setParent(this);
  aMsg->setFolderOffset(offs);
  aMsg->setMsgSize(size);
  idx = mMsgList.append(aMsg);

  // write index entry if desired
  if (mAutoCreateIndex)
  {
    assert(mIndexStream != NULL);
    clearerr(mIndexStream);
    fseek(mIndexStream, 0, SEEK_END);
    revert = ftell(mIndexStream);
    fprintf(mIndexStream, "%s\n", (const char*)aMsg->asIndexString()); 
    fflush(mIndexStream);
    error = ferror(mIndexStream);
    if (error) {
      debug( "Error: Could not add message to folder (No space left on device?)" );
      if (ftell(mIndexStream) > revert) {
	debug("Undoing changes");
	truncate( indexLocation(), revert );
      }
      kbp->idle();
      KMessageBox::sorry(0,
        i18n("Unable to add message to folder.\n"
	     "(No space left on device or insufficient quota?)\n\n"
	     "Free space and sufficient quota are required to continue safely."));
      kbp->busy();
      if (opened) close();
      return error;
    }
  }

  // some "paper work"
  if (aIndex_ret) *aIndex_ret = idx;
  if (!mQuiet) 
    emit msgAdded(idx);
  else
    mChanged = TRUE;

  if (opened) close();

  // All streams have been flushed without errors if we arrive here
  // Return success!
  // (Don't return status of stream, it may have been closed already.)
  return 0;
} 


//-----------------------------------------------------------------------------
int KMFolder::rename(const QString& aName, KMFolderDir *aParent)
{
  QString oldLoc, oldIndexLoc, newLoc, newIndexLoc, oldName;
  QString oldSubDirLoc, newSubDirLoc;
  int rc=0, openCount=mOpenCount;
  KMFolderDir *oldParent;

  assert(!aName.isEmpty());

  oldLoc = location().copy();
  oldIndexLoc = indexLocation().copy();
  oldSubDirLoc = subdirLocation().copy();

  close(TRUE);

  oldName = name();
  oldParent = parent();
  if (aParent)
    setParent( aParent );    

  setName(aName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = subdirLocation();

  if (_rename(oldLoc, newLoc)) {
    setName(oldName);
    setParent(oldParent);
    rc = errno;
  }
  else if (!oldIndexLoc.isEmpty()) {
    _rename(oldIndexLoc, newIndexLoc);
    if (!_rename(oldSubDirLoc, newSubDirLoc )) {
      KMFolderDir* fdir = parent();
      KMFolderNode* fN;

      for (fN = fdir->first(); fN != 0; fN = fdir->next())
	if (fN->name() == "." + oldName + ".directory" ) {
	  fN->setName( "." + name() + ".directory" );
	  break;
	}
    }
  }
  
  if (aParent) {
    if (oldParent->findRef( this ) != -1)
      oldParent->take();
    aParent->inSort( this );
    if (mChild) {
      if (mChild->parent()->findRef( mChild ) != -1)
	mChild->parent()->take();
      aParent->inSort( mChild );
    }
  }
  
  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::remove()
{
  int rc;

  assert(name() != "");

  close(TRUE);
  unlink(indexLocation());
  rc = unlink(location());
  if (rc) return rc;

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we are dead - no need to compact us
  return 0;
}
	

//-----------------------------------------------------------------------------
int KMFolder::expunge()
{
  int openCount = mOpenCount;

  assert(name() != "");

  close(TRUE);

  if (mAutoCreateIndex) truncate(indexLocation(), mHeaderOffset);
  else unlink(indexLocation());

  if (truncate(location(), 0)) return errno;
  mDirty = FALSE;

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we're cleared and truncated no need to compact

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  unreadMsgs = 0;
  emit numUnreadMsgsChanged( this );
  if (!mQuiet) 
    emit changed();
  else
    mChanged = TRUE;
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::compact()
{
  KMFolder* tempFolder;
  KMMessage* msg;
  QString tempName;
  QString msgStr;
  int openCount = mOpenCount;
  int num, numStatus;
  int rc = 0;

  if (!needsCompact)
    return 0;
  debug( "Compacting " + name() );
  tempName = "." + name();
  
  tempName += ".compacted";
  unlink(path() + "/" + tempName);
  tempFolder = new KMFolder(parent(), tempName);   //sven: we create it
  if(tempFolder->create()) {
    debug("KMFolder::compact() Creating tempFolder failed!\n");
    delete tempFolder;                             //sven: and we delete it
    return 0;
  }

  quiet(TRUE);
  tempFolder->open();
  open();

  for(num=1,numStatus=9; count() > 0; num++, numStatus--)
  {
    if (numStatus <= 0)
    {
      msgStr = i18n("Compacting folder: %1 messages done").arg(num);
      emit statusMsg(msgStr);
      numStatus = 10;
    }

    msg = getMsg(0);
    if(msg)
      rc = tempFolder->moveMsg(msg);
    if (rc)
      break;
  }
  tempName = tempFolder->location();
  tempFolder->close(TRUE);
  close(TRUE);
  mMsgList.clear(TRUE);

  if (!rc) { 
    _rename(tempName, location());
    _rename(tempFolder->indexLocation(), indexLocation());
  }
  else
  {
    debug( "Error occurred while compacting" );
    debug( "Compaction aborted." );
  }

  // Now really free all memory
  delete tempFolder;                //sven: we delete it, not the manager

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }
  quiet(FALSE);

  if (!mQuiet) 
    emit changed();
  else
    mChanged = TRUE;
  needsCompact = false;             // We are clean now
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::sync()
{
  KMMsgBasePtr mb;
  unsigned long offset = mHeaderOffset;
  int i, rc, recSize = KMMsgBase::indexStringLength()+1;
  int high = mMsgList.high();

  if (!mIndexStream) return 0;

  for (rc=0,i=0; i<high; i++)
  {
    mb = mMsgList[i];
    if (mb->dirty())
    {
      fseek(mIndexStream, offset, SEEK_SET);
      fprintf(mIndexStream, "%s\n", (const char*)mb->asIndexString());
      rc = errno;
      if (rc) break;
      mb->setDirty(FALSE);
    }
    offset += recSize;
  }
  fflush(mIndexStream);

  mDirty = FALSE;
  return rc;
}


//-----------------------------------------------------------------------------
// This method is now obsolete (QListView is used for sorting) -sanders
void KMFolder::sort(KMMsgList::SortField aField, bool aDesc)
{
  debug( "KMFolder::sort redundant call" );
  mMsgList.sort(aField, aDesc);
  if (!mQuiet) 
    emit changed();
  else
    mChanged = TRUE;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMFolder::type() const
{
  if (mAcctList) return "In";
  return KMFolderInherited::type();
}


//-----------------------------------------------------------------------------
const QString KMFolder::label() const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  return name();
}


//-----------------------------------------------------------------------------
int KMFolder::countUnread()
{
  register int  i;

  if (unreadMsgs > -1)
    return unreadMsgs;

  readConfig();

  if (unreadMsgs > -1)
    return unreadMsgs;

  open();
  for (i=0, unreadMsgs=0; i<mMsgList.high(); i++)
  {
    if (mMsgList[i]->status()==KMMsgStatusUnread ||
	mMsgList[i]->status()==KMMsgStatusNew)
      unreadMsgs++;
  }
  close();

  return unreadMsgs;
}

//-----------------------------------------------------------------------------
void KMFolder::msgStatusChanged(const KMMsgStatus oldStatus,
  const KMMsgStatus newStatus)
{
  int oldUnread = 0;
  int newUnread = 0;

  if (oldStatus==KMMsgStatusUnread || oldStatus==KMMsgStatusNew)
    oldUnread = 1;
  if (newStatus==KMMsgStatusUnread || newStatus==KMMsgStatusNew)
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  if (deltaUnread != 0) {
    mDirty = TRUE;
    unreadMsgs += deltaUnread;
    emit numUnreadMsgsChanged( this );
  }
}


//-----------------------------------------------------------------------------
void KMFolder::headerOfMsgChanged(const KMMsgBase* aMsg)
{
  int idx = mMsgList.find((KMMsgBasePtr)aMsg);
  if (idx >= 0 && !mQuiet) 
    emit msgHeaderChanged(idx);
  else
    mChanged = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMFolder::whoField() const
{
  return (mWhoField.isEmpty() ? "From" : (const char*)mWhoField);
}


//-----------------------------------------------------------------------------
void KMFolder::setWhoField(const QString& aWhoField)
{
  mWhoField = aWhoField.copy();
}

//-----------------------------------------------------------------------------
QString KMFolder::idString()
{
  KMFolderNode* folderNode = parent();
  if (!folderNode)
    return "";
  while (folderNode->parent())
    folderNode = folderNode->parent();
  int pathLen = path().length() - folderNode->path().length();
  QString relativePath = path().right( pathLen );
  if (!relativePath.isEmpty())
    relativePath = relativePath.right( relativePath.length() - 1 ) + "/";
  return relativePath + QString(name());
}

//-----------------------------------------------------------------------------
void KMFolder::readConfig()
{
  KConfig* config = app->config();
  config->setGroup("Folder-" + idString());
  unreadMsgs = config->readNumEntry("UnreadMsgs", -1);
}

//-----------------------------------------------------------------------------
void KMFolder::writeConfig()
{
  KConfig* config = app->config();
  config->setGroup("Folder-" + idString());
  config->writeEntry("UnreadMsgs", countUnread());
}

//-----------------------------------------------------------------------------
void KMFolder::correctUnreadMsgsCount()
{
  int i;

  open();
  for (i=0, unreadMsgs=0; i<mMsgList.high(); i++)
  {
    if (mMsgList[i]->status()==KMMsgStatusUnread ||
	mMsgList[i]->status()==KMMsgStatusNew)
      unreadMsgs++;
  }
  close();
  emit numUnreadMsgsChanged( this );
}

//-----------------------------------------------------------------------------
#include "kmfolder.moc"
