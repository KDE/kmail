// kmmsginfo.cpp

#include "kmmsginfo.h"
#include "kmmessage.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <mimelib/datetime.h>

static QString result;

//-----------------------------------------------------------------------------
KMMsgInfo::KMMsgInfo(KMFolder* p): 
  KMMsgInfoInherited(p), mSubject(), mFrom(), mTo()
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
  mTo = other.mTo.copy();
  return *this;
}


//-----------------------------------------------------------------------------
KMMsgInfo& KMMsgInfo::operator=(const KMMessage& msg)
{
  KMMsgInfoInherited::assign(&msg);
  mSubject = msg.subject().copy();
  mFrom = msg.from().copy();
  mTo = msg.to().copy();
  return *this;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const QString aSubject, const QString aFrom,
                     const QString aTo, time_t aDate,
		     KMMsgStatus aStatus, const QString aXMark, 
		     unsigned long aFolderOffset, unsigned long aMsgSize)
{
  mSubject = decodeRFC1522String(aSubject).copy();
  mFrom    = decodeRFC1522String(aFrom).copy();
  mTo      = decodeRFC1522String(aTo).copy();
  mDate    = aDate;
  mXMark   = aXMark;
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
const QString KMMsgInfo::to(void) const
{
  return mTo;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::xmark(void) const
{
  return mXMark;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setSubject(const QString aSubject)
{
  mSubject = aSubject.copy();
  mDirty = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setFrom(const QString aFrom)
{
  mFrom = aFrom.copy();
  mDirty = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setXMark(const QString aXMark)
{
  mXMark = aXMark.copy();
  mDirty = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::fromIndexString(const QString str)
{
  char statusCh;
  unsigned long ldate;

  sscanf(str.data(), "%c %9lu %9lu %10lu", &statusCh, &mFolderOffset,
	 &mMsgSize, &ldate);

  mDate    = (time_t)ldate;
  mStatus  = (KMMsgStatus)statusCh;
  mXMark   = str.mid(33, 3).stripWhiteSpace();
  mSubject = str.mid(37, 100).stripWhiteSpace();
  mFrom    = str.mid(138, 100).stripWhiteSpace();
  mTo      = str.mid(239, 100).stripWhiteSpace();
  mDirty   = FALSE;
}
