// kmmsgbase.cpp

#include "kmmsgbase.h"
#include <mimelib/mimepp.h>
#include <qregexp.h>
#ifndef KRN
#include <kmfolder.h>
#endif

#include <ctype.h>

#define NUM_STATUSLIST 9
static KMMsgStatus sStatusList[NUM_STATUSLIST] =
{
  KMMsgStatusDeleted, KMMsgStatusNew,
  KMMsgStatusUnread,  KMMsgStatusOld,
  KMMsgStatusRead,    KMMsgStatusReplied,
  KMMsgStatusSent,    KMMsgStatusQueued,
  KMMsgStatusUnknown /* "Unknown" must be at the *end* of the list */
};


//-----------------------------------------------------------------------------
KMMsgBase::KMMsgBase(KMFolder* aParent)
{
  mParent  = aParent;
  mDirty   = FALSE;
  mMsgSize = 0;
  mFolderOffset = 0;
  mStatus  = KMMsgStatusNew;
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
#ifndef KRN
  if (mParent) mParent->headerOfMsgChanged(this);
#endif
}


//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const char* aStatusStr, const char* aXStatusStr)
{
  int i;

  mStatus = KMMsgStatusUnknown;

  // first try to find status from "X-Status" field if given
  if (aXStatusStr) for (i=0; i<NUM_STATUSLIST-1; i++)
  {
    if (strchr(aXStatusStr, (char)sStatusList[i]))
    {
      mStatus = sStatusList[i];
      break;
    }
  }

  // if not successful then use the "Status" field
  if (mStatus == KMMsgStatusUnknown)
  {
    if (aStatusStr && 
        ((aStatusStr[0]=='R' && aStatusStr[1]=='O') ||
	 (aStatusStr[0]=='O' && aStatusStr[1]=='R')))
	mStatus=KMMsgStatusOld;
    else if (aStatusStr && aStatusStr[0]=='R') mStatus=KMMsgStatusRead;
    else if (aStatusStr && aStatusStr[0]=='D') mStatus=KMMsgStatusDeleted;
    else mStatus=KMMsgStatusNew;
  }

  mDirty = TRUE;
#ifndef KRN
  if (mParent) mParent->headerOfMsgChanged(this);
#endif
}


//-----------------------------------------------------------------------------
KMMsgStatus KMMsgBase::status(void) const
{
  return mStatus;
}


