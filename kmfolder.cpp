// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include "kmfolder.h"
#include "kmmessage.h"
#include "kmglobal.h"

#include <klocale.h>

#include <mimelib/mimepp.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" 
{
#include <regex.h>
}

#define MAX_LINE 4096
#define INIT_MSGS 32

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

  mStream     = NULL;
  mTocStream  = NULL;
  mMsgs       = 0;
  mUnreadMsgs = 0;
  mOpenCount  = 0;
  mQuiet      = 0;
  mAutoCreateToc = TRUE;
  mType = "plain";
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mStream) close();
}


//-----------------------------------------------------------------------------
const char* KMFolder::type(void) const
{
  return "plain";
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
const QString KMFolder::tocLocation(void) const
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
  assert(name() != NULL);

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  mStream = fopen(location(), "r+"); // messages file
  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    //#define TOC_DEBUG
#ifdef TOC_DEBUG
    createTocFromContents();
#else
    mTocStream = fopen(tocLocation(), "r+"); // index file
    if (!mTocStream) return createTocFromContents();
    readToc();
#endif
  }
  else
  {
    debug("No path specified for folder " + name() +
	  " -- Turning autoCreateToc off");
    mAutoCreateToc = FALSE;
    createTocFromContents();
  }

  mQuiet = 0;

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::create(void)
{
  assert(name() != NULL);
  assert(mOpenCount == 0);

  mStream = fopen(location(), "w");
  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    mTocStream = fopen(tocLocation(), "w");
    if (!mTocStream) return errno;
  }
  else
  {
    debug("folder " + name() +
	  " has no path specified -- turning autoCreateToc off");
    mAutoCreateToc = FALSE;
  }

  mOpenCount++;
  mQuiet = 0;

  return createTocHeader();
}


//-----------------------------------------------------------------------------
void KMFolder::close(bool aForced)
{
  int i;

  if (mTocDirty && mAutoCreateToc) writeToc();

  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;

  if (mStream) fclose(mStream);
  if (mTocStream) fclose(mTocStream);

  mOpenCount  = 0;
  mStream     = NULL;
  mTocStream  = NULL;
  mMsgs       = 0;
  mUnreadMsgs = 0;
  mActiveMsgs = 0;

  for (i=0; i<mMsgs; i++)
  {
    if (mMsgInfo[i].msg()) mMsgInfo[i].deleteMsg();
  }
}


//-----------------------------------------------------------------------------
int KMFolder::createTocFromContents(void)
{
  char line[MAX_LINE];
  char status[8], subjStr[128], dateStr[80], fromStr[80];
  char* msg;
  unsigned long offs, size, pos;
  int i, msgArrNum;
  bool atEof = FALSE;
  KMMsgInfo* mi;

  assert(name() != NULL);
  assert(path() != NULL);
  assert(mStream != NULL);

  rewind(mStream);

  debug("*** creating toc file %s\n", (const char*)tocLocation());

  offs = 0;
  size = 0;
  strcpy(status, "RO");

  msg = re_comp(MSG_SEPERATOR_REGEX);
  if (msg)
  {
    fatal("Error in kmfolder.cpp\nwhile parsing bultin\n"
	  "regular expression\n%s\n%s", MSG_SEPERATOR_REGEX, msg);
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

  if (mAutoCreateToc) writeToc();
  else mHeaderOffset = 0;

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::writeToc(void)
{
  int rc, i;
  int tocRecSize = KMMsgInfo::recSize();

  if (mTocStream) fclose(mTocStream);
  mTocStream = fopen(tocLocation(), "w");
  if (!mTocStream) return errno;

  rc = createTocHeader();
  if (rc) return rc;

  mHeaderOffset = ftell(mTocStream);

  for (i=0; i<mMsgs; i++)
  {
    fprintf(mTocStream, "%s\n", mMsgInfo[i].asString());
  }
  fflush(mTocStream);

  mTocDirty = FALSE;
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::setAutoCreateToc(bool autoToc)
{
  mAutoCreateToc = autoToc;
}


//-----------------------------------------------------------------------------
int KMFolder::createTocHeader(void)
{
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::readTocHeader(void)
{
}


//-----------------------------------------------------------------------------
int KMFolder::readHeader(void)
{
  if (mTocStream) return 0;
  assert(name() != NULL);
  assert(path() != NULL);

  mTocStream = fopen(tocLocation(), "r+");
  if (!mTocStream) return errno;

  readTocHeader();

  fclose(mTocStream);
  mTocStream = NULL;

  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::readToc(void)
{
  char line[MAX_LINE];
  int  msgArrNum;

  assert(mTocStream != NULL);
  rewind(mTocStream);

  mMsgs       = 0;
  mUnreadMsgs = 0;
  mActiveMsgs = 0;

  readTocHeader();
  mHeaderOffset = ftell(mTocStream);
  msgArrNum = mMsgInfo.size();

  for (; !feof(mTocStream); mMsgs++)
  {
    fgets(line, MAX_LINE, mTocStream);
    if (feof(mTocStream)) break;

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

  assert(mStream != NULL);
  len = aMsg->msgStr().length() - 2;

  if (len <= 0)
  {
    warning(nls->translate("KMFolder::addMsg():\nmessage contains no data !"));
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
  fwrite((char*)aMsg->msgStr().data(), len, 1, mStream);
  fflush(mStream);

  size = ftell(mStream) - offs;

  mMsgInfo[mMsgs].init(aMsg->status(), offs, size, aMsg);
  aMsg->setOwner(this);

  if (mAutoCreateToc)
  {
    debug("writing new toc entry to toc file");
    assert(mTocStream != NULL);
    fseek(mTocStream, 0, SEEK_END);
    fprintf(mTocStream, "%s\n", mMsgInfo[mMsgs].asString()); 
    fflush(mTocStream);
  }

  if (aIndex_ret) *aIndex_ret = mMsgs;
  if (!mQuiet) emit msgAdded(mMsgs);

  mMsgs++;

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

  unlink(tocLocation());
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

  if (mAutoCreateToc) truncate(tocLocation(), mHeaderOffset);
  else unlink(tocLocation());

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

  if (!mTocStream) return 0;

  for (i=0; i<mMsgs; i++)
  {
    mi = &mMsgInfo[i];
    if (mi->dirty())
    {
      fseek(mTocStream, offset, SEEK_SET);
      fprintf(mTocStream, "%s\n", mi->asString());
      rc = errno;
      if (rc) break;
      mi->setDirty(FALSE);
    }
    offset += recSize;
  }
  fflush(mTocStream);
  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::lock(bool/*sharedLock*/)
{
  assert(mStream != NULL);
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::unlock(void)
{
  assert(mStream != NULL);  
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

  mTocDirty = TRUE;
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
  return mMsgInfo[msgId-1].date();
}


//-----------------------------------------------------------------------------
KMMessage::Status KMFolder::msgStatus(int msgId) const
{
  assert(msgId>0);
  return mMsgInfo[msgId-1].status();
}


//-----------------------------------------------------------------------------
#include "kmfolder.moc"
