// kmmsginfo.cpp

#include "kmmsginfo.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <mimelib/datetime.h>


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
    setDate(mMsg->date());
    setFrom(mMsg->from());
  }
  else
  {
    mSubject[0] = '\0';
    mFrom[0]    = '\0';
    mDate       = 0;
  }

  mDirty = FALSE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const char* aStatusStr, unsigned long aOffset, 
		     unsigned long aSize, KMMessage* aMsg)
{
  init(KMMessage::stUnknown, aOffset, aSize, aMsg);
  setStatus(aStatusStr);
  mDirty = FALSE;
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
int KMMsgInfo::compareBySubject(const KMMsgInfo* other) const
{
  int rc;
  const char *skip, *otherSkip;
  bool hasRe, otherHasRe;

  skip = skipKeyword(mSubject, ':');
  otherSkip = skipKeyword(other->mSubject, ':');
  rc = strcasecmp(skip, otherSkip);
  if (rc) return rc;

  // If both are equal return the one with a "Re:" at the beginning as the
  // larger one.
  hasRe = ((const char*)mSubject != skip);
  otherHasRe = (((const char*)other->mSubject) != otherSkip);
  return (hasRe - otherHasRe);
}


//-----------------------------------------------------------------------------
int KMMsgInfo::compareByDate(const KMMsgInfo* other) const
{
  return (mDate - other->mDate);
}


//-----------------------------------------------------------------------------
int KMMsgInfo::compareByFrom(const KMMsgInfo* other) const
{
  return (strcasecmp(skipKeyword(mFrom, '"'), 
		     skipKeyword(other->mFrom, '"')));
}


//-----------------------------------------------------------------------------
const char* KMMsgInfo::skipKeyword(const char* aStr, char aKeyWord) const
{
  const char* str = aStr;
  int maxChars=4;

  while(*str >= 'A' && maxChars > 0)
  {
    str++;
    maxChars--;
  }

  if (*str != aKeyWord) return aStr;

  do
  {
    str++;
  }
  while(*str <= ' ' && *str != '\0');

  return str;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::stripBlanks(char* str, int pos) const
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
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setFrom(const char* aFrom)
{
  strncpy(mFrom, aFrom, 59);
  stripBlanks(mFrom, 58);
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
void KMMsgInfo::setDate(const time_t aUnixTime)
{
  mDate  = aUnixTime;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setDate(const char* aDateStr)
{
  DwDateTime dwDate;

  dwDate.FromString(aDateStr);
  dwDate.Parse();
  mDate  = dwDate.AsUnixTime();
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMsgInfo::dateStr(void) const
{
  return ctime(&mDate);
}


//-----------------------------------------------------------------------------
void KMMsgInfo::fromString(const char* aStr)
{
  char st;
  unsigned long idate;

  assert(aStr != NULL);

  sscanf(aStr,"%c %lu %lu %lu  ", &st, &mOffset, &mSize, &idate);

  setFrom(aStr+30);
  setSubject(aStr+90);
  mDate = (time_t)idate;

  mStatus = (KMMessage::Status)st;
  mMsg = NULL;
}


//-----------------------------------------------------------------------------
const char* KMMsgInfo::asString(void) const
{
  const char *fromstr, *subjstr;
  static char str[240];
  time_t sdate;
  int i;

  if (mMsg)
  {
    fromstr = mMsg->from();
    subjstr = mMsg->subject();
    sdate   = mMsg->date();
  }
  else
  {
    fromstr = mFrom;
    subjstr = mSubject;
    sdate   = mDate;
  }

  // skip leading blanks
  while (*fromstr==' ') fromstr++;
  while (*subjstr==' ') subjstr++;

  for(i=220; i>=0; i--)
    str[i] = ' ';

   sprintf(str, "%c %-.8lu %-.8lu %-.8lu  ",
	   (char)mStatus, mOffset, mSize, (long int)sdate);

  strncpy(str+30, fromstr, 60);
  strncpy(str+90, subjstr, 80);
 
  for(i=0; i<220; i++)
    if (str[i] < ' ') str[i] = ' ';
  str[i] = '\0';

  return str;
}
