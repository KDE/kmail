// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include "kmglobal.h"
#include "kmfolder.h"
#include "kmmessage.h"

#include <klocale.h>
#include <mimelib/mimepp.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

extern "C" 
{
#include <regex.h>
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_RE_COMP_H
#include <re_comp.h>
#endif

#define MAX_LINE 4096
#define INIT_MSGS 32

// Current version of the table of contents (index) files
#define INDEX_VERSION 1.1

// Regular expression to find the line that seperates messages in a mail
// folder:
#define MSG_SEPERATOR_START "From "
#define MSG_SEPERATOR_REGEX "^From.*..\\:..\\:...*$"

static short msgSepLen = strlen(MSG_SEPERATOR_START);


//-----------------------------------------------------------------------------
KMFolder :: KMFolder(KMFolderDir* aParent, const char* aName) : 
  KMFolderInherited(aParent, aName), mMsgInfo(INIT_MSGS)
{
  //-- in case that the compiler has problems with the static version above:
  //msgSepLen = strlen(MSG_SEPERATOR_START);

  initMetaObject();

  mStream         = NULL;
  mIndexStream    = NULL;
  mMsgs           = 0;
  mUnreadMsgs     = 0;
  mOpenCount      = 0;
  mQuiet          = 0;
  mAutoCreateIndex= TRUE;
  mFilesLocked    = FALSE;
  mIsSystemFolder = FALSE;
  mType           = "plain";
  mAcctList       = NULL;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mOpenCount>0) close(TRUE);

  //if (mAcctList) delete mAcctList;

  /* Well, this is a problem. If I add the line above then kmfolder depends
   * on kmaccount and is then not that portable. Hmm.
   */
}


//-----------------------------------------------------------------------------
const QString KMFolder::location(void) const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += name();

  return sLocation;
}


//-----------------------------------------------------------------------------
const QString KMFolder::indexLocation(void) const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name();

  return sLocation;
}


