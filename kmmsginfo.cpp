// kmmsginfo.cpp

#include "kmmsginfo.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>


//-----------------------------------------------------------------------------
KMMsgInfo::~KMMsgInfo()
{
  if (mMsg) delete mMsg;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(KMMessage::Status aStatus, unsigned long aOffset, 
		     unsigned long aSize, KMMessage* aMsg)
{
  mStatus = aStatus;
  mOffset = aOffset;
  mSize   = aSize;
  mMsg    = aMsg;

  if (mMsg)
  {
    setSubject(mMsg->subject());
    setDate(mMsg->dateStr());
    setFrom(mMsg->from());
  }
  else
  {
    mSubject[0] = '\0';
    mDate[0]    = '\0';
    mFrom[0]    = '\0';
  }
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const char* aStatusStr, unsigned long aOffset, 
		     unsigned long aSize, KMMessage* aMsg)
{
  init(KMMessage::stUnknown, aOffset, aSize, aMsg);
  setStatus(aStatusStr);
}


//-----------------------------------------------------------------------------
void KMMsgInfo::deleteMsg(void)
{
  if (mMsg)
  {
    delete mMsg;
    mMsg = NULL;
  }
}


//-----------------------------------------------------------------------------
void KMMsgInfo::stripBlanks(char* str, int pos)
{
  while (str[pos] <= ' ' && pos >= 0)
    pos--;

  str[pos+1] = '\0';
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setSubject(const char* aSubject)
{
  strncpy(mSubject, aSubject, 79);
  stripBlanks(mSubject, 78);
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setFrom(const char* aFrom)
{
  strncpy(mFrom, aFrom, 59);
  stripBlanks(mFrom, 58);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setDate(const char* aDate)
{
  strncpy(mDate, aDate, 59);
  stripBlanks(mDate, 58);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setStatus(KMMessage::Status aStatus)
{
  mStatus = aStatus;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setStatus(const char* aStatusStr)
{
  static KMMessage::Status stList[] = 
  {
    KMMessage::stDeleted, KMMessage::stNew, 
    KMMessage::stUnread, KMMessage::stOld, 
    KMMessage::stUnknown   /*...must be at the end of this list! */
  };
  int i;

  for (i=0; stList[i]!=KMMessage::stUnknown; i++)
    if (strchr(aStatusStr, (char)stList[i])) break;

  mStatus = stList[i];
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::fromString(const char* aStr)
{
  char st;

  assert(aStr != NULL);

  sscanf(aStr,"%c %lu %lu", &st, &mOffset, &mSize);

  setFrom(aStr+20);
  setSubject(aStr+80);
  setDate(aStr+160);

  mStatus = (KMMessage::Status)st;
  mMsg = NULL;
}


//-----------------------------------------------------------------------------
const char* KMMsgInfo::asString(void) const
{
  static char str[256];
  const char *fromStr, *subjStr, *dateStr;
  int i;

  if (mMsg)
  {
    fromStr = mMsg->from();
    subjStr = mMsg->subject();
    dateStr = mMsg->dateStr();
  }
  else
  {
    fromStr = mFrom;
    subjStr = mSubject;
    dateStr = mDate;
  }

  // skip leading blanks
  while (*fromStr==' ') fromStr++;
  while (*subjStr==' ') subjStr++;
  while (*dateStr==' ') dateStr++;

  for(i=220; i>=0; i--)
    str[i] = ' ';

  sprintf(str, "%c %-.8lu %-.8lu ", (char)mStatus, mOffset, mSize);

  strncpy(str+20,  fromStr, 60);
  strncpy(str+80,  subjStr, 80);
  strncpy(str+160, dateStr, 60);

  for(i=0; i<220; i++)
    if (str[i] < ' ') str[i] = ' ';
  str[i] = '\0';

  return str;
}
