// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include "kmfolder.h"
#include "kmmessage.h"

#include <mimelib/mimepp.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <regex.h>


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
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mStream) close();
}


//-----------------------------------------------------------------------------
const QString& KMFolder::location(void) const
{
  static QString sLocation;

  sLocation = path();
  sLocation += "/";
  sLocation += name();

  return sLocation;
}


//-----------------------------------------------------------------------------
int KMFolder::open(void)
{
  QString p = path();

  assert(name() != NULL);

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  mStream = fopen(p+"/"+name(), "r+"); // messages file
  if (!mStream) return errno;

  if (!p.isEmpty())
  {
    mTocStream = fopen(p+"/."+name(), "r+"); // index file
    if (!mTocStream) return createToc();
    readToc();
  }
  else
  {
    debug("no path for folder "+name()+" -- Turning autoCreateToc off");
    mAutoCreateToc = FALSE;
    createToc();
  }

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::create(void)
{
  QString p = path();
  
  assert(name() != NULL);
  assert(mOpenCount == 0);

  mStream = fopen(p+"/"+name(), "w");
  if (!mStream) return errno;

  if (!p.isEmpty())
  {
    mTocStream = fopen(p+"/."+name(), "w");
    if (!mTocStream) return errno;
  }
  else
  {
    debug("no path for folder "+name()+" -- turning autoCreateToc off");
    mAutoCreateToc = FALSE;
  }

  mOpenCount++;

  return createTocHeader();
}


//-----------------------------------------------------------------------------
void KMFolder::close(bool aForce)
{
  int i;

  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForce) return;

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
int KMFolder::createToc(void)
{
  char line[MAX_LINE];
  char status[8];
  char* msg;
  unsigned long offs, size, pos;
  int i, msgArrNum;
  int rc;
  bool atEof = FALSE;

  assert(name() != NULL);
  assert(path() != NULL);
  assert(mStream != NULL);

  rewind(mStream);

  QString p=path()+"/."+name();
  printf("*** creating toc for %s\n", (const char*)p);

  if (mAutoCreateToc)
  {
    mTocStream = fopen(path()+"/."+name(), "w");
    if (!mTocStream) return errno;
  }
  else mTocStream = NULL; // just to be sure

  rc = createTocHeader();
  if (rc) return rc;

  offs = 0;
  size = 0;
  strcpy(status, "RO");
  mHeaderOffset = ftell(mTocStream);

  msg = re_comp(MSG_SEPERATOR_REGEX);
  if (msg)
  {
    fatal("Error parsing bultin (kmfolder.cpp)\nregular expression\n'" +
	  QString(MSG_SEPERATOR_REGEX) + "':\n" + msg);
    return -1;
  }

  msgArrNum = mMsgInfo.size();

  mMsgs = -1;
  offs  = 0;

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

	mMsgInfo[mMsgs].init(status, offs, size);
	strcpy(status, "RO");
	debug(mMsgInfo[mMsgs].asString());
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
  }

  // write TOC file
  if (mAutoCreateToc)
  {
    for (i=0; i<mMsgs; i++)
    {
      fprintf(mTocStream, "%s\n", mMsgInfo[i].asString());
    }
    fflush(mTocStream);
  }

  mUnreadMsgs = 0;
  mActiveMsgs = mMsgs;

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

  mTocStream = fopen(path()+"/."+name(), "r+");
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

  QString p=path()+"/."+name();
  printf("*** reading toc for %s\n", (const char*)p);

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
long KMFolder::status(long) 
{
  // return mail_status(mStream, MBOX(mStream), ops);
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::setMsgStatus(KMMessage* aMsg, KMMessage::Status aStatus)
{
  int i = indexOfMsg(aMsg);
  if (i < 0) 
  {
    debug("KMFolder::setMsgStatus() called for a message that is not in"
	  "this folder !");
    return;
  }

  mMsgInfo[i].setStatus(aStatus);
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
void KMFolder::detachMsg(int msgno)
{
  KMMsgInfo* mi;
  KMMessage* msg;

  assert(mStream != NULL);
  assert(msgno >= 1 && msgno <= mMsgs);

  mi = &mMsgInfo[msgno-1];

  msg = mi->msg();
  msg->setOwner(NULL);
  mi->setMsg(NULL);
  mi->setStatus(KMMessage::stDeleted);
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

  printf("Message #%d: contentType=%d\n", msgno, 
	 dwmsg->Headers().ContentType().Type());

  msg->takeMessage(dwmsg);

  DwBodyPart* part = dwmsg->Body().FirstBodyPart();
  while (part)
  {
    printf("part\n");
    part = part->Next();
  }

  printf("numBodyParts=%d\n", msg->numBodyParts());
}


//-----------------------------------------------------------------------------
int KMFolder::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  long offs, size, len;
  int rc, msgArrNum = mMsgInfo.size();

  assert(mStream != NULL);
  len = aMsg->msgStr().length() - 2;

  if (len <= 0)
  {
    warning("KMFolder::addMsg():\nmessage contains no data !");
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
int KMFolder::rename(const char*)
{
  return -1;
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

  unlink(path()+"/."+name());
  rc = unlink(path()+"/"+name());
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
void KMFolder::ping()
{
}


//-----------------------------------------------------------------------------
int KMFolder::expunge(void)
{
  int rc;
  QString p = path();
  
  assert(name() != NULL);

  close(TRUE);

  if (mAutoCreateToc) truncate(p+"/."+name(), mHeaderOffset);
  rc = truncate(p+"/"+name(), 0);
  if (rc) return rc;

  mMsgInfo.truncate(INIT_MSGS);
  mMsgs = 0;

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::lock(bool sharedLock)
{
  assert(mStream != NULL);
}


//-----------------------------------------------------------------------------
void KMFolder::unlock(void)
{
  assert(mStream != NULL);  
}


#include "kmfolder.moc"