//-----------------------------------------------------------------------------
int KMFolder::open(void)
{
  int rc = 0;

  assert(name() != NULL);

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  mStream = fopen(location(), "r+"); // messages file
  if (!mStream) 
  {
    debug("Cannot open folder `%s': %s", (const char*)location(), strerror(errno));
    return errno;
  }

  if (!path().isEmpty())
  {
    mIndexStream = fopen(indexLocation(), "r+"); // index file
    if (!mIndexStream) rc = createIndexFromContents();
    else readIndex();
  }
  else
  {
    debug("No path specified for folder `" + name() +
	  "' -- Turning autoCreateIndex off");
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

  if (!rc) lock();
  mQuiet = 0;

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::create(void)
{
  int rc;

  assert(name() != NULL);
  assert(mOpenCount == 0);

  mStream = fopen(location(), "w");
  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    mIndexStream = fopen(indexLocation(), "w");
    if (!mIndexStream) return errno;
  }
  else
  {
    debug("Folder `" + name() +
	  "' has no path specified -- turning autoCreateIndex off");
    mAutoCreateIndex = FALSE;
  }

  mOpenCount++;
  mQuiet = 0;

  rc = writeIndex();
  if (!rc) lock();
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolder::close(bool aForced)
{
  int i;

  if (mDirty && mAutoCreateIndex) writeIndex();

  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  unlock();

  if (mStream) fclose(mStream);
  if (mIndexStream) fclose(mIndexStream);

  mOpenCount   = 0;
  mStream      = NULL;
  mIndexStream   = NULL;
  mMsgs        = 0;
  mUnreadMsgs  = 0;
  mActiveMsgs  = 0;
  mFilesLocked = FALSE;

  for (i=0; i<mMsgs; i++)
  {
    if (mMsgInfo[i].msg()) mMsgInfo[i].deleteMsg();
  }

  mMsgInfo.resize(INIT_MSGS);
}


//-----------------------------------------------------------------------------
int KMFolder::lock(void)
{
  int rc;

  mFilesLocked = FALSE;

  rc = flock(fileno(mStream), LOCK_EX|LOCK_NB);
  if (rc) return errno;

  if (mIndexStream >= 0)
  {
    rc = flock(fileno(mIndexStream), LOCK_EX|LOCK_NB);
    if (rc) 
    {
      rc = errno;
      flock(fileno(mStream), LOCK_UN|LOCK_NB);
      return rc;
    }
  }

  mFilesLocked = TRUE;
  debug("Folder `%s' is now locked.", (const char*)location());
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::unlock(void)
{
  int rc;

  mFilesLocked = FALSE;
  debug("Unlocking folder `%s'.", (const char*)location());

  rc = flock(fileno(mStream), LOCK_UN|LOCK_NB);
  if (rc) return errno;

  if (mIndexStream >= 0)
  {
    rc = flock(fileno(mIndexStream), LOCK_UN|LOCK_NB);
    if (rc) return errno;
  }
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::createIndexFromContents(void)
{
  char line[MAX_LINE];
  char status[8], subjStr[128], dateStr[80], fromStr[80];
  char* msg;
  unsigned long offs, size, pos;
  int i, msgArrNum;
  bool atEof = FALSE;
  KMMsgInfo* mi;
  QString msgStr;

  assert(name() != NULL);
  assert(path() != NULL);
  assert(mStream != NULL);

  rewind(mStream);

  debug("*** creating index file %s\n", (const char*)indexLocation());

  offs = 0;
  size = 0;
  strcpy(status, "RO");

  msg = re_comp(MSG_SEPERATOR_REGEX);
  if (msg)
  {
    fatal("Error in %s:%d\nwhile parsing bultin\n"
	  "regular expression\n%s\n%s", __FILE__, __LINE__,
	  MSG_SEPERATOR_REGEX, msg);
    return -1;
  }

  msgArrNum = mMsgInfo.size();

  mMsgs = -1;
  offs  = 0;
  dateStr[0] = '\0';
  fromStr[0] = '\0';
  subjStr[0] = '\0';

  while (!atEof)
  {
    if ((mMsgs & 127) == 0)
    {
      msgStr.sprintf(nls->translate("Creating index file: %d messages done"), 
		     mMsgs);
      emit statusMsg(msgStr);
    }

    pos = ftell(mStream);
    if (!fgets(line, MAX_LINE, mStream)) atEof = TRUE;

    if (atEof ||
	(strncmp(line,MSG_SEPERATOR_START, msgSepLen)==0 && re_exec(line)))
    {
      size = pos - offs;
      if (mMsgs >= 0)
      {
	if (mMsgs >= msgArrNum)
	{
	  msgArrNum <<= 1;
	  mMsgInfo.resize(msgArrNum);
	}

	mi = &mMsgInfo[mMsgs];
	mi->init(KMMessage::stNew, offs, size);
	mi->setFrom(fromStr);
	mi->setSubject(subjStr);
	mi->setDate(dateStr);
	mi->setDirty(FALSE);

	strcpy(status, "RO");
	dateStr[0] = '\0';
	fromStr[0] = '\0';
	subjStr[0] = '\0';
      }

      offs = ftell(mStream);
      mMsgs++;
    }
    else if (strncmp(line, "Status: ", 8) == 0)
    {
      for(i=0; i<8 && line[i+8] > ' '; i++)
	status[i] = line[i+8];
      status[i] = '\0';
    }
    else if (strncmp(line, "Date: ", 6) == 0)
    {
      strncpy(dateStr, line+6, 59);
      dateStr[39] ='\0';
    }
    else if (strncmp(line, "From: ", 6) == 0)
    {
      strncpy(fromStr, line+6, 59);
      fromStr[39] ='\0';
    }
    else if (strncmp(line, "Subject: ", 9) == 0)
    {
      strncpy(subjStr, line+9, 79);
      subjStr[79] ='\0';
    }
  }

  if (mAutoCreateIndex) writeIndex();
  else mHeaderOffset = 0;

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::writeIndex(void)
{
  int rc=0, i=0;

  if (mIndexStream) fclose(mIndexStream);
  mIndexStream = fopen(indexLocation(), "w");
  if (!mIndexStream) return errno;

  fprintf(mIndexStream, "# KMail-Index V%g", INDEX_VERSION);

  mHeaderOffset = ftell(mIndexStream);
  for (i=0; i<mMsgs; i++)
  {
    fprintf(mIndexStream, "%s\n", (const char*)mMsgInfo[i].asString());
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
bool KMFolder::readIndexHeader(void)
{
  float indexVersion;

  assert(mIndexStream != NULL);

  fscanf(mIndexStream, "# KMail-Index V%g", &indexVersion);
  if (indexVersion < INDEX_VERSION)
  {
    debug("Index index file %s is out of date. Re-creating it.", 
	  (const char*)indexLocation());
    createIndexFromContents();
    return FALSE;
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolder::readIndex(void)
{
  char line[MAX_LINE];
  int  msgArrNum;

  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgs       = 0;
  mUnreadMsgs = 0;
  mActiveMsgs = 0;

  if (!readIndexHeader()) return;

  mHeaderOffset = ftell(mIndexStream);
  msgArrNum = mMsgInfo.size();

  for (; !feof(mIndexStream); mMsgs++)
  {
    fgets(line, MAX_LINE, mIndexStream);
    if (feof(mIndexStream)) break;

    if (mMsgs >= msgArrNum)
    {
      msgArrNum <<= 1;
      mMsgInfo.resize(msgArrNum);
    }

    mMsgInfo[mMsgs].fromString(line);

    if (mMsgInfo[mMsgs].status() == KMMessage::stDeleted) 
    {
      mMsgs--;   // skip deleted messages
      continue;
    }
    mActiveMsgs++;
    if (mMsgInfo[mMsgs].status() == KMMessage::stUnread ||
	mMsgInfo[mMsgs].status() == KMMessage::stNew)
    {
      mUnreadMsgs++;
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolder::quiet(bool beQuiet)
{
  if (beQuiet) mQuiet++;
  else 
  {
    mQuiet--;
    if (mQuiet <= 0)
    {
      mQuiet = 0;
      emit changed();
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolder::setMsgStatus(KMMessage* aMsg, KMMessage::Status aStatus)
{
  int i = indexOfMsg(aMsg);
  if (i <= 0) 
  {
    debug("KMFolder::setMsgStatus() called for a message that is not in"
	  "this folder !");
    return;
  }

  mMsgInfo[i-1].setStatus(aStatus);
  if (!mQuiet) emit msgHeaderChanged(i);
}


//-----------------------------------------------------------------------------
int KMFolder::indexOfMsg(const KMMessage* aMsg) const
{
  int i;

  for (i=0; i<mMsgs; i++)
    if (mMsgInfo[i].msg() == aMsg) return i+1;

  return -1;
}


//-----------------------------------------------------------------------------
void KMFolder::detachMsg(KMMessage* aMsg)
{
  int msgno = indexOfMsg(aMsg);
  detachMsg(msgno);
}


//-----------------------------------------------------------------------------
void KMFolder::detachMsg(int msgno)
{
  KMMsgInfo* mi;
  KMMessage* msg;

  assert(mStream != NULL);
  assert(msgno >= 1 && msgno <= mMsgs);

  mi = &mMsgInfo[msgno-1];
  msg = mi->msg();

  if (msg)
  {
    msg->setOwner(NULL);
    mi->setMsg(NULL);
  }
  else debug("KMFolder::detachMsg() called for non-loaded message");

  removeMsgInfo(msgno);

  if (!mQuiet) emit msgRemoved(msgno);
}


//-----------------------------------------------------------------------------
KMMessage *KMFolder::getMsg(int msgno)
{
  KMMsgInfo* mi;

  assert(mStream != NULL);
  assert(msgno >= 1 && msgno <= mMsgs);

  mi = &mMsgInfo[msgno-1];

  if (!mi->msg()) readMsg(msgno);
  return mi->msg();
}


//-----------------------------------------------------------------------------
void KMFolder::readMsg(int msgno)
{
  KMMessage* msg;
  DwMessage* dwmsg;
  unsigned long msgSize;

  assert(mStream != NULL);
  assert(msgno >= 1 && msgno <= mMsgs);
  msgno--;

  msg = new KMMessage(this);

  msgSize = mMsgInfo[msgno].size();
  msg->msgStr().resize(msgSize+2);

  fseek(mStream, mMsgInfo[msgno].offset(), SEEK_SET);

  fread((char*)msg->msgStr().data(), msgSize, 1, mStream);
  ((char*)msg->msgStr().data())[msgSize] = '\0';

  mMsgInfo[msgno].setMsg(msg);
  msg->setStatus(mMsgInfo[msgno].status());
  dwmsg = DwMessage::NewMessage(msg->msgStr(), 0);
  dwmsg->Parse();

  msg->takeMessage(dwmsg);
}


//-----------------------------------------------------------------------------
int KMFolder::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  KMFolder* msgOwner;
  int rc;

  assert(aMsg != NULL);
  msgOwner = aMsg->owner();

  if (msgOwner)
  {
    msgOwner->open();
    msgOwner->detachMsg(aMsg);
    msgOwner->close();
  }

  open();
  rc = addMsg(aMsg, aIndex_ret);
  close();

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  long offs, size, len;
  int msgArrNum = mMsgInfo.size();
  bool opened = FALSE;

  if (!mStream)
  {
    opened = TRUE;
    open();
  }

  assert(mStream != NULL);
  len = strlen(aMsg->asString());

  if (len <= 0)
  {
    debug("KMFolder::addMsg():\nmessage contains no data !");
    if (opened) close();
    return 0;
  }

  if (mMsgs >= msgArrNum)
  {
    if (msgArrNum < 16) msgArrNum = 16;
    msgArrNum <<= 1;
    mMsgInfo.resize(msgArrNum);
  }

  fseek(mStream, 0, SEEK_END);
  offs = ftell(mStream);

  fwrite("From ???@??? 00:00:00 1997 +0000\n", 33, 1, mStream);
  fwrite((char*)aMsg->asString(), len, 1, mStream);
  fflush(mStream);

  size = ftell(mStream) - offs;

  mMsgInfo[mMsgs].init(aMsg->status(), offs, size, aMsg);
  aMsg->setOwner(this);

  if (mAutoCreateIndex)
  {
    debug("writing new index entry to index file");
    assert(mIndexStream != NULL);
    fseek(mIndexStream, 0, SEEK_END);
    fprintf(mIndexStream, "%s\n", (const char*)mMsgInfo[mMsgs].asString()); 
    fflush(mIndexStream);
  }

  if (aIndex_ret) *aIndex_ret = mMsgs;
  mMsgs++;

  if (!mQuiet) emit msgAdded(mMsgs);

  if (opened) close();
  return 0;
} 


//-----------------------------------------------------------------------------
int KMFolder::remove(void)
{
  int rc;

  assert(name() != NULL);

  if (mOpenCount > 0) 
  {
    mOpenCount = 1; // force a close
    close();
  }

  unlink(indexLocation());
  rc = unlink(location());
  if (rc) return rc;

  mMsgInfo.truncate(INIT_MSGS);
  mMsgs = 0;

  return 0;
}
	

//-----------------------------------------------------------------------------
int KMFolder::isValid(unsigned long)
{
  return FALSE;
}


//-----------------------------------------------------------------------------
int KMFolder::expunge(void)
{
  int openCount = mOpenCount;

  assert(name() != NULL);

  close(TRUE);

  if (mAutoCreateIndex) truncate(indexLocation(), mHeaderOffset);
  else unlink(indexLocation());

  if (truncate(location(), 0)) return errno;

  mMsgInfo.truncate(INIT_MSGS);
  mMsgs = 0;

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  if (!mQuiet) emit changed();

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::sync(void)
{
  int i, rc;
  KMMsgInfo* mi;
  unsigned long offset = mHeaderOffset;
  int recSize = KMMsgInfo::recSize();

  if (!mIndexStream) return 0;

  for (rc=0,i=0; i<mMsgs; i++)
  {
    mi = &mMsgInfo[i];
    if (mi->dirty())
    {
      fseek(mIndexStream, offset, SEEK_SET);
      fprintf(mIndexStream, "%s\n", (const char*)mi->asString());
      rc = errno;
      if (rc) break;
      mi->setDirty(FALSE);
    }
    offset += recSize;
  }
  fflush(mIndexStream);
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsgInfo(int msgId)
{
  int i;

  assert(msgId>=1 && msgId<=mMsgs);
  msgId--;

  for (i=msgId; i<mMsgs-1; i++)
  {
    mMsgInfo[i] = mMsgInfo[i+1];
  }
  mMsgs--;

  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMFolder::msgSubject(int msgId) const
{
  assert(msgId>0);
  return mMsgInfo[msgId-1].subject();
}


//-----------------------------------------------------------------------------
const char* KMFolder::msgFrom(int msgId) const
{
  assert(msgId>0);
  return mMsgInfo[msgId-1].from();
}


//-----------------------------------------------------------------------------
const char* KMFolder::msgDate(int msgId) const
{
  assert(msgId>0);
  return mMsgInfo[msgId-1].dateStr();
}


//-----------------------------------------------------------------------------
KMMessage::Status KMFolder::msgStatus(int msgId) const
{
  assert(msgId>0);
  return mMsgInfo[msgId-1].status();
}


//-----------------------------------------------------------------------------
static KMMsgInfoList* sortMsgInfoList;
static KMFolder::SortField sortCriteria;

int msgSortCompFunc(const void* a, const void* b)
{
  KMMsgInfo  *miA = &sortMsgInfoList->at(*(int*)a);
  KMMsgInfo  *miB = &sortMsgInfoList->at(*(int*)b);
  int        res = 0;

  if (sortCriteria==KMFolder::sfSubject)
    res = miA->compareBySubject(miB);

  else if (sortCriteria==KMFolder::sfFrom)
    res = miA->compareByFrom(miB);

  if (res==0 || sortCriteria==KMFolder::sfDate)
    res = miA->compareByDate(miB);

  return res;
}


//-----------------------------------------------------------------------------
void KMFolder::sort(KMFolder::SortField aField)
{
  int sortIndex[mMsgs];
  int i;
  KMMsgInfoList newList(mMsgs);

  for (i=0; i<mMsgs; i++)
    sortIndex[i] = i;

  sortMsgInfoList = &mMsgInfo;
  sortCriteria = aField;

  qsort(sortIndex, mMsgs, sizeof(int), msgSortCompFunc);

  for (i=0; i<mMsgs; i++)
    newList[i] = mMsgInfo[sortIndex[i]];

  mMsgInfo  = newList;
  mDirty = TRUE;

  if (!mQuiet) emit changed();
}


//-----------------------------------------------------------------------------
const char* KMFolder::type(void) const
{
  if (mAcctList) return "in";
  return KMFolderInherited::type();
}


//-----------------------------------------------------------------------------
const QString KMFolder::label(void) const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  return name();
}


//-----------------------------------------------------------------------------
#include "kmfolder.moc"
