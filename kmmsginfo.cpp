// kmmsginfo.cpp

#include "kmmsginfo.h"
#include "kmmessage.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <mimelib/datetime.h>


//-----------------------------------------------------------------------------
KMMsgInfo::KMMsgInfo(KMFolder* p): 
  KMMsgInfoInherited(p), mSubject(), mFrom()
{
}


//-----------------------------------------------------------------------------
KMMsgInfo::~KMMsgInfo()
{
}


//-----------------------------------------------------------------------------
KMMsgInfo& KMMsgInfo::operator=(const KMMsgInfo& other)
{
  KMMsgInfoInherited::assign(&other);
  mSubject = other.mSubject.copy();
  mFrom = other.mFrom.copy();
  return *this;
}


//-----------------------------------------------------------------------------
KMMsgInfo& KMMsgInfo::operator=(const KMMessage& msg)
{
  KMMsgInfoInherited::assign(&msg);
  mSubject = msg.subject().copy();
  mFrom = msg.from().copy();
  return *this;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const QString aSubject, const QString aFrom, time_t aDate,
		     KMMsgStatus aStatus, unsigned long aFolderOffset, 
		     unsigned long aMsgSize)
{
  mSubject = aSubject.copy();
  mFrom    = aFrom.copy();
  mDate    = aDate;
  mStatus  = aStatus;
  mMsgSize = aMsgSize;
  mFolderOffset = aFolderOffset;
  mDirty   = FALSE;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::subject(void) const
{
  return mSubject;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::from(void) const
{
  return mFrom;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setSubject(const QString aSubject)
{
  mSubject = aSubject.copy();
  mDirty   = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setFrom(const QString aFrom)
{
  mFrom = aFrom.copy();
  mDirty   = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::fromIndexString(const QString str)
{
  char statusCh;
  unsigned long ldate;

  sscanf(str.data(), "%c %9lu %9lu %9lu", &statusCh, &mFolderOffset,
	 &mMsgSize, &ldate);

  mDate    = (time_t)ldate;
  mStatus  = (KMMsgStatus)statusCh;
  mFrom    = str.mid(32, 100).stripWhiteSpace();
  mSubject = str.mid(133, 100).stripWhiteSpace();
  mDirty   = FALSE;
}
