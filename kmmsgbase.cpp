// kmmsgbase.cpp

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include "kmmsgbase.h"
#include <mimelib/mimepp.h>
#include <qtextcodec.h>
#include <qregexp.h>
#include <kmfolder.h>
#include <kmheaders.h>

#include <ctype.h>
#include <stdlib.h>

#define NUM_STATUSLIST 10
static KMMsgStatus sStatusList[NUM_STATUSLIST] =
{
  KMMsgStatusDeleted, KMMsgStatusNew,
  KMMsgStatusUnread,  KMMsgStatusOld,
  KMMsgStatusRead,    KMMsgStatusReplied,
  KMMsgStatusSent,    KMMsgStatusQueued,
  KMMsgStatusFlag,
  KMMsgStatusUnknown /* "Unknown" must be at the *end* of the list */
};


//-----------------------------------------------------------------------------
KMMsgBase::KMMsgBase(KMFolder* aParent)
{
  mParent  = aParent;
  mDirty   = FALSE;
  mIndexOffset = mIndexLength = 0;
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
  mIndexOffset = other->mIndexOffset;
  mIndexLength = other->mIndexLength;
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
  if (mParent)
    mParent->msgStatusChanged( status(), aStatus );
  mDirty = TRUE;
  if (mParent)
    mParent->headerOfMsgChanged(this);
}



//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const char* aStatusStr, const char* aXStatusStr)
{
  setStatus(KMMsgStatusUnknown);
  // first try to find status from "X-Status" field if given
  if (aXStatusStr) {
    for (int i=0; i<NUM_STATUSLIST-1; i++) {
      if (strchr(aXStatusStr, (char)sStatusList[i])) {
	  setStatus(sStatusList[i]);
	  break;
      }
    }
  }

  // if not successful then use the "Status" field
  if (status() == KMMsgStatusUnknown)
  {
    if (aStatusStr && ((aStatusStr[0]==(char)KMMsgStatusRead && aStatusStr[1]==(char)KMMsgStatusOld) ||
		       (aStatusStr[0]==(char)KMMsgStatusOld && aStatusStr[1]==(char)KMMsgStatusRead)))
	setStatus(KMMsgStatusOld);
    else if (aStatusStr && aStatusStr[0]==(char)KMMsgStatusRead)
	setStatus(KMMsgStatusRead);
    else if (aStatusStr && aStatusStr[0]==(char)KMMsgStatusDeleted)
	setStatus(KMMsgStatusDeleted);
    else
	setStatus(KMMsgStatusNew);
  }
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
void KMMsgBase::setDate(const QCString& aDateStr)
{
  DwDateTime dwDate;
  dwDate.FromString(aDateStr);
  dwDate.Parse();
  setDate(dwDate.AsUnixTime());
}


//-----------------------------------------------------------------------------
QString KMMsgBase::dateStr(void) const
{
  time_t d = date();
  return KMHeaders::fancyDate(d);
}


//-----------------------------------------------------------------------------
QString KMMsgBase::skipKeyword(const QString& aStr, QChar sepChar,
			       bool* hasKeyword)
{
  unsigned int i = 0, maxChars = 3;
  QString str = aStr;

  while (str[0] == ' ') str.remove(0,1);
  if (hasKeyword) *hasKeyword=FALSE;

  for (i=0; i < str.length() && i < maxChars; i++)
  {
    if (str[i] < 'A' || str[i] == sepChar) break;
  }

  if (str[i] == sepChar) // skip following spaces too
  {
    do {
      i++;
    } while (str[i] == ' ');
    if (hasKeyword) *hasKeyword=TRUE;
    return str.mid(i);
  }
  return str;
}


//-----------------------------------------------------------------------------
QTextCodec* KMMsgBase::codecForName(const QCString& _str)
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
    QString result = _str;
  int len = result.length();
  for (int i = 0; i < len; i++)
    if (result.at(i).unicode() >= 128) result.at(i) = '?';
  return result.latin1();
}


