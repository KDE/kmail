// kmmsginfo.cpp

#include "kmmsginfo.h"
#include "kmmessage.h"
#include "kmmsgpart.h" // for encode

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <mimelib/datetime.h>

class KMMsgInfo::KMMsgInfoPrivate
{
public:
    enum {
	SUBJECT_SET = 0x01, TO_SET = 0x02, REPLYTO_SET = 0x04, MSGID_SET=0x08,
	DATE_SET = 0x10, OFFSET_SET = 0x20, SIZE_SET = 0x40,
	XMARK_SET=0x100, FROM_SET=0x200,

	ALL_SET = 0xFFFF, NONE_SET = 0x0000
    };
    ushort modifiers;
    QString subject, from, to, replyToIdMD5, msgIdMD5, xmark;
    unsigned long folderOffset, msgSize;
    time_t date;

    KMMsgInfoPrivate() : modifiers(NONE_SET) { }
    KMMsgInfoPrivate& operator=(const KMMsgInfoPrivate& other) {
	modifiers = NONE_SET;
	if (other.modifiers & SUBJECT_SET) {
	    modifiers |= SUBJECT_SET;
	    subject = other.subject;
	}
	if (other.modifiers & FROM_SET) {
	    modifiers |= FROM_SET;
	    from = other.from;
	}
	if (other.modifiers & TO_SET) {
	    modifiers |= TO_SET;
	    to = other.to;
	}
	if (other.modifiers & REPLYTO_SET) {
	    modifiers |= REPLYTO_SET;
	    replyToIdMD5 = other.replyToIdMD5;
	}
	if(other.modifiers & MSGID_SET) {
	    modifiers |= MSGID_SET;
	    msgIdMD5 = other.msgIdMD5;
	}
	if(other.modifiers & XMARK_SET) {
	    modifiers |= XMARK_SET;
	    xmark = other.xmark;
	}
	if(other.modifiers & OFFSET_SET) {
	    modifiers |= OFFSET_SET;
	    folderOffset = other.folderOffset;
	}
	if(other.modifiers & SIZE_SET) {
	    modifiers |= SIZE_SET;
	    msgSize = other.msgSize;
	}
	if(other.modifiers & DATE_SET) {
	    modifiers |= DATE_SET;
	    date = other.date;
	}
	return *this;
    }
};

//-----------------------------------------------------------------------------
KMMsgInfo::KMMsgInfo(KMFolder* p, long off, short len) :
    KMMsgInfoInherited(p), mStatus(KMMsgStatusUnknown), kd(NULL)
{
    setIndexOffset(off);
    setIndexLength(len);
}


//-----------------------------------------------------------------------------
KMMsgInfo::~KMMsgInfo()
{
    delete kd;
}


//-----------------------------------------------------------------------------
KMMsgInfo& KMMsgInfo::operator=(const KMMsgInfo& other)
{
    KMMsgInfoInherited::assign(&other);
    if(other.kd) {
        if(!kd)
	    kd = new KMMsgInfoPrivate;
	*kd = *other.kd;
    } else {
	delete kd;
	kd = NULL;
    }
    mStatus = other.status();
    return *this;
}


