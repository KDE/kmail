// kmmsgbase.cpp

#include "kmmsgbase.h"
#include "kmfolder.h"
#include <mimelib/mimepp.h>
#include <qregexp.h>

#define NUM_STATUSLIST 7
static KMMsgStatus sStatusList[NUM_STATUSLIST+1] = 
{
  KMMsgStatusDeleted, KMMsgStatusNew, 
  KMMsgStatusUnread, KMMsgStatusOld,
  KMMsgStatusReplied, KMMsgStatusSent,
  KMMsgStatusQueued,
  KMMsgStatusUnknown /* "Unknown" must be at the *end* of the list */
};


//-----------------------------------------------------------------------------
KMMsgBase::KMMsgBase(KMFolder* aParent)
{
  mParent  = aParent;
  mDirty   = FALSE;
  mMsgSize = 0;
  mFolderOffset = 0;
  mStatus  = KMMsgStatusUnknown;
  mDate    = 0;
}


//-----------------------------------------------------------------------------
KMMsgBase::~KMMsgBase()
{
}


//-----------------------------------------------------------------------------
void KMMsgBase::assign(const KMMsgBase* other)
{
  mParent = other->mParent;
  mDirty  = other->mDirty;
  mMsgSize = other->mMsgSize;
  mFolderOffset = other->mFolderOffset;
  mStatus = other->mStatus;
  mDate = other->mDate;
}


//-----------------------------------------------------------------------------
KMMsgBase& KMMsgBase::operator=(const KMMsgBase& other)
{
  assign(&other);
  return *this;
}


//-----------------------------------------------------------------------------
bool KMMsgBase::isMessage(void) const
{
  return FALSE;
}


//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(KMMsgStatus aStatus)
{
  mStatus = aStatus;
  mDirty = TRUE;
  if (mParent) mParent->headerOfMsgChanged(this);
}


//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const char* aStatusStr)
{
  int i;

  for (i=0; i<NUM_STATUSLIST-1; i++)
    if (strchr(aStatusStr, (char)sStatusList[i])) break;

  mStatus = sStatusList[i];
  mDirty = TRUE;
  if (mParent) mParent->headerOfMsgChanged(this);
}


//-----------------------------------------------------------------------------
KMMsgStatus KMMsgBase::status(void) const
{
  return mStatus;
}


//-----------------------------------------------------------------------------
const char* KMMsgBase::statusToStr(KMMsgStatus aStatus)
{
  static char sstr[2];

  sstr[0] = (char)aStatus;
  sstr[1] = '\0';

  return sstr;
}


//-----------------------------------------------------------------------------
void KMMsgBase::setDate(const time_t aUnixTime)
{
  mDate  = aUnixTime;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgBase::setDate(const char* aDateStr)
{
  DwDateTime dwDate;

  dwDate.FromString(aDateStr);
  dwDate.Parse();
  mDate  = dwDate.AsUnixTime();
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
time_t KMMsgBase::date(void) const
{
  return mDate;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::dateStr(void) const
{
  return ctime(&mDate);
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::asIndexString(void) const
{
  int i, len;
  QString str;

  str.sprintf("%c %-.9lu %-.9lu %-.9lu %-100s %-100s\n",
	      (char)status(), folderOffset(), msgSize(), (unsigned long)date(),
	      (const char*)decodeQuotedPrintableString(from()),
	      (const char*)decodeQuotedPrintableString(subject()));
  len = str.length();
  for (i=0; i<len; i++)
    if (str[i] < ' ') str[i] = ' ';

  return str;
}


//-----------------------------------------------------------------------------
int KMMsgBase::indexStringLength(void)
{
  return 234;
}


//-----------------------------------------------------------------------------
int KMMsgBase::compareBySubject(const KMMsgBase* other) const
{
  const char *subjStr, *otherSubjStr;
  bool hasKeywd, otherHasKeywd;
  int rc;

  subjStr = skipKeyword(subject(), ':', &hasKeywd);
  otherSubjStr = skipKeyword(other->subject(), ':', &otherHasKeywd);

  rc = stricmp(subjStr, otherSubjStr);
  if (rc) return rc;

  // If both are equal return the one with a keyword (Re: / Fwd: /...)
  // at the beginning as the larger one.
  return (hasKeywd - otherHasKeywd);
}


//-----------------------------------------------------------------------------
int KMMsgBase::compareByStatus(const KMMsgBase* other) const
{
  KMMsgStatus stat;
  int i;

  for (i=NUM_STATUSLIST-1; i>0; i--)
  {
    stat = sStatusList[i];
    if (mStatus==stat || other->mStatus==stat) break;
  }

  return ((mStatus==stat) - (other->mStatus==stat));
}


//-----------------------------------------------------------------------------
int KMMsgBase::compareByDate(const KMMsgBase* other) const
{
  return (mDate - other->mDate);
}


//-----------------------------------------------------------------------------
int KMMsgBase::compareByFrom(const KMMsgBase* other) const
{
  const char *f, *fo;

  f = from();
  fo = other->from();

  while (*f && *f<'A') f++;
  while (*fo && *fo<'A') fo++;

  return stricmp(f, fo);
}


//-----------------------------------------------------------------------------
const char* KMMsgBase::skipKeyword(const QString aStr, char sepChar,
				   bool* hasKeyword)
{
  int i, maxChars=4;
  const char *pos, *str;

  for (str=aStr.data(); *str && *str==' '; str++)
    ;
  if (hasKeyword) *hasKeyword=FALSE;

  for (i=0,pos=str; *pos && i<maxChars; pos++,i++)
  {
    if (*pos < 'A' || *pos == sepChar) break;
  }

  if (*pos == sepChar) // skip following spaces too
  {
    for (pos++; *pos && *pos==' '; pos++)
      ;
    if (hasKeyword) *hasKeyword=TRUE;
    return pos;
  }
  return str;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeQuotedPrintableString(const QString aStr)
{
  //static QRegExp qpExp("=\\?[^? ]*\\?Q\?[^? ]*\\?=");
  static QRegExp qpExp("=\\?[^\\ ]*\\?=");
  int pos, end, start;
  QString result, str;

  str = aStr.copy();
  pos = str.find(qpExp);
  if (pos < 0)
  {
    return str;
  }

  while (pos >= 0)
  {
    pos = str.find("?Q", pos+3);
    end = str.find("?=", pos+3);
    if (str.mid(pos+2,2)=="?_") pos += 2;
    result += decodeQuotedPrintable(str.mid(pos+2, end-pos-2));
    start = end+2;
    pos = str.find(qpExp, start);
  }
  result += str.mid(start, 32767);

  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeQuotedPrintable(const QString aStr)
{
  DwString dwsrc(aStr.data());
  DwString dwdest;

  DwDecodeQuotedPrintable(dwsrc, dwdest);
  return dwdest.c_str();
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::encodeQuotedPrintable(const QString aStr)
{
  DwString dwsrc(aStr.data(), aStr.size(), 0, aStr.length());
  DwString dwdest;
  QString result;

  DwEncodeQuotedPrintable(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeBase64(const QString aStr)
{
  DwString dwsrc(aStr.data(), aStr.size(), 0, aStr.length());
  DwString dwdest;
  QString result;

  DwDecodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::encodeBase64(const QString aStr)
{
  DwString dwsrc(aStr.data(), aStr.size(), 0, aStr.length());
  DwString dwdest;
  QString result;

  DwEncodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}