//-----------------------------------------------------------------------------
bool KMMsgBase::isUnread(void) const
{
  KMMsgStatus st = status();
  return (st==KMMsgStatusNew || st==KMMsgStatusUnread);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isNew(void) const
{
  KMMsgStatus st = status();
  return (st==KMMsgStatusNew);
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
  QString str(356); // Sven is blind.

  // don't forget to change indexStringLength() below !!
  str.sprintf("%c %-.9lu %-.9lu %-.9lu %-3.3s %-100.100s %-100.100s %-100.100s",
	      (char)status(), folderOffset(), msgSize(), (unsigned long)date(),
	      (const char*)xmark(),
	      (const char*)decodeQuotedPrintableString(subject()),
	      (const char*)decodeQuotedPrintableString(from()),
	      (const char*)decodeQuotedPrintableString(to())); //sven
  len = str.length();
  for (i=0; i<len; i++)
    if (str[i] < ' ' && str[i] >= 0)
      str[i] = ' ';

  return str;
}


//-----------------------------------------------------------------------------
int KMMsgBase::indexStringLength(void)
{
  //return 237;
  return 338; //sven (+ 100 chars to + one space, right?
}


//-----------------------------------------------------------------------------
int KMMsgBase::compareByIndex(const KMMsgBase* other) const
{
  return (mFolderOffset - other->mFolderOffset);
}


//-----------------------------------------------------------------------------
int KMMsgBase::compareBySubject(const KMMsgBase* other) const
{
  //const char *subjStr, *otherSubjStr;
  QString subjStr, otherSubjStr;
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

  if (!f || !fo) return ((!fo) - (!f));

  while (*f && *f<'A') f++;
  while (*fo && *fo<'A') fo++;

  return stricmp(f, fo);
}


//-----------------------------------------------------------------------------
const char* KMMsgBase::skipKeyword(const QString aStr, char sepChar,
				   bool* hasKeyword)
{
  int i, maxChars=3;
  const char *pos, *str = aStr.data();

  if (!str) return 0;

  while (*str==' ')
    str++;
  if (hasKeyword) *hasKeyword=FALSE;

  for (i=0,pos=str; *pos && i<maxChars; pos++,i++)
  {
    if (*pos < 'A' || *pos == sepChar) break;
  }

  if (i>1 && *pos == sepChar) // skip following spaces too
  {
    for (pos++; *pos==' '; pos++)
      ;
    if (hasKeyword) *hasKeyword=TRUE;
    return pos;
  }
  return str;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeRFC1522String(const QString aStr)
{
  static QString result;
  char *pos, *dest, *beg, *end, *mid;
  QString str;
  char encoding, ch;
  bool valid;
  const int maxLen=200;
  int i;

  if (aStr.find("=?") < 0) return aStr;

  result.truncate(aStr.length());
  for (pos=aStr.data(), dest=result.data(); *pos; pos++)
  {
    if (pos[0]!='=' || pos[1]!='?')
    {
      *dest++ = *pos;
      continue;
    }
    beg = pos+2;
    end = beg;
    valid = TRUE;
    // parse charset name
    for (i=2,pos+=2; i<maxLen && (*pos!='?'&&(ispunct(*pos)||isalnum(*pos))); i++)
      pos++;
    if (*pos!='?' || i<4 || i>=maxLen) valid = FALSE;
    else
    {
      // get encoding and check delimiting question marks
      encoding = toupper(pos[1]);
      if (pos[2]!='?' || (encoding!='Q' && encoding!='B'))
	valid = FALSE;
      pos+=3;
      i+=3;
    }
    if (valid)
    {
      mid = pos;
      // search for end of encoded part
      while (i<maxLen && *pos && !(*pos=='?' && *(pos+1)=='='))
      {
	i++;
	pos++;
      }
      end = pos+2;//end now points to the first char after the encoded string
      if (i>=maxLen || !*pos) valid = FALSE;
    }
    if (valid)
    {
      ch = *pos;
      *pos = '\0';
      str = QString(mid, (int)(mid - pos - 1));
      if (encoding == 'Q')
      {
	// decode quoted printable text
	for (i=str.length()-1; i>=0; i--)
	  if (str[i]=='_') str[i]=' ';
	str = decodeQuotedPrintable(str);
      }
      else
      {
	// decode base64 text
	str = decodeBase64(str);
      }
      *pos = ch;
      for (i=0; str[i]; i++)
	*dest++ = str[i];

      pos = end -1;
    }
    else
    {
      //result += "=?";
      //pos = beg -1; // because pos gets increased shortly afterwards
      pos = beg - 2;
      *dest++ = *pos++;
      *dest++ = *pos;
    }
  }
  *dest = '\0';
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeQuotedPrintableString(const QString aStr)
{
#ifdef BROKEN
  static QString result;
  int start, beg, mid, end;
  end = 0; // Remove compiler warning;

  start = 0;
  end = 0;
  result = "";

  while (1)
  {
    beg = aStr.find("=?", start);
    if (beg < 0)
    {
      // no more suspicious string parts found -- done
      result += aStr.mid(start, 32767);
      break;
    }

    if (beg > start) result += aStr.mid(start, beg-start);
    mid = aStr.find("?Q?", beg+2);
    if (mid>beg) end = aStr.find("?=", mid+3);
    if (mid < 0 || end < 0)
    {
      // no quoted printable part -- skip it
      result += "=?";
      start += 2;
      continue;
    }
    if (aStr[mid+3]=='_' )
    {
      result += ' ';
      mid++;
    }
    else if (aStr[mid+3]==' ') mid++;

    if (end-mid-3 > 0)
      result += decodeQuotedPrintable(aStr.mid(mid+3, end-mid-3).data());
    start = end+2;
  }
  return result;
#else
  return decodeRFC1522String(aStr);
#endif
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
  DwString dwsrc(aStr.data(), aStr.length());
  DwString dwdest;
  QString result;

  DwEncodeQuotedPrintable(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeBase64(const QString aStr)
{
  DwString dwsrc(aStr.data(), aStr.length());
  DwString dwdest;
  QString result;

  DwDecodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::encodeBase64(const QString aStr)
{
  DwString dwsrc(aStr.data(), aStr.size()-1);
  DwString dwdest;
  QString result;

  DwEncodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}