//-----------------------------------------------------------------------------
QString KMMsgBase::decodeRFC2047String(const QCString& aStr)
{
  QString result;
  QCString charset;
  char *pos, *beg, *end, *mid;
  QCString str, cstr;
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
      str = QCString(mid).left((int)(mid - pos - 1));
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
      if (!codec) codec = codecForName(KGlobal::locale()->charset().latin1());
      if (codec) result += codec->toUnicode(cstr);
      else result += QString::fromLocal8Bit(cstr);

      *pos = ch;
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
  return result.replace(QRegExp("\n[ \t]")," ");
}


//-----------------------------------------------------------------------------
const QString especials = "()<>@,;:\"/[]?.= \033";
const QString dontQuote = "\"()<>,";

QCString KMMsgBase::encodeRFC2047Quoted(const QCString& aStr, bool base64)
{
  if (base64)
      return encodeBase64(aStr).replace(QRegExp("\n"),"");
  QCString result;
  unsigned char ch, hex;
  for (unsigned int i = 0; i < aStr.length(); i++)
  {
    ch = aStr.at(i);
    if (ch >= 128 || especials.find(aStr.at(i)) != -1)
    {
      result += "=";
      hex = ((ch & 0xF0) >> 4) + 48;
      if (hex >= 58) hex += 7;
      result += hex;
      hex = (ch & 0x0F) + 48;
      if (hex >= 58) hex += 7;
      result += hex;
    } else {
      result += aStr.at(i);
    }
  }
  return result;
}


