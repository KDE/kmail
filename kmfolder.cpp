// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include "kmfolder.h"
#include "kmmessage.h"

#include <mimelib/message.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <regex.h>


#define MAX_LINE 4096
#define MAX_MSGS 1000
//#define MSG_SEPERATOR "From ???@???"
#define MSG_SEPERATOR_START "From "
#define MSG_SEPERATOR_REGEX "^From.*..\\:..\\:...*$"

static short msgSepLen;


//-----------------------------------------------------------------------------
KMFolder :: KMFolder(KMFolderDir* aParent, const char* aName) : 
  KMFolderInherited(aParent, aName), mMsgInfo(MAX_MSGS)
{
  int i;

  mStream     = NULL;
  mTocStream  = NULL;
  msgSepLen   = strlen(MSG_SEPERATOR_START);
  mMsgList    = new KMMessage*[MAX_MSGS];
  mMsgs       = 0;
  mUnreadMsgs = 0;
  mOpenCount  = 0;
  mAutoCreateToc = FALSE;

  for (i=MAX_MSGS-1; i>=0; i--)
    mMsgList[i] = NULL;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mStream) close();
  if (mMsgList) delete mMsgList;
}


//-----------------------------------------------------------------------------
int KMFolder::open(void)
{
  assert(name() != NULL);

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  mStream = fopen(path()+"/"+name(), "r+"); // messages file
  if (!mStream) return errno;

  mTocStream = fopen(path()+"/."+name(), "r+"); // index file

  if (mTocStream) readToc();
  else return createToc();

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::create(void)
{
  assert(name() != NULL);
  assert(mOpenCount == 0);

  mStream = fopen(path()+"/"+name(), "w");
  if (!mStream) return errno;

  mTocStream = fopen(path()+"/."+name(), "w");
  if (!mTocStream) return errno;

  mOpenCount++;

  return createTocHeader();
}


//-----------------------------------------------------------------------------
void KMFolder::close(void)
{
  int i;

  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0) return;

  if (mStream) fclose(mStream);
  if (mTocStream) fclose(mTocStream);

  mStream     = NULL;
  mTocStream  = NULL;
  mMsgs       = 0;
  mUnreadMsgs = 0;
  mActiveMsgs = 0;

  for (i=0; i<MAX_MSGS; i++)
  {
    if (mMsgList[i]) delete mMsgList[i];
    mMsgList[i] = NULL;
  }
}


//-----------------------------------------------------------------------------
int KMFolder::createToc(void)
{
  char line[MAX_LINE];
  char status[8];
  char* msg;
  unsigned long offs, size, pos;
  int i, rc;

  assert(name() != NULL);
  assert(path() != NULL);
  assert(mStream != NULL);

  rewind(mStream);

  if (mAutoCreateToc)
  {
    mTocStream = fopen(path()+"/."+name(), "w");
    if (!mTocStream) return errno;
  }

  rc = createTocHeader();
  if (rc) return rc;

  offs = 0;
  size = 0;
  strcpy(status, "RO");

  msg = re_comp(MSG_SEPERATOR_REGEX);
  if (msg)
  {
    warning("Error parsing regular expression '" +
	    QString(MSG_SEPERATOR_REGEX) + "': " + msg);
    return -1;
  }

  for (mMsgs=-1, offs=0; !feof(mStream) && mMsgs<MAX_MSGS; )
  {
    pos = ftell(mStream);
    fgets(line, MAX_LINE, mStream);
    if (strncmp(line,MSG_SEPERATOR_START, msgSepLen)==0 && re_exec(line))
    {
      size = pos - offs;

      if (mMsgs>=0)
      {
	mMsgInfo[mMsgs].init(status, offs, size);
	strcpy(status, "RO");
      }

      offs = ftell(mStream);
      mMsgs++;
    }
    if (strncmp(line, "Status: ", 8) == 0)
    {
      for(i=0; i<8 && line[i+8] > ' '; i++)
	status[i] = line[i+8];
      status[i] = '\0';
    }
  }

  for (i=0; i<mMsgs; i++)
  {
    if (mAutoCreateToc) fprintf(mTocStream, "%s\n", mMsgInfo[i].asString());
    //printf("%s\n", mMsgInfo[i].asString());
  }
  if (mAutoCreateToc) fflush(mTocStream);

  mUnreadMsgs = 0;
  mActiveMsgs = mMsgs;

  if (mMsgs >= MAX_MSGS) 
  {
    // I know, this is not nice. But it was easier to implement
    // for now. Tell me (taferner@alpin.or.at) if you think that 
    // this restriction is problematic.

    printf("WARNING: too much messages in folder. All messages after %dth "
	   "message in this folder are ignored !\n", MAX_MSGS);
  }
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

  assert(mTocStream != NULL);
  rewind(mTocStream);

  mUnreadMsgs = 0;
  mActiveMsgs = 0;

  readTocHeader();

  for (mMsgs=0; !feof(mTocStream) && mMsgs<MAX_MSGS; mMsgs++)
  {
    fgets(line, MAX_LINE, mTocStream);
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
long KMFolder::status(long) 
{
  // return mail_status(mStream, MBOX(mStream), ops);
  return 0;
}


//-----------------------------------------------------------------------------
KMMessage *KMFolder::getMsg(int msgno)
{
  assert(mStream != NULL);
  assert(msgno >= 1 && msgno <= mMsgs);

  if (!mMsgList[msgno-1]) readMsg(msgno);
  return mMsgList[msgno-1];
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
  assert(mMsgList[msgno] == NULL);

  msg = new KMMessage(this);

  msgSize = mMsgInfo[msgno].size();
  msg->msgStr().resize(msgSize+2);

  fseek(mStream, mMsgInfo[msgno].offset(), SEEK_SET);

  fread((char*)msg->msgStr().data(), msgSize, 1, mStream);
  ((char*)msg->msgStr().data())[msgSize] = '\0';

  msg->setStatus(mMsgInfo[msgno].status());
  dwmsg = DwMessage::NewMessage(msg->msgStr(), 0);
  dwmsg->Parse();
  msg->takeMessage(dwmsg);

  mMsgList[msgno] = msg;
}


//-----------------------------------------------------------------------------
long KMFolder::addMsg(KMMessage* /*aMsg*/, int* aIndex_ret)
{
  *aIndex_ret = -1;
  return -1;
} 


//-----------------------------------------------------------------------------
int KMFolder::rename(const char*)
{
  return -1;
}


//-----------------------------------------------------------------------------
int KMFolder::remove(void)
{
  return -1;
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
void KMFolder::expunge()
{
}