//-----------------------------------------------------------------------------
KMMsgInfo& KMMsgInfo::operator=(const KMMessage& msg)
{
    KMMsgInfoInherited::assign(&msg);
    if(!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers = KMMsgInfoPrivate::ALL_SET;
    kd->subject = msg.subject();
    kd->from = msg.fromStrip();
    kd->to = msg.toStrip();
    kd->replyToIdMD5 = msg.replyToIdMD5();
    kd->msgIdMD5 = msg.msgIdMD5();
    kd->xmark = msg.xmark();
    mStatus = msg.status();
    kd->folderOffset = msg.folderOffset();
    kd->msgSize = msg.msgSize();
    kd->date = msg.date();
    return *this;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::init(const QString& aSubject, const QString& aFrom,
                     const QString& aTo, time_t aDate,
		     KMMsgStatus aStatus, const QString& aXMark,
		     const QString& replyToId, const QString& msgId,
		     unsigned long aFolderOffset, unsigned long aMsgSize)
{
    mIndexOffset = 0;
    mIndexLength = 0;
    if(!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers = KMMsgInfoPrivate::ALL_SET;
    kd->subject = decodeRFC2047String(aSubject);
    kd->from = KMMessage::stripEmailAddr( decodeRFC2047String(aFrom) );
    kd->to = KMMessage::stripEmailAddr( decodeRFC2047String(aTo) );
    kd->replyToIdMD5 = KMMessagePart::encodeBase64( replyToId );
    kd->msgIdMD5 = KMMessagePart::encodeBase64( msgId );
    kd->xmark = aXMark;
    kd->folderOffset = aFolderOffset;
    mStatus    = aStatus;
    kd->msgSize = aMsgSize;
    kd->date = aDate;
    mDirty     = FALSE;
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::subject(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::SUBJECT_SET)
	return kd->subject;
    return getStringPart(MsgSubjectPart);
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::fromStrip(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::FROM_SET)
	return kd->from;
    return getStringPart(MsgFromPart);
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::toStrip(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::TO_SET)
	return kd->to;
    return getStringPart(MsgToPart);
}

//-----------------------------------------------------------------------------
QString KMMsgInfo::xmark(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::XMARK_SET)
	return kd->xmark;
    return getStringPart(MsgXMarkPart);
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::replyToIdMD5(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::REPLYTO_SET)
	return kd->replyToIdMD5;
    return getStringPart(MsgReplyToIdMD5Part);
}


//-----------------------------------------------------------------------------
QString KMMsgInfo::msgIdMD5(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::MSGID_SET)
	return kd->msgIdMD5;
    return getStringPart(MsgIdMD5Part);
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setSubject(const QString& aSubject)
{
    if(aSubject == subject())
	return;

    if (!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::SUBJECT_SET;
    kd->subject = aSubject;
    mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setXMark(const QString& aXMark)
{
    if (aXMark == xmark())
	return;

    if (!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::XMARK_SET;
    kd->xmark = aXMark;
    mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setReplyToIdMD5(const QString& aReplyToIdMD5)
{
    if (aReplyToIdMD5 == replyToIdMD5())
	return;

    if (!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::REPLYTO_SET;
    kd->replyToIdMD5 = aReplyToIdMD5;
    mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgInfo::setMsgIdMD5(const QString& aMsgIdMD5)
{
    if (aMsgIdMD5 == msgIdMD5())
	return;

    if (!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::MSGID_SET;
    kd->msgIdMD5 = aMsgIdMD5;
    mDirty = TRUE;
}

//-----------------------------------------------------------------------------
KMMsgStatus KMMsgInfo::status(void) const
{
    if (mStatus == KMMsgStatusUnknown)
	((KMMsgInfo *)this)->mStatus = (KMMsgStatus)getLongPart(MsgStatusPart);
    return mStatus;
}

//-----------------------------------------------------------------------------
unsigned long KMMsgInfo::folderOffset(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::OFFSET_SET)
	return kd->folderOffset;
    return getLongPart(MsgOffsetPart);
}

//-----------------------------------------------------------------------------
unsigned long KMMsgInfo::msgSize(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::SIZE_SET)
	return kd->msgSize;
    return getLongPart(MsgSizePart);
}

//-----------------------------------------------------------------------------
time_t KMMsgInfo::date(void) const
{
    if (kd && kd->modifiers & KMMsgInfoPrivate::DATE_SET)
	return kd->date;
    return getLongPart(MsgDatePart);
}

//-----------------------------------------------------------------------------
void KMMsgInfo::setMsgSize(unsigned long sz)
{
    if (sz == msgSize())
	return;

    if(!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::SIZE_SET;
    kd->msgSize = sz;
    mDirty = TRUE;
}

//-----------------------------------------------------------------------------
void KMMsgInfo::setFolderOffset(unsigned long offs)
{
    if (folderOffset() == offs)
	return;

    if (!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::OFFSET_SET;
    kd->folderOffset = offs;
    mDirty = TRUE;
}

//-----------------------------------------------------------------------------
void KMMsgInfo::setStatus(const KMMsgStatus aStatus)
{
    if(aStatus == status())
	return;
    KMMsgBase::setStatus(aStatus); //base does more "stuff"
    mStatus = aStatus;
    mDirty = TRUE;
}

//-----------------------------------------------------------------------------
void KMMsgInfo::setDate(time_t aUnixTime)
{
    if(aUnixTime == date())
	return;

    if(!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers |= KMMsgInfoPrivate::DATE_SET;
    kd->date = aUnixTime;
    mDirty = TRUE;
}

//--- For compatability with old index files
void KMMsgInfo::compat_fromOldIndexString(const QCString& str, bool toUtf8)
{
    char *start, *offset;

    if(!kd)
	kd = new KMMsgInfoPrivate;
    kd->modifiers = KMMsgInfoPrivate::ALL_SET;
    kd->xmark   = str.mid(33, 3).stripWhiteSpace();
    kd->folderOffset = str.mid(2,9).toULong();
    kd->msgSize = str.mid(12,9).toULong();
    kd->date = (time_t)str.mid(22,10).toULong();
    mStatus = (KMMsgStatus)str.at(0);
    if (toUtf8) {
	kd->subject = str.mid(37, 100).stripWhiteSpace();
	kd->from = str.mid(138, 50).stripWhiteSpace();
	kd->to = str.mid(189, 50).stripWhiteSpace();
    } else {
	start = offset = str.data() + 37;
	while (*start == ' ' && start - offset < 100) start++;
	kd->subject = QString::fromUtf8(str.mid(start - str.data(),
            100 - (start - offset)), 100 - (start - offset));
	start = offset = str.data() + 138;
	while (*start == ' ' && start - offset < 50) start++;
	kd->from = QString::fromUtf8(str.mid(start - str.data(),
            50 - (start - offset)), 50 - (start - offset));
	start = offset = str.data() + 189;
	while (*start == ' ' && start - offset < 50) start++;
	kd->to = QString::fromUtf8(str.mid(start - str.data(),
            50 - (start - offset)), 50 - (start - offset));
    }
    kd->replyToIdMD5 = str.mid(240, 22).stripWhiteSpace();
    kd->msgIdMD5 = str.mid(263, 22).stripWhiteSpace();
    mDirty = FALSE;
}

bool KMMsgInfo::dirty(void) const
{
    if(KMMsgBase::dirty())
	return TRUE;
    return kd && kd->modifiers != KMMsgInfoPrivate::NONE_SET;
}
