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
  mXMark = other.xmark();
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
  mXMark = msg.xmark();
  return *this;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const QString& aSubject, const QString& aFrom,
                     const QString& aTo, time_t aDate,
		     KMMsgStatus aStatus, const QString& aXMark,
		     const QString& replyToId, const QString& msgId,
		     unsigned long aFolderOffset, unsigned long aMsgSize)
{
  mSubject   = decodeRFC2047String(aSubject).copy();
  mFromStrip = KMMessage::stripEmailAddr( decodeRFC2047String(aFrom) );
  mToStrip   = KMMessage::stripEmailAddr( decodeRFC2047String(aTo) );
  mDate      = aDate;
  mXMark     = aXMark;
  //  mReplyToIdMD5 =  KMMessagePart::encodeBase64( decodeRFC2047String(replyToId) );
  mReplyToIdMD5 =  KMMessagePart::encodeBase64( replyToId );
  //  mMsgIdMD5  = KMMessagePart::encodeBase64( decodeRFC2047String(msgId) );
  mMsgIdMD5  = KMMessagePart::encodeBase64( msgId );
  mStatus    = aStatus;
  mMsgSize   = aMsgSize;
  mFolderOffset = aFolderOffset;
  mDirty     = FALSE;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::subject(void) const
{
  return mSubject;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::fromStrip(void) const
{
  return mFromStrip;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::toStrip(void) const
{
  return mToStrip;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::xmark(void) const
{
  return mXMark;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::replyToIdMD5(void) const
{
  return mReplyToIdMD5;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::msgIdMD5(void) const
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
void KMMsgInfo::fromIndexString(const QCString& str, bool toUtf8)
{
  char *start, *offset;

  mXMark   = str.mid(33, 3).stripWhiteSpace();

  mFolderOffset = str.mid(2,9).toULong();
  mMsgSize = str.mid(12,9).toULong();
  mDate = (time_t)str.mid(22,10).toULong();
  mStatus = (KMMsgStatus)str.at(0);
  if (toUtf8)
  {
    mSubject = str.mid(37, 100).stripWhiteSpace();
    mFromStrip = str.mid(138, 50).stripWhiteSpace();
    mToStrip = str.mid(189, 50).stripWhiteSpace();
  } else {
    start = offset = str.data() + 37;
    while (*start == ' ' && start - offset < 100) start++;
    mSubject = QString::fromUtf8(str.mid(start - str.data(),
      100 - (start - offset)), 100 - (start - offset));
    start = offset = str.data() + 138;
    while (*start == ' ' && start - offset < 50) start++;
    mFromStrip = QString::fromUtf8(str.mid(start - str.data(),
      50 - (start - offset)), 50 - (start - offset));
    start = offset = str.data() + 189;
    while (*start == ' ' && start - offset < 50) start++;
    mToStrip = QString::fromUtf8(str.mid(start - str.data(),
      50 - (start - offset)), 50 - (start - offset));
  }
  mReplyToIdMD5 = str.mid(240, 22).stripWhiteSpace();
  mMsgIdMD5 = str.mid(263, 22).stripWhiteSpace();
  mDirty = FALSE;
}
