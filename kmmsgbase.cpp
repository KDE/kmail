// kmmsgbase.cpp

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include "kmmsgbase.h"
#include <mimelib/mimepp.h>
#include <qtextcodec.h>
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
  if (mStatus == aStatus) return;
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
  if (mDate == aUnixTime) return;
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
const QCString KMMsgBase::asIndexString(void) const
{
  int i, len;
  QCString str; 
  unsigned long dateTen = date();
//  dateTen %= 10000000000; // In index only 10 chars are reserved for the date
//  This is nonsense because 10000000000 is bigger than the highest unsigned
//  long. (Or is there any compiler that defines unsigned long as something
//  really huge?? )

  QCString a(subject().utf8());
  a.truncate(100);
  QCString b(fromStrip().utf8());
  b.truncate(50);
  QCString c(toStrip().utf8());
  c.truncate(47);
  QCString d((const char*)replyToIdMD5());
  d.truncate(22);
  QCString e((const char*)msgIdMD5());
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
QTextCodec* KMMsgBase::codecForName(const QString& _str)
{
  if (_str.isEmpty()) return NULL;
  if (_str.lower() == "shift_jis" || _str.lower() == "shift-jis")
    return QTextCodec::codecForName("sjis");
  return QTextCodec::codecForName(_str.lower().replace(
    QRegExp("windows"), "cp") );
}


//-----------------------------------------------------------------------------
const QCString KMMsgBase::toUsAscii(const QString& _str)
{
  QString result = _str.copy();
  int len = result.length();
  for (int i = 0; i < len; i++)
    if (result.at(i).unicode() >= 128) result.at(i) = '?';
  return result.latin1();
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeRFC2047String(const QString& _str)
{
  QCString aStr = _str.ascii();
  QString result;
  QCString charset;
  char *pos, *beg, *end, *mid;
  QCString cstr;
  QString str;
  char encoding, ch;
  bool valid;
  const int maxLen=200;
  int i;

  if (aStr.find("=?") < 0) return QString::fromLocal8Bit(aStr);

  for (pos=aStr.data(); *pos; pos++)
  {
    if (pos[0]!='=' || pos[1]!='?')
    {
      result += *pos;
      continue;
    }
    beg = pos+2;
    end = beg;
    valid = TRUE;
    // parse charset name
    charset = "";
    for (i=2,pos+=2; i<maxLen && (*pos!='?'&&(*pos==' '||ispunct(*pos)||isalnum(*pos))); i++)
    {
      charset += *pos;
      pos++;
    }
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
	cstr = decodeQuotedPrintable(str);
      }
      else
      {
	// decode base64 text
	cstr = decodeBase64(str);
      }
      QTextCodec *codec = codecForName(charset);
      if (!codec) codec = codecForName(KGlobal::locale()->charset());
      if (codec) str = codec->toUnicode(cstr);
      else str = QString::fromLocal8Bit(cstr);

      // Workaround for bug in QT-2.2.2
      // the utf-8 QTextCodec adds a 0x0000 at the end of the string
      if (str.at(str.length() - 1).isNull()) str = str.left(str.length() - 1);

      *pos = ch;
      result += str;
//      for (i=0; i < (int)str.length(); i++)
//	result += (char)(QChar)str[i];

      pos = end -1;
    }
    else
    {
      //result += "=?";
      //pos = beg -1; // because pos gets increased shortly afterwards
      pos = beg - 2;
      result += *pos++;
      result += *pos;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
const char especials[18] = "()<>@,;:\"/[]?.= \033";

const QString KMMsgBase::encodeRFC2047String(const QString& _str,
  const QString& charset)
{
  if (_str.isEmpty()) return _str;
  QString cset;
  if (charset.isEmpty()) cset = KGlobal::locale()->charset();
    else cset = charset;
  QTextCodec *codec = codecForName(cset);
  QCString latin;
  if (charset == "us-ascii") latin = toUsAscii(_str);
  else if (codec) latin = codec->fromUnicode(_str);
    else latin = _str.local8Bit();
  int cr, start, stop, pos = 0;
  int latinLen = latin.length();
  char hexcode;
  int numQuotes, i;
  QString result = QString();
  while (pos < latinLen)
  {
    cr = pos;
    start = pos;
    while (cr < latinLen)
    {
      if (latin[cr] == 32) start = cr + 1;
      if (latin[cr] < 32) break;
      cr++;
    }
    if (cr < latinLen)
    {
      if (latin[start] == 34) start++;
      numQuotes = 1;
      while (cr < latinLen)
      {
        /* The encoded word must be limited to 75 character */
        for (i = 0; i < 17; i++) if (latin[cr] == especials[i]) numQuotes++;
        if (latin[cr] < 0) numQuotes++;
        /* Stop after 58 = 75 - 17 characters or at "<user@host..." */
        if (cr - start + 2 * numQuotes >= 58 || latin[cr] == 60) break;
        cr++;
      }
      if (cr < latinLen)
      {
        stop = cr - 1;
        while (stop >= start && latin[stop] != 32) stop--;
        if (latin[stop - 1] == 34) stop--;
        if (stop <= start) stop = cr;
      } else stop = cr;
      while (pos < start) { result += latin[pos]; pos++; }
      result += QString("=?%1?q?").arg(cset);
      while (pos < stop)
      {
        numQuotes = 0;
        for (i = 0; i < 17; i++) if (latin[pos] == especials[i]) numQuotes = 1;
        if (latin[pos] < 0) numQuotes = 1;
        if (numQuotes)
        {
          result += "=";
          hexcode = ((latin[pos] & 0xF0) >> 4) + 48;
          if (hexcode >= 58) hexcode += 7;
          result += hexcode;
          hexcode = (latin[pos] & 0x0F) + 48;
          if (hexcode >= 58) hexcode += 7;
          result += hexcode;
        } else {
          result += latin[pos];
        }
        pos++;
      }
      result += "?=";
    } else {
      while (pos < latinLen) { result += latin[pos]; pos++; }
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::encodeRFC2231String(const QString& _str,
  const QString& charset)
{
  if (_str.isEmpty()) return _str;
  QString cset;
  if (charset.isEmpty()) cset = KGlobal::locale()->charset();
    else cset = charset;
  QTextCodec *codec = codecForName(cset);
  QCString latin;
  if (charset == "us-ascii") latin = toUsAscii(_str);
  else if (codec) latin = codec->fromUnicode(_str);
    else latin = _str.local8Bit();

  char *l = latin.data();
  char hexcode;
  int i;
  bool quote;
  while (*l)
  {
    if (*l < 32) break;
    l++;
  }
  if (!*l) return latin;
  QString result = cset;
  result += QString("''");
  l = latin.data();
  while (*l)
  {
    quote = *l < 0;
    for (i = 0; i < 17; i++) if (*l == especials[i]) quote = true;
    if (quote)
    {
      result += "%";
      hexcode = ((*l & 0xF0) >> 4) + 48;
      if (hexcode >= 58) hexcode += 7;
      result += hexcode;
      hexcode = (*l & 0x0F) + 48;
      if (hexcode >= 58) hexcode += 7;
      result += hexcode;
    } else {
      result += *l;
    }
    l++;
  }
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMsgBase::decodeRFC2231String(const QString& _str)
{
  int p = _str.find("'");
  if (p < 0) return QString::fromLocal8Bit(_str);

  QString charset = _str.left(p);

  QCString st = _str.mid(_str.findRev("'") + 1).ascii();
  char ch, ch2;
  p = 0;
  while (p < (int)st.length())
  {
    if (st.at(p) == 37)
    {
      ch = st.at(p+1) - 48;
      if (ch > 16) ch -= 7;
      ch2 = st.at(p+2) - 48;
      if (ch2 > 16) ch2 -= 7;
      st.at(p) = ch * 16 + ch2;
      st.remove( p+1, 2 );
    }
    p++;
  }
  QString result;
  QTextCodec *codec = codecForName(charset);
  if (!codec) codec = codecForName(KGlobal::locale()->charset());
  if (codec) result = codec->toUnicode(st);
  else result = QString::fromLocal8Bit(st);

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
  return decodeRFC2047String(aStr);
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