QCString KMMsgBase::encodeRFC2047String(const QString& _str,
  const QCString& charset)
{
  if (_str.isEmpty()) return QCString();
  if (charset == "us-ascii") return toUsAscii(_str);

  QCString cset;
  if (charset.isEmpty()) cset = KGlobal::locale()->charset().latin1();
    else cset = charset;
  QTextCodec *codec = codecForName(cset);
  if (!codec) codec = QTextCodec::codecForLocale();

  unsigned int nonAscii = 0;
  for (unsigned int i = 0; i < _str.length(); i++)
    if (_str.at(i).unicode() >= 128) nonAscii++;
  bool useBase64 = (nonAscii * 6 > _str.length());

  unsigned int start, stop, p, pos = 0, encLength;
  QCString result;
  bool breakLine;
  const unsigned int maxLen = 75 - 7 - cset.length();

  while (pos < _str.length())
  {
    start = pos; p = pos;
    while (p < _str.length())
    {
      if (_str.at(p) == ' ' || dontQuote.find(_str.at(p)) != -1) start = p + 1;
      if (_str.at(p).unicode() >= 128 || _str.at(p) < ' ') break;
      p++;
    }
    if (p < _str.length())
    {
      while (dontQuote.find(_str.at(start)) != -1) start++;
      stop = start;
      while (stop < _str.length() && dontQuote.find(_str.at(stop)) == -1)
        stop++;
      result += _str.mid(pos, start - pos).latin1();
      encLength = encodeRFC2047Quoted(codec->fromUnicode(_str.
        mid(start, stop - start)), useBase64).length();
      breakLine = (encLength > maxLen);
      if (breakLine)
      {
        int dif = (stop - start) / 2;
        int step = dif;
        while (abs(step) > 1)
        {
          encLength = encodeRFC2047Quoted(codec->fromUnicode(_str.
            mid(start, dif)), useBase64).length();
          step = (encLength > maxLen) ? (-abs(step) / 2) : (abs(step) / 2);
          dif += step;
        }
        stop = start + dif;
      }
      p = stop;
      while (p > start && _str.at(p) != ' ') p--;
      if (p > start) stop = p;
      int lastNewLine = result.findRev("\n ");
      if (!result.mid(lastNewLine).stripWhiteSpace().isEmpty()
        && result.length() - lastNewLine + encLength + 2 > maxLen)
          result += "\n ";
      result += "=?";
      result += cset;
      result += (useBase64) ? "?b?" : "?q?";
      result += encodeRFC2047Quoted(codec->fromUnicode(_str.mid(start,
        stop - start)), useBase64);
      result += "?=";
      if (breakLine) result += "\n ";
      pos = stop;
    } else {
      result += _str.mid(pos).latin1();
      break;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::encodeRFC2231String(const QString& _str,
  const QCString& charset)
{
  if (_str.isEmpty()) return QCString();
  QCString cset;
  if (charset.isEmpty()) cset = KGlobal::locale()->charset().latin1();
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
  QCString result = cset + "''";
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
QString KMMsgBase::decodeRFC2231String(const QCString& _str)
{
  int p = _str.find("'");
  if (p < 0) return QString::fromLocal8Bit(_str);

  QCString charset = _str.left(p);

  QCString st = _str.mid(_str.findRev("'") + 1);
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
  if (!codec) codec = codecForName(KGlobal::locale()->charset().latin1());
  if (codec) result = codec->toUnicode(st);
  else result = QString::fromLocal8Bit(st);

  return result;
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::decodeQuotedPrintable(const QCString& aStr)
{
  QCString bStr = aStr;
  if (aStr.isNull())
    bStr = "";

  DwString dwsrc(bStr.data());
  DwString dwdest;

  DwDecodeQuotedPrintable(dwsrc, dwdest);
  return dwdest.c_str();
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::encodeQuotedPrintable(const QCString& aStr)
{
  QCString bStr = aStr;
  if (aStr.isNull())
    bStr = "";

  DwString dwsrc(bStr.data(), bStr.length());
  DwString dwdest;
  QCString result;

  DwEncodeQuotedPrintable(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::decodeBase64(const QCString& aStr)
{
  QCString bStr = aStr;
  if (aStr.isNull())
    bStr = "";
  while (bStr.length() < 16) bStr += "=";

  DwString dwsrc(bStr.data(), bStr.length());
  DwString dwdest;
  QCString result;

  DwDecodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::encodeBase64(const QCString& aStr)
{
  QCString bStr = aStr;
  if (aStr.isNull())
    bStr = "";

  DwString dwsrc(bStr.data(), bStr.length());
  DwString dwdest;
  QCString result;

  DwEncodeBase64(dwsrc, dwdest);
  result = dwdest.c_str();
  return result;
}

//-----------------------------------------------------------------------------
static int g_chunk_length = 0, g_chunk_offset=0;
static uchar *g_chunk = NULL;

#define COPY_DATA(x, length) do { \
     if(g_chunk_offset + ((int)length) > g_chunk_length) {\
        g_chunk_offset = g_chunk_length; \
        kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl; \
        memset(x, length, '\0'); \
     } else { \
        memcpy(x, g_chunk+g_chunk_offset, length); \
	g_chunk_offset += length; \
     } } while(0)
#define COPY_HEADER_TYPE(x) ASSERT(sizeof(x) == sizeof(MsgPartType)); COPY_DATA(&x, sizeof(x))
#define COPY_HEADER_LEN(x)  ASSERT(sizeof(x) == sizeof(short)); COPY_DATA(&x, sizeof(x));
//-----------------------------------------------------------------------------
QString KMMsgBase::getStringPart(MsgPartType t) const
{
  QString ret("");

  g_chunk_offset = 0;
  bool using_mmap = FALSE;
  if (mParent->indexStreamBasePtr()) {
    if (g_chunk)
	free(g_chunk);
    using_mmap = TRUE;
    g_chunk = mParent->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if(!mParent->mIndexStream)
      return ret;
    if (g_chunk_length < mIndexLength)
	g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    int first_off=ftell(mParent->mIndexStream);
    fseek(mParent->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, mParent->mIndexStream);
    fseek(mParent->mIndexStream, first_off, SEEK_SET);
  }

  MsgPartType type;
  short l;
  while(g_chunk_offset < mIndexLength) {
    COPY_HEADER_TYPE(type);
    COPY_HEADER_LEN(l);
    if(g_chunk_offset + l > mIndexLength) {
	kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl;
	break;
    }
    if(type == t) {
	if(l)
	    ret = QString((QChar *)(g_chunk + g_chunk_offset), l/2);
	break;
    }
    g_chunk_offset += l;
  }
  if(using_mmap) {
      g_chunk_length = 0;
      g_chunk = NULL;
  }
  return ret;
}

//-----------------------------------------------------------------------------
unsigned long KMMsgBase::getLongPart(MsgPartType t) const
{
  unsigned long ret = 0;

  g_chunk_offset = 0;
  bool using_mmap = FALSE;
  if (mParent->indexStreamBasePtr()) {
    if (g_chunk)
      free(g_chunk);
    using_mmap = TRUE;
    g_chunk = mParent->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if (!mParent->mIndexStream)
      return ret;
    if (g_chunk_length < mIndexLength)
      g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    int first_off=ftell(mParent->mIndexStream);
    fseek(mParent->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, mParent->mIndexStream);
    fseek(mParent->mIndexStream, first_off, SEEK_SET);
  }

  MsgPartType type;
  short l;
  while (g_chunk_offset < mIndexLength) {
    COPY_HEADER_TYPE(type);
    COPY_HEADER_LEN(l);

    if (g_chunk_offset + l > mIndexLength) {
      kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl;
      break;
    }
    if(type == t) {
      ASSERT(l == sizeof(unsigned long));
      COPY_DATA(&ret, sizeof(ret));
      break;
    }
    g_chunk_offset += l;
  }
  if(using_mmap) {
    g_chunk_length = 0;
    g_chunk = NULL;
  }
  return ret;
}
#undef COPY_DATA

//-----------------------------------------------------------------------------
const uchar *KMMsgBase::asIndexString(int &length) const
{
  unsigned int csize = 256;
  static uchar *ret = NULL; //different static buffer here for we may use the other buffer in the functions below
  if(!ret)
    ret = (uchar *)malloc(csize);
  length = 0;

#define STORE_DATA_LEN(type, x, len) do { \
	if(csize < (length + (len + sizeof(short) + sizeof(MsgPartType)))) \
    	   ret = (uchar *)realloc(ret, csize += QMAX(256, (len+sizeof(short)+sizeof(MsgPartType)))); \
        MsgPartType t = type; memcpy(ret+length, &t, sizeof(MsgPartType)); \
        short l = len; memcpy(ret+length+sizeof(MsgPartType), &l, sizeof(short)); \
        memcpy(ret+length+sizeof(short)+sizeof(MsgPartType), x, len); \
        length += len + sizeof(short) + sizeof(MsgPartType); \
    } while(0)
#define STORE_DATA(type, x) STORE_DATA_LEN(type, &x, sizeof(x))
  unsigned long tmp;
  QString tmp_str;

  //these is at the beginning because it is queried quite often
  tmp_str = msgIdMD5().stripWhiteSpace();
  STORE_DATA_LEN(MsgIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2);
  tmp = status();
  STORE_DATA(MsgStatusPart, tmp);

  //these are completely arbitrary order
  tmp_str = fromStrip().stripWhiteSpace();
  STORE_DATA_LEN(MsgFromPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = subject().stripWhiteSpace();
  STORE_DATA_LEN(MsgSubjectPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = toStrip().stripWhiteSpace();
  STORE_DATA_LEN(MsgToPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = replyToIdMD5().stripWhiteSpace();
  STORE_DATA_LEN(MsgReplyToIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = xmark().stripWhiteSpace();
  STORE_DATA_LEN(MsgXMarkPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp = msgSize();
  STORE_DATA(MsgSizePart, tmp);
  tmp = folderOffset();
  STORE_DATA(MsgOffsetPart, tmp);
  tmp = date();
  STORE_DATA(MsgDatePart, tmp);
#undef STORE_DATA_LEN
  return ret;
}

bool KMMsgBase::syncIndexString() const
{
  if(!dirty())
    return TRUE;
  int len;
  const uchar *buffer = asIndexString(len);
  if (len == mIndexLength) {
    ASSERT(mParent->mIndexStream);
    fseek(mParent->mIndexStream, mIndexOffset, SEEK_SET);
    fwrite( buffer, len, 1, mParent->mIndexStream);
    return TRUE;
  }
  return FALSE;
}
	
