// kmmsginfo.cpp

#include "kmmsginfo.h"
#include "kmmessage.h"
#include "kmmsgpart.h" // for encode

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <mimelib/datetime.h>

// Unused, right ? (David)
//static QString result;

//-----------------------------------------------------------------------------
KMMsgInfo::KMMsgInfo(KMFolder* p): 
  KMMsgInfoInherited(p), mSubject(), mFromStrip(), mToStrip()
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
  mFromStrip = other.mFromStrip.copy();
  mToStrip = other.mToStrip.copy();
  mReplyToIdMD5 = other.replyToIdMD5().copy();
  mMsgIdMD5 = other.msgIdMD5().copy();
  return *this;
}


//-----------------------------------------------------------------------------
KMMsgInfo& KMMsgInfo::operator=(const KMMessage& msg)
{
  KMMsgInfoInherited::assign(&msg);
  mSubject = msg.subject().copy();
  mFromStrip = msg.fromStrip().copy();
  mToStrip = msg.toStrip().copy();
  mReplyToIdMD5 = msg.replyToIdMD5().copy();
  mMsgIdMD5 = msg.msgIdMD5().copy();
  return *this;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const QString& aSubject, const QString& aFrom,
                     const QString& aTo, time_t aDate,
		     KMMsgStatus aStatus, const QString& aXMark, 
		     const QString& replyToId, const QString& msgId,
		     unsigned long aFolderOffset, unsigned long aMsgSize)
{
  mSubject   = decodeRFC1522String(aSubject).copy();
  mFromStrip = KMMessage::stripEmailAddr( decodeRFC1522String(aFrom) );
  mToStrip   = KMMessage::stripEmailAddr( decodeRFC1522String(aTo) );
  mDate      = aDate;
  mXMark     = aXMark;
  //  mReplyToIdMD5 =  KMMessagePart::encodeBase64( decodeRFC1522String(replyToId) );
  mReplyToIdMD5 =  KMMessagePart::encodeBase64( replyToId );
  //  mMsgIdMD5  = KMMessagePart::encodeBase64( decodeRFC1522String(msgId) );
  mMsgIdMD5  = KMMessagePart::encodeBase64( msgId );
  mStatus    = aStatus;
  mMsgSize   = aMsgSize;
  mFolderOffset = aFolderOffset;
  mDirty     = FALSE;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::subject(void) const
{
  return mSubject;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::fromStrip(void) const
{
  return mFromStrip;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::toStrip(void) const
{
  return mToStrip;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::xmark(void) const
{
  return mXMark;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::replyToIdMD5(void) const
{
  return mReplyToIdMD5;
}


//-----------------------------------------------------------------------------
const QString KMMsgInfo::msgIdMD5(void) const
{
  return mMsgIdMD5;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setSubject(const QString& aSubject)
{
  mSubject = aSubject.copy();
  mDirty = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setXMark(const QString& aXMark)
{
  mXMark = aXMark.copy();
  mDirty = TRUE;  
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setReplyToIdMD5(const QString& aReplyToIdMD5)
{
  mReplyToIdMD5 = aReplyToIdMD5.copy();
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setMsgIdMD5(const QString& aMsgIdMD5)
{
  mMsgIdMD5 = aMsgIdMD5.copy();
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::fromIndexString(const QString& str)
{
  char statusCh;
  unsigned long ldate;

  sscanf(str.data(), "%c %9lu %9lu %10lu", &statusCh, 
	 &mFolderOffset, &mMsgSize, &ldate);

  mDate    = (time_t)ldate;
  mStatus  = (KMMsgStatus)statusCh;
  mXMark   = str.mid(33, 3).stripWhiteSpace();
  mSubject = str.mid(37, 100).stripWhiteSpace();
  mFromStrip = str.mid(138, 50).stripWhiteSpace();
  mToStrip = str.mid(189, 50).stripWhiteSpace();
  mReplyToIdMD5 = str.mid(240, 22).stripWhiteSpace();
  mMsgIdMD5 = str.mid(263, 22).stripWhiteSpace();
  mDirty = FALSE;
}
