// kmmsginfo.cpp

#include "kmmsginfo.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>


//-----------------------------------------------------------------------------
void KMMsgInfo::init(KMMessage::Status aStatus, unsigned long offset, 
		     unsigned long size)
{
  mStatus = aStatus;
  mOffset = offset;
  mSize   = size;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const char* aStatusStr, unsigned long aOffset, 
		     unsigned long aSize)
{
  static KMMessage::Status stList[] = 
  {
    KMMessage::stDeleted, KMMessage::stNew, 
    KMMessage::stUnread, KMMessage::stOld, 
    KMMessage::stUnknown /*...must be at the end of this list! */
  };
  int i;

  for (i=0; stList[i]!=KMMessage::stUnknown; i++)
    if (strchr(aStatusStr, (char)stList[i])) break;

  init(stList[i], aOffset, aSize);
}


//-----------------------------------------------------------------------------
void KMMsgInfo::fromString(const char* aStr)
{
  char st;

  assert(aStr != NULL);

  sscanf(aStr,"%c %lu %lu", &st, &mOffset, &mSize);
  mStatus = (KMMessage::Status)st;
}


//-----------------------------------------------------------------------------
const char* KMMsgInfo::asString(void) const
{
  static char str[80];
  sprintf(str, "%c %-.8lu %-.8lu", (char)mStatus, mOffset, mSize);
  return str;
}
