// kmmsgbase.cpp

#include <config.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kcharsets.h>

#include <kmime_util.h>
#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <qstringlist.h>
#include <kmfolder.h>
#include <kmheaders.h>
#include <kmmsgdict.h>
#include <krfcdate.h>

#include <ctype.h>
#include <stdlib.h>

#include <config.h>

#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 16 bit value.  */
#ifdef bswap_16
#define kmail_swap_16(x) bswap_16(x)
#else
#define kmail_swap_16(x) \
     ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#endif

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

/* Swap bytes in 64 bit value.  */
#ifdef bswap_64
#define kmail_swap_64(x) bswap_64(x)
#else
#define kmail_swap_64(x) \
     ((((x) & 0xff00000000000000ull) >> 56)				      \
      | (((x) & 0x00ff000000000000ull) >> 40)				      \
      | (((x) & 0x0000ff0000000000ull) >> 24)				      \
      | (((x) & 0x000000ff00000000ull) >> 8)				      \
      | (((x) & 0x00000000ff000000ull) << 8)				      \
      | (((x) & 0x0000000000ff0000ull) << 24)				      \
      | (((x) & 0x000000000000ff00ull) << 40)				      \
      | (((x) & 0x00000000000000ffull) << 56))
#endif

static KMMsgStatus sStatusList[] =
{
  KMMsgStatusDeleted, KMMsgStatusNew,
  KMMsgStatusUnread,  KMMsgStatusOld,
  KMMsgStatusRead,    KMMsgStatusReplied,
  KMMsgStatusSent,    KMMsgStatusQueued,
  KMMsgStatusFlag,
  KMMsgStatusUnknown /* "Unknown" must be at the *end* of the list */
};
static const int NUM_STATUSLIST = sizeof sStatusList / sizeof *sStatusList;


