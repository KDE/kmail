// kmmsgbase.cpp

#include <kdebug.h>

#include "kmmsgbase.h"
#include <mimelib/mimepp.h>
#include <qregexp.h>
#ifndef KRN
#include <kmfolder.h>
#endif

#include <ctype.h>
#include <stdlib.h>

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
void KMMsgBase::setStatus(const KMMsgStatus aStatus)
{
  if (mParent) mParent->msgStatusChanged( mStatus, aStatus );
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
void KMMsgBase::setDate(time_t aUnixTime)
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
  unsigned long dateTen = date();
//  dateTen %= 10000000000; // In index only 10 chars are reserved for the date
//  This is nonsense because 10000000000 is bigger than the highest unsigned
//  long. (Or is there any compiler that defines unsigned long as something
//  really huge?? )

  QString a((const char*)decodeQuotedPrintableString(subject()));
  a.truncate(100);
  QString b((const char*)decodeQuotedPrintableString(fromStrip()));
  b.truncate(50);
  QString c((const char*)decodeQuotedPrintableString(toStrip()));
  c.truncate(47);
  QString d((const char*)replyToIdMD5());
  d.truncate(22);
  QString e((const char*)msgIdMD5());
  e.truncate(22);

  // don't forget to change indexStringLength() below !!
  str.sprintf("%c %-.9lu %-.9lu %-.10lu %-3.3s ",
	      (char)status(), folderOffset(), msgSize(), dateTen,
	      (const char*)xmark() );
  if (str.length() != 37)
    kdDebug() << "Invalid length " << endl;
  str += a.rightJustify( 100, ' ' );
  str += " ";
  str += b.rightJustify( 50, ' ' );
  str += " ";
  str += c.rightJustify( 50, ' ' );
  str += " ";
  str += d.rightJustify( 22, ' ' );
  str += " ";
  str += e.rightJustify( 22, ' ' );

  len = str.length();
  for (i=0; i<len; i++)
    if (str[i] < ' ' && str[i] >= 0)
      str[i] = ' ';

  if (str.length() != 285) {
    kdDebug() << QString( "Error invalid index entry %1").arg(str.length()) << endl;
    kdDebug() << str << endl;
  }
  return str;
}


//-----------------------------------------------------------------------------
int KMMsgBase::indexStringLength(void)
{
  //return 237;
  //  return 338; //sven (+ 100 chars to + one space, right?
  //  return 339; //sanders (use 10 digits for the date we need this in 2001!)
  //  return 541; //sanders include Reply-To and Message-Id for threading
  return 285; // sanders strip from and to and use MD5 on Ids
}


//-----------------------------------------------------------------------------
QString KMMsgBase::skipKeyword(const QString& aStr, char sepChar,
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
const QString KMMsgBase::decodeRFC1522String(const QString& _str)
{
  QCString aStr = _str.ascii();
  QCString result;
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
      str = QString(mid).left((int)(mid - pos - 1));
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
      for (i=0; i < (int)str.length(); i++)
	*dest++ = (char)(QChar)str[i];

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
const char especials[17] = "()<>@,;:\"/[]?.= ";

const QString KMMsgBase::encodeRFC2047String(const QString& _str)
{
  if (_str.isEmpty()) return _str;
  char *latin = (char *)calloc(1, _str.length() + 1);
  strcpy(latin, _str.latin1());
  char *latinStart = latin, *l, *start, *stop;
  char hexcode;
  int numQuotes, i;
  QString result = QString();
  while (*latin)
  {
    l = latin;
    start = latin;
    while (*l)
    {
      if (*l == 32) start = l + 1;
      if (*l < 0) break;
      l++;
    }
    if (*l)
    {
      numQuotes = 1;
      while (*l)
      {
        /* The encoded word must be limited to 75 character */
        for (i = 0; i < 16; i++) if (*l == especials[i]) numQuotes++;
        if (*l < 0) numQuotes++;
        /* Stop after 58 = 75 - 17 characters or at "<user@host..." */
        if (l - start + 2 * numQuotes >= 58 || *l == 60) break;
        l++;
      }
      if (*l)
      {
        stop = l - 1;
        while (stop >= start && *stop != 32) stop--;
        if (stop <= start) stop = l;
      } else stop = l;
      while (latin < start) { result += *latin; latin++; }
      result += QString("=?iso-8859-1?q?");
      while (latin < stop)
      {
        numQuotes = 0;
        for (i = 0; i < 16; i++) if (*latin == especials[i]) numQuotes = 1;
        if (*latin < 0) numQuotes = 1;
        if (numQuotes)
        {
          result += "=";
          hexcode = ((*latin & 0xF0) >> 4) + 48;
          if (hexcode >= 58) hexcode += 7;
          result += hexcode;
          hexcode = (*latin & 0x0F) + 48;
          if (hexcode >= 58) hexcode += 7;
          result += hexcode;
        } else {
          result += *latin;
        }
        latin++;
      }
      result += "?=";
    } else {
      while (*latin) { result += *latin; latin++; }
    }
  }
  free(latinStart);
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeQuotedPrintableString(const QString& aStr)
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
const QString KMMsgBase::decodeQuotedPrintable(const QString& aStr)
{
  QString bStr = aStr;
  if (aStr.isNull())
    bStr = "";

  DwString dwsrc(bStr.data());
  DwString dwdest;

  DwDecodeQuotedPrintable(dwsrc, dwdest);
  return dwdest.c_str();
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::encodeQuotedPrintable(const QString& aStr)
{
  QString bStr = aStr;
  if (aStr.isNull())
    bStr = "";

  DwString dwsrc(bStr.data(), bStr.length());
  DwString dwdest;
  QString result;

  DwEncodeQuotedPrintable(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeBase64(const QString& aStr)
{
  QString bStr = aStr;
  if (aStr.isNull())
    bStr = "";
  while (bStr.length() < 16) bStr += "=";

  DwString dwsrc(bStr.data(), bStr.length());
  DwString dwdest;
  QString result;

  DwDecodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::encodeBase64(const QString& aStr)
{
  QString bStr = aStr;
  if (aStr.isNull())
    bStr = "";

  DwString dwsrc(bStr.data(), bStr.length());
  DwString dwdest;
  QString result;

  DwEncodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}