//-----------------------------------------------------------------------------
KMMsgBase::KMMsgBase(KMFolder* aParent)
{
  mParent  = aParent;
  mDirty   = FALSE;
  mIndexOffset = 0;
  mIndexLength = 0;
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


//----------------------------------------------------------------------------
KMMsgBase::KMMsgBase( const KMMsgBase& other )
{
    assign( &other );
}


//-----------------------------------------------------------------------------
bool KMMsgBase::isMessage(void) const
{
  return FALSE;
}

//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const KMMsgStatus aStatus, int idx)
{
  if (mParent)
    mParent->msgStatusChanged( status(), aStatus );
  mDirty = TRUE;
  if (mParent)
    mParent->headerOfMsgChanged(this, idx);
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


void KMMsgBase::setEncryptionState( const KMMsgEncryptionState status,
                                    int idx )
{
    qDebug( "***setEncryptionState1( %c )", status );
    mDirty = TRUE;
    if (mParent)
        mParent->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setEncryptionState( const char* status, int idx )
{
    qDebug( "***setEncryptionState2( %c )", status ? status[0] : '?' );
    
    if( status ){
        if( status[0] == (char)KMMsgEncryptionStateUnknown )
            setEncryptionState( KMMsgEncryptionStateUnknown, idx );
        else if( status[0] == (char)KMMsgNotEncrypted )
            setEncryptionState( KMMsgNotEncrypted, idx );
        else if( status[0] == (char)KMMsgPartiallyEncrypted )
            setEncryptionState( KMMsgPartiallyEncrypted, idx );
        else if( status[0] == (char)KMMsgFullyEncrypted )
            setEncryptionState( KMMsgFullyEncrypted, idx );
        else
            setEncryptionState( KMMsgEncryptionStateUnknown, idx );
    }
    else
        setEncryptionState( KMMsgEncryptionStateUnknown, idx );
}


void KMMsgBase::setSignatureState( const KMMsgSignatureState status,
                                   int idx ) 
{ 
    qDebug( "***setSignatureState1( %c )", status );
    mDirty = TRUE;
    if (mParent)
         mParent->headerOfMsgChanged(this, idx);
}



void KMMsgBase::setSignatureState( const char* status, int idx )
{
    qDebug( "***setSignatureState2( %c )", status ? status[0] : '?' );
    
    if( status ){
        if( status[0] == (char)KMMsgSignatureStateUnknown )
            setSignatureState( KMMsgSignatureStateUnknown, idx );
        else if( status[0] == (char)KMMsgNotSigned )
            setSignatureState( KMMsgNotSigned, idx );
        else if( status[0] == (char)KMMsgPartiallySigned )
            setSignatureState( KMMsgPartiallySigned,idx );
        else if( status[0] == (char)KMMsgFullySigned )
            setSignatureState( KMMsgFullySigned, idx );
        else
            setSignatureState( KMMsgSignatureStateUnknown, idx );
    }
    else
        setSignatureState( KMMsgSignatureStateUnknown, idx );
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
  setDate( KRFCDate::parseDate( aDateStr ) );
}


//-----------------------------------------------------------------------------
QString KMMsgBase::dateStr(void) const
{
  time_t d = date();
  return KMime::DateFormatter::formatDate(KMime::DateFormatter::Fancy, d);
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
  return KGlobal::charsets()->codecForName(_str.lower());
}


//-----------------------------------------------------------------------------
const QCString KMMsgBase::toUsAscii(const QString& _str, bool *ok)
{
  bool all_ok =true;
  QString result = _str;
  int len = result.length();
  for (int i = 0; i < len; i++)
    if (result.at(i).unicode() >= 128) {
      result.at(i) = '?';
      all_ok = false;
    }
  if (ok)
    *ok = all_ok;
  return result.latin1();
}


//-----------------------------------------------------------------------------
QStringList KMMsgBase::supportedEncodings(bool usAscii)
{
  QStringList encodingNames = KGlobal::charsets()->availableEncodingNames();
  QStringList encodings;
  QMap<QString,bool> mimeNames;
  for (QStringList::Iterator it = encodingNames.begin();
    it != encodingNames.end(); it++)
  {
    QTextCodec *codec = KGlobal::charsets()->codecForName(*it);
    QString mimeName = (codec) ? QString(codec->mimeName()).lower() : (*it);
    if (mimeNames.find(mimeName) == mimeNames.end())
    {
      encodings.append(KGlobal::charsets()->languageForEncoding(*it)
        + " ( " + mimeName + " )");
      mimeNames.insert(mimeName, TRUE);
    }
  }
  encodings.sort();
  if (usAscii) encodings.prepend(KGlobal::charsets()
    ->languageForEncoding("us-ascii") + " ( us-ascii )");
  return encodings;
}


//-----------------------------------------------------------------------------
QString KMMsgBase::decodeRFC2047String(const QCString& aStr)
{
  QString result;
  QCString charset;
  char *pos, *beg, *end, *mid=0;
  QCString str, cstr, LWSP_buffer;
  char encoding='Q', ch;
  bool valid, lastWasEncodedWord=FALSE;
  const int maxLen=200;
  int i;

  if (aStr.find("=?") < 0)
  {
    QString str = kernel->networkCodec()->toUnicode(aStr);
    if (str.find('\n') == -1) return str;
    QString str2((QChar*)NULL, str.length());
    uint i = 0;
    while (i < str.length())
    {
      if (str[i] == '\n')
      {
        str2 += " ";
        i += 2;
      } else {
        str2 += str[i];
        i++;
      }
    }
    return str2;
  }

  for (pos=aStr.data(); *pos; pos++)
  {
    // line unfolding
    if ( pos[0] == '\r' && pos[1] == '\n' ) {
      pos++;
      continue;
    }
    if ( pos[0] == '\n' )
      continue;
    // collect LWSP after encoded-words,
    // because we might need to throw it out
    // (when the next word is an encoded-word)
    if ( lastWasEncodedWord && ( pos[0] == ' ' || pos[0] == '\t' ) ) {
      LWSP_buffer += pos[0];
      continue;
    }
    // verbatimly copy normal text
    if (pos[0]!='=' || pos[1]!='?')
    {
      result += LWSP_buffer + pos[0];
      LWSP_buffer = 0;
      lastWasEncodedWord = FALSE;
      continue;
    }
    // found possible encoded-word
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
      // valid encoding: decode and throw away separating LWSP
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
      if (!codec) codec = kernel->networkCodec();
      result += codec->toUnicode(cstr);
      lastWasEncodedWord = TRUE;

      *pos = ch;
      pos = end -1;
    }
    else
    {
      // invalid encoding, keep separating LWSP.
      //result += "=?";
      //pos = beg -1; // because pos gets increased shortly afterwards
      pos = beg - 2;
      result += LWSP_buffer;
      result += *pos++;
      result += *pos;
      lastWasEncodedWord = FALSE;
    }
    LWSP_buffer = 0;
  }
  return result;
}


//-----------------------------------------------------------------------------
const QString especials = "()<>@,;:\"/[]?.= \033";
const QString dontQuote = "\"()<>,@";

QCString KMMsgBase::encodeRFC2047Quoted(const QCString& aStr, bool base64)
{
  if (base64)
      return encodeBase64(aStr).replace(QRegExp("\n"),"");
  QCString result;
  unsigned char ch, hex;
  for (unsigned int i = 0; i < aStr.length(); i++)
  {
    ch = aStr.at(i);
    if (ch >= 128 || ch == '_' || especials.find(aStr.at(i)) != -1)
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
  if (charset.isEmpty()) cset = QCString(kernel->networkCodec()->mimeName()).lower();
    else cset = charset;
  QTextCodec *codec = codecForName(cset);
  if (!codec) codec = kernel->networkCodec();

  unsigned int nonAscii = 0;
  for (unsigned int i = 0; i < _str.length(); i++)
    if (_str.at(i).unicode() >= 128) nonAscii++;
  bool useBase64 = (nonAscii * 6 > _str.length());

  unsigned int start, stop, p, pos = 0, encLength;
  QCString result;
  bool breakLine = FALSE;
  const unsigned int maxLen = 75 - 7 - cset.length();

  while (pos < _str.length())
  {
    start = pos; p = pos;
    while (p < _str.length())
    {
      if (!breakLine && (_str.at(p) == ' ' || dontQuote.find(_str.at(p)) != -1))
        start = p + 1;
      if (_str.at(p).unicode() >= 128 || _str.at(p) < ' ') break;
      p++;
    }
    if (breakLine || p < _str.length())
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
      if (result.right(3) == "?= ") start--;
      if (result.right(5) == "?=\n  ") {
        start--; result.truncate(result.length() - 1);
      }
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
  if (charset.isEmpty()) cset = QCString(kernel->networkCodec()->mimeName()).lower();
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
  if (p < 0) return kernel->networkCodec()->toUnicode(_str);

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
  if (!codec) codec = kernel->networkCodec();
  if (codec) result = codec->toUnicode(st);
  else result = kernel->networkCodec()->toUnicode(st);

  return result;
}

//-----------------------------------------------------------------------------
QCString KMMsgBase::autoDetectCharset(const QCString &_encoding, const QStringList &encodingList, const QString &text)
{
    QStringList charsets = encodingList;
    if (!_encoding.isEmpty())
    {
       QString currentCharset = QString::fromLatin1(_encoding);
       charsets.remove(currentCharset);
       charsets.prepend(currentCharset);
    }

    QStringList::ConstIterator it = charsets.begin();
    for (; it != charsets.end(); ++it)
    {
       QCString encoding = (*it).latin1();
       if (encoding == "locale")
          encoding = QCString(kernel->networkCodec()->mimeName()).lower();
       if (text.isEmpty())
         return encoding;
       if (encoding == "us-ascii") {
         bool ok;
         (void) KMMsgBase::toUsAscii(text, &ok);
         if (ok)
            return encoding;
       }
       else
       {
         QTextCodec *codec = KMMsgBase::codecForName(encoding);
         if (!codec) {
           kdDebug(5006) << "Auto-Charset: Something is wrong and I can not get a codec. [" << encoding << "]" << endl;
         } else {
           if (codec->canEncode(text))
              return encoding;
         }
       }
    }
    return 0;
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
unsigned long KMMsgBase::getMsgSerNum() const
{
  unsigned long msn = 0;
  if (mParent) {
    int index = mParent->find((KMMsgBase*)this);
    msn = kernel->msgDict()->getMsgSerNum(mParent, index);
  }
  return msn;
}

//-----------------------------------------------------------------------------
void swapEndian(QString &str)
{
  ushort us;
  uint len = str.length();
  for (uint i = 0; i < len; i++)
  {
    us = str[i].unicode();
    str[i] = QChar(kmail_swap_16(us));
  }
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
#define COPY_HEADER_TYPE(x) Q_ASSERT(sizeof(x) == sizeof(Q_UINT32)); COPY_DATA(&x, sizeof(x));
#define COPY_HEADER_LEN(x)  Q_ASSERT(sizeof(x) == sizeof(Q_UINT16)); COPY_DATA(&x, sizeof(x));
//-----------------------------------------------------------------------------
QString KMMsgBase::getStringPart(MsgPartType t) const
{
  QString ret("");

  g_chunk_offset = 0;
  bool using_mmap = FALSE;
  bool swapByteOrder = mParent->indexSwapByteOrder();
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
    off_t first_off=ftell(mParent->mIndexStream);
    fseek(mParent->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, mParent->mIndexStream);
    fseek(mParent->mIndexStream, first_off, SEEK_SET);
  }

  MsgPartType type;
  Q_UINT16 l;
  while(g_chunk_offset < mIndexLength) {
    Q_UINT32 tmp;
    COPY_HEADER_TYPE(tmp);
    COPY_HEADER_LEN(l);
    if (swapByteOrder)
    {
       tmp = kmail_swap_32(tmp);
       l = kmail_swap_16(l);
    }
    type = (MsgPartType) tmp;
    if(g_chunk_offset + l > mIndexLength) {
	kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl;
	break;
    }
    if(type == t) {
        // This works because the QString constructor does a memcpy.
        // Otherwise we would need to be concerned about the alignment.
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
  // Normally we need to swap the byte order because the QStrings are written
  // in the style of Qt2 (MSB -> network ordered).
  // QStrings in Qt3 expect host ordering.
  // On e.g. Intel host ordering is LSB, on e.g. Sparc it is MSB.

#ifndef WORDS_BIGENDIAN
  // #warning Byte order is little endian (swap is true)
  swapEndian(ret);
#else
  // #warning Byte order is big endian (swap is false)
#endif

  return ret;
}

//-----------------------------------------------------------------------------
off_t KMMsgBase::getLongPart(MsgPartType t) const
{
  off_t ret = 0;

  g_chunk_offset = 0;
  bool using_mmap = FALSE;
  int sizeOfLong = mParent->indexSizeOfLong();
  bool swapByteOrder = mParent->indexSwapByteOrder();
  if (mParent->indexStreamBasePtr()) {
    if (g_chunk)
      free(g_chunk);
    using_mmap = TRUE;
    g_chunk = mParent->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if (!mParent->mIndexStream)
      return ret;
    assert(mIndexLength >= 0);
    if (g_chunk_length < mIndexLength)
      g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    off_t first_off=ftell(mParent->mIndexStream);
    fseek(mParent->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, mParent->mIndexStream);
    fseek(mParent->mIndexStream, first_off, SEEK_SET);
  }

  MsgPartType type;
  Q_UINT16 l;
  while (g_chunk_offset < mIndexLength) {
    Q_UINT32 tmp;
    COPY_HEADER_TYPE(tmp);
    COPY_HEADER_LEN(l);
    if (swapByteOrder)
    {
       tmp = kmail_swap_32(tmp);
       l = kmail_swap_16(l);
    }
    type = (MsgPartType) tmp;

    if (g_chunk_offset + l > mIndexLength) {
      kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl;
      break;
    }
    if(type == t) {
      assert(sizeOfLong == l);
      if (sizeOfLong == sizeof(ret))
      {
         COPY_DATA(&ret, sizeof(ret));
         if (swapByteOrder)
         {
            if (sizeof(ret) == 4)
               ret = kmail_swap_32(ret);
            else
               ret = kmail_swap_64(ret);
         }
      }
      else if (sizeOfLong == 4)
      {
         // Long is stored as 4 bytes in index file, sizeof(long) = 8
         Q_UINT32 ret_32;
         COPY_DATA(&ret_32, sizeof(ret_32));
         if (swapByteOrder)
            ret_32 = kmail_swap_32(ret_32);
         ret = ret_32;
      }
      else if (sizeOfLong == 8)
      {
         // Long is stored as 8 bytes in index file, sizeof(long) = 4
         Q_UINT32 ret_1;
         Q_UINT32 ret_2;
         COPY_DATA(&ret_1, sizeof(ret_1));
         COPY_DATA(&ret_2, sizeof(ret_2));
         if (!swapByteOrder)
         {
            // Index file order is the same as the order of this CPU.
#ifndef WORDS_BIGENDIAN
            // Index file order is little endian
            ret = ret_1; // We drop the 4 most significant bytes
#else
            // Index file order is big endian
            ret = ret_2; // We drop the 4 most significant bytes
#endif
         }
         else
         {
            // Index file order is different from this CPU.
#ifndef WORDS_BIGENDIAN
            // Index file order is big endian
            ret = ret_2; // We drop the 4 most significant bytes
#else
            // Index file order is little endian
            ret = ret_1; // We drop the 4 most significant bytes
#endif
            // We swap the result to host order.
            ret = kmail_swap_32(ret);
         }

      }
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
	int len2 = (len > 256) ? 256 : len; \
	if(csize < (length + (len2 + sizeof(short) + sizeof(MsgPartType)))) \
    	   ret = (uchar *)realloc(ret, csize += len2+sizeof(short)+sizeof(MsgPartType)); \
        Q_UINT32 t = (Q_UINT32) type; memcpy(ret+length, &t, sizeof(t)); \
        Q_UINT16 l = len2; memcpy(ret+length+sizeof(t), &l, sizeof(l)); \
        memcpy(ret+length+sizeof(t)+sizeof(l), x, len2); \
        length += len2+sizeof(t)+sizeof(l); \
    } while(0)
#define STORE_DATA(type, x) STORE_DATA_LEN(type, &x, sizeof(x))
#ifndef WORDS_BIGENDIAN
  // #warning Byte order is little endian (call swapEndian)
#define SWAP_TO_NETWORK_ORDER(x) swapEndian(x)
#else
  // #warning Byte order is big endian
#define SWAP_TO_NETWORK_ORDER(x)
#endif
  unsigned long tmp;
  QString tmp_str;

  //these is at the beginning because it is queried quite often
  tmp_str = msgIdMD5().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2);
  tmp = status();
  STORE_DATA(MsgStatusPart, tmp);

  //these are completely arbitrary order
  tmp_str = fromStrip().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgFromPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = subject().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgSubjectPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = toStrip().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgToPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = replyToIdMD5().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgReplyToIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = xmark().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgXMarkPart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp_str = fileName().stripWhiteSpace();
  SWAP_TO_NETWORK_ORDER(tmp_str);
  STORE_DATA_LEN(MsgFilePart, tmp_str.unicode(), tmp_str.length() * 2);
  tmp = msgSize();
  STORE_DATA(MsgSizePart, tmp);
  tmp = folderOffset();
  STORE_DATA(MsgOffsetPart, tmp);
  tmp = date();
  STORE_DATA(MsgDatePart, tmp);
  tmp = (signatureState() << 16) | encryptionState();
  STORE_DATA(MsgCryptoStatePart, tmp);
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
    Q_ASSERT(mParent->mIndexStream);
    fseek(mParent->mIndexStream, mIndexOffset, SEEK_SET);
    fwrite( buffer, len, 1, mParent->mIndexStream);
    return TRUE;
  }
  return FALSE;
}

