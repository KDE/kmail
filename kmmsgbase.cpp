// kmmsgbase.cpp

#include "kmmsgbase.h"

#include <config-kmail.h>

#include "globalsettings.h"
#include "kmfolderindex.h"
#include "kmfolder.h"
#include "kmmsgdict.h"
#include "kmmessagetag.h"
#include "messageproperty.h"
#include <QByteArray>
using KMail::MessageProperty;

#include <kdebug.h>
#include <kde_file.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kascii.h>
#include <kcodecs.h>
#include <kdatetime.h>
#include <kconfiggroup.h>

#include <mimelib/mimepp.h>
#include <kmime/kmime_codecs.h>
#include <kmime/kmime_util.h>

#include <QTextCodec>
#include <QRegExp>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include <kmime/kmime_dateformatter.h> // kdepimlibs

#ifdef HAVE_BYTESWAP_H
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

//-----------------------------------------------------------------------------
KMMsgBase::KMMsgBase(KMFolder* aParentFolder)
  : mParent( aParentFolder ),
#ifdef KMAIL_SQLITE_INDEX
    mData( 0 ),
    mDbId( 0 ),
#else
    mIndexOffset( 0 ),
#endif
    mIndexLength( 0 ),
    mDirty( false ),
    mEnableUndo( false ),
    mStatus(),
    mTagList( 0 ),
    mStringPartCacheBuilt( false )
{
}

#ifdef KMAIL_SQLITE_INDEX
KMMsgBase::KMMsgBase( KMFolder* aParentFolder, char* data, short len, sqlite_int64 dbId )
  : mParent( aParentFolder ),
    mData( data ),
    mDbId( dbId ),
#else
KMMsgBase::KMMsgBase(KMFolder* aParentFolder, off_t off, short len)
  : mParent( aParentFolder ),
    mIndexOffset( off ),
#endif
    mIndexLength( len ),
    mDirty( false ),
    mEnableUndo( false ),
    mStatus(),
    mTagList( 0 ),
    mStringPartCacheBuilt( false )
{
}

//----------------------------------------------------------------------------
KMMsgBase::KMMsgBase( const KMMsgBase& other )
 : mTagList( 0 )
#ifdef KMAIL_SQLITE_INDEX
 , mData( 0 )
#endif
 , mStringPartCacheBuilt( false )
{
    assign( &other );
}

//-----------------------------------------------------------------------------
KMMsgBase::~KMMsgBase()
{
#if 0 //TODO port to akonadi
  MessageProperty::forget( this );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  delete mTagList;
#ifdef KMAIL_SQLITE_INDEX
  free( mData );
#endif
}

KMFolderIndex* KMMsgBase::storage() const
{
  // TODO: How did this ever work? What about KMFolderSearch that does
  // not inherit KMFolderIndex?
  if( mParent )
    return static_cast<KMFolderIndex*>( mParent->storage() );
  return 0;
}

//-----------------------------------------------------------------------------
void KMMsgBase::assign(const KMMsgBase* other)
{
  mParent = other->mParent;
  mDirty  = other->mDirty;
#ifdef KMAIL_SQLITE_INDEX
  free( mData );
  mData = (char*)malloc( other->mIndexLength );
  mDbId = other->mDbId;
  memcpy( mData, other->mData, other->mIndexLength );
#else
  mIndexOffset = other->mIndexOffset;
#endif
  mIndexLength = other->mIndexLength;
  //Not sure why these 4 are copied here, but mStatus is not. Nevertheless,
  //it probably does no harm to copy the taglist here
  if ( other->tagList() ) {
   if ( !mTagList )
     mTagList = new KMMessageTagList();
   *mTagList = *other->tagList();
  } else {
    delete mTagList;
    mTagList = 0;
  }
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
  return false;
}

//-----------------------------------------------------------------------------
void KMMsgBase::toggleStatus(const MessageStatus& aStatus, int idx)
{

  mDirty = true;
  MessageStatus oldStatus = mStatus;
  mStatus.toggle( aStatus );
  if (storage()) {
     if (idx < 0)
       idx = storage()->find( this );
     storage()->msgStatusChanged( oldStatus, mStatus, idx );
     storage()->headerOfMsgChanged(this, idx);
  }

}

//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const MessageStatus& aStatus, int idx)
{
  MessageStatus oldStatus = mStatus;
  mStatus.set( aStatus );
  if ( oldStatus != mStatus ) {
    mDirty = true;
    if ( storage() ) {
      if (idx < 0)
        idx = storage()->find( this );
      storage()->msgStatusChanged( oldStatus, mStatus, idx );
      storage()->headerOfMsgChanged( this, idx );
    }
  }
}

//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const char* aStatusStr, const char* aXStatusStr)
{
  // cumulate the changes in "status" to avoid multiple emits of
  // msgStatusChanged(...) when calling setStatus() multiple times
  MessageStatus status;

  // first try to find status from "X-Status" field if given
  if (aXStatusStr) {
    status.setStatusFromStr( aXStatusStr );
  }

  // Merge the contents of the "Status" field
  if (aStatusStr) {
    if ((aStatusStr[0]== 'R' && aStatusStr[1]== 'O') ||
        (aStatusStr[0]== 'O' && aStatusStr[1]== 'R')) {
      status.setOld();
      status.setRead();
    }
    else if (aStatusStr[0] == 'R')
      status.setRead();
    else if (aStatusStr[0] == 'D')
      status.setDeleted();
    else
      status.setNew();
  }
  setStatus( status );
}


void KMMsgBase::setEncryptionState( const KMMsgEncryptionState /*status*/, int idx )
{
    //kDebug() << "***setEncryptionState1(" << status << ")";
    mDirty = true;
    if (storage())
        storage()->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setEncryptionStateChar( QChar status, int idx )
{
    //kDebug() << "***setEncryptionState2(" << (status.isNull() ? '?' : status.toLatin1()) << ")";

    if( status.toLatin1() == (char)KMMsgEncryptionStateUnknown )
        setEncryptionState( KMMsgEncryptionStateUnknown, idx );
    else if( status.toLatin1() == (char)KMMsgNotEncrypted )
        setEncryptionState( KMMsgNotEncrypted, idx );
    else if( status.toLatin1() == (char)KMMsgPartiallyEncrypted )
        setEncryptionState( KMMsgPartiallyEncrypted, idx );
    else if( status.toLatin1() == (char)KMMsgFullyEncrypted )
        setEncryptionState( KMMsgFullyEncrypted, idx );
    else
        setEncryptionState( KMMsgEncryptionStateUnknown, idx );
}


void KMMsgBase::setSignatureState( const KMMsgSignatureState /*status*/, int idx )
{
    //kDebug() << "***setSignatureState1(" << status << ")";
    mDirty = true;
    if (storage())
         storage()->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setMDNSentState( KMMsgMDNSentState, int idx ) {
  mDirty = true;
  if ( storage() )
    storage()->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setSignatureStateChar( QChar status, int idx )
{
    //kDebug() << "***setSignatureState2(" << (status.isNull() ? '?' : status.toLatin1()) << ")";

    if( status.toLatin1() == (char)KMMsgSignatureStateUnknown )
        setSignatureState( KMMsgSignatureStateUnknown, idx );
    else if( status.toLatin1() == (char)KMMsgNotSigned )
        setSignatureState( KMMsgNotSigned, idx );
    else if( status.toLatin1() == (char)KMMsgPartiallySigned )
        setSignatureState( KMMsgPartiallySigned,idx );
    else if( status.toLatin1() == (char)KMMsgFullySigned )
        setSignatureState( KMMsgFullySigned, idx );
    else
        setSignatureState( KMMsgSignatureStateUnknown, idx );
}

//-----------------------------------------------------------------------------
MessageStatus& KMMsgBase::status() const
{
  return mStatus;
}

//-----------------------------------------------------------------------------
const MessageStatus& KMMsgBase::messageStatus() const
{
  return mStatus;
}


//-----------------------------------------------------------------------------
void KMMsgBase::setDate(const QByteArray& aDateStr)
{
  KDateTime extractedDate = KDateTime::fromString( aDateStr, KDateTime::RFCDate );
  if ( extractedDate.isValid() || aDateStr.isEmpty() )
  {
    setDate( extractedDate.toTime_t() );
    return;
  }

  // KDateTime doesn't handle slightly non-standard formats.
  // Let's try to do better.

  QString dateString = aDateStr.trimmed();

  // Look for a very common non-standard RFC format

  // Thu, 17 Jul 2008 14:03:39 +0200 (CEST)
  // Wed, 9 Jul 2008 18:22:45 +0200 (MET DST)
  // Thu, 17 Jul 2008 07:50:24 -0400 (EDT)
  // Wed, 16 Jul 2008 16:04:37 -0700 (PDT)
  // Thu, 17 Jul 2008 04:21:20 +0000 (UTC)
  // Wed, 09 Jan 2008 18:50:36 +0900 (JST)
  // Wed, 9 Jan 2008 03:17:37 +0000 (GMT)
  // Mon, 23 Jun 2008 00:40:38 +0600 (YEKST)

  if ( aDateStr[ aDateStr.length() - 1 ] == ')' )
  {
    // Might be RFC format followed by (TIMEZONE). Very common.
    QRegExp regularExpression("^(.+)\\s+\\([A-Z]{3,5}(\\s[A-Z]{3})?\\)$");
    if ( dateString.indexOf(regularExpression) == 0 )
    {
      QStringList parts = regularExpression.capturedTexts();

      Q_ASSERT( parts.size() >= 2 ); // Must be at least 2 (as per Qt docs)

      extractedDate = KDateTime::fromString( parts[1], KDateTime::RFCDate );
      if ( extractedDate.isValid() )
      {
        setDate( extractedDate.toTime_t() );
        return;
      }
    }
  }

  // Something that I've seen around and we don't handle yet is

  // Sat, 21 Jun 2008 8:58:23 -0400

  // which doesn't seem to have the leading 0 digit in front of the hour...
  // Until now I've seen only it in spam stuff so let's ignore it (it's even good to catch spam).

  // Another set of horrors I've seen is

  // Sun, 13 Apr 2008 14:05:54 +0100 (GMT+01:00)
  // Sun, 15 Feb 2009 22:48:51 -0800 (Pacific Standard Time)
  // Fri, 13 Mar 2009 07:33:20 -0600 (Mountain Daylight Time)

  // We again ignore it as it's very rare.

  kDebug() << "Unrecognized date format [" << aDateStr.data() << "]: fix this function if you think it's correct :)";
  setDate( extractedDate.toTime_t() );
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
  QString str = aStr;

  while (str[0] == ' ') str.remove(0,1);
  if (hasKeyword) *hasKeyword=false;

  unsigned int i = 0, maxChars = 3;
  unsigned int strLength(str.length());
  for (i=0; i < strLength && i < maxChars; i++)
  {
    if (str[i] < 'A' || str[i] == sepChar) break;
  }

  if (str[i] == sepChar) // skip following spaces too
  {
    do {
      i++;
    } while (str[i] == ' ');
    if (hasKeyword) *hasKeyword=true;
    return str.mid(i);
  }
  return str;
}


//-----------------------------------------------------------------------------
QStringList KMMsgBase::supportedEncodings(bool usAscii)
{
  // cberzan: replaced by KCodecAction in CodecManager
  QStringList encodingNames = KGlobal::charsets()->availableEncodingNames();
  QStringList encodings;
  QMap<QString,bool> mimeNames;
  for (QStringList::Iterator it = encodingNames.begin();
    it != encodingNames.end(); ++it)
  {
    bool ok;
    QTextCodec *codec = KGlobal::charsets()->codecForName( *it, ok );
    kDebug() << "name" << *it << "codec" << codec << "name" << (codec ? codec->name() : "NULL");
    QString mimeName = codec ? QString( codec->name() ).toLower() : *it;
    if ( ok && !mimeNames.contains( mimeName ) )
    {
      encodings.append( KGlobal::charsets()->descriptionForEncoding( *it ) );
      mimeNames.insert( mimeName, true );
      kDebug() << "added" << mimeName;
    }
  }
  encodings.sort();
  if (usAscii)
    encodings.prepend(KGlobal::charsets()->descriptionForEncoding("us-ascii") );
  return encodings;
}

//-----------------------------------------------------------------------------
QString KMMsgBase::fixEncoding( const QString &encoding )
{
  QString returnEncoding = encoding;
  // According to http://www.iana.org/assignments/character-sets, uppercase is
  // preferred in MIME headers
  if ( returnEncoding.toUpper().contains( "ISO " ) ) {
    returnEncoding = returnEncoding.toUpper();
    returnEncoding.replace( "ISO ", "ISO-" );
  }
  return returnEncoding;
}

//-----------------------------------------------------------------------------
QString KMMsgBase::encodingForName( const QString &descriptiveName )
{
  QString encoding = KGlobal::charsets()->encodingForName( descriptiveName );
  return KMMsgBase::fixEncoding( encoding );
}

//-----------------------------------------------------------------------------
QByteArray fallbackCharsetForRFC2047Decoding( const QByteArray &prefCharset )
{
  QByteArray charsetName;
  if ( !prefCharset.isEmpty() ) {
    if ( kasciistricmp( prefCharset.data(), "us-ascii" ) == 0 ) {
      charsetName = "utf-8";
    } else {
      charsetName = prefCharset;
    }
  } else {
    charsetName = GlobalSettings::self()->fallbackCharacterEncoding().toLatin1();
  }

  const QTextCodec *codec = KMMsgBase::codecForName( charsetName );
  if ( ! codec ) {
    codec = kmkernel->networkCodec();
  }
  return codec->name();
}

//-----------------------------------------------------------------------------
QString KMMsgBase::decodeRFC2047String( const QByteArray &aStr,
                                        const QByteArray &prefCharset )
{
  QByteArray str = KMime::unfoldHeader( aStr );
  QByteArray usedCharset;
  QByteArray fallback = fallbackCharsetForRFC2047Decoding( prefCharset );

  if ( str.indexOf( "=?" ) < 0 ) {
    const QTextCodec *codec = KMMsgBase::codecForName( fallback );
    if ( !codec ) {
      codec = kmkernel->networkCodec();
    }
    return codec->toUnicode( str );
  }

  return KMime::decodeRFC2047String( str, usedCharset, fallback );
}

//-----------------------------------------------------------------------------
static const QByteArray especials = "()<>@,;:\"/[]?.= \033";

QByteArray KMMsgBase::encodeRFC2047Quoted( const QByteArray & s, bool base64 ) {
  const char * codecName = base64 ? "b" : "q" ;
  const KMime::Codec * codec = KMime::Codec::codecForName( codecName );
  if ( !codec ) {
    kFatal() << "No \"" << codecName <<"\" found!?";
  }
  QByteArray in = QByteArray::fromRawData( s.data(), s.length() );
  const QByteArray result = codec->encode( in );
  in.clear();
  return QByteArray( result.data(), result.size());
}

QByteArray KMMsgBase::encodeRFC2047String( const QString& _str,
                                           const QByteArray& charset )
{
  static const QString dontQuote = "\"()<>,@";

  if (_str.isEmpty()) return QByteArray();
  if (charset == "us-ascii") return toUsAscii(_str);

  QByteArray cset;
  if (charset.isEmpty())
  {
    cset = kmkernel->networkCodec()->name();
    kAsciiToLower(cset.data());
  }
  else cset = charset;

  const QTextCodec *codec = codecForName(cset);
  if (!codec) codec = kmkernel->networkCodec();

  unsigned int nonAscii = 0;
  unsigned int strLength(_str.length());
  for (unsigned int i = 0; i < strLength; i++)
    if (_str.at(i).unicode() >= 128) nonAscii++;
  bool useBase64 = (nonAscii * 6 > strLength);

  unsigned int start, stop, p, pos = 0, encLength;
  QByteArray result;
  bool breakLine = false;
  const unsigned int maxLen = 75 - 7 - cset.length();

  while (pos < strLength)
  {
    start = pos; p = pos;
    while (p < strLength)
    {
      if (!breakLine && (_str.at(p) == ' ' || dontQuote.contains(_str.at(p)) ))
        start = p + 1;
      if (_str.at(p).unicode() >= 128 || _str.at(p).unicode() < 32)
        break;
      p++;
    }
    if (breakLine || p < strLength)
    {
      while (dontQuote.contains(_str.at(start)) ) start++;
      stop = start;
      while (stop < strLength && !dontQuote.contains(_str.at(stop)) )
        stop++;
      result += _str.mid(pos, start - pos).toLatin1();
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
      p = (stop >= strLength ? strLength - 1 : stop);
      while (p > start && _str.at(p) != ' ') p--;
      if (p > start) stop = p;
      if (result.right(3) == "?= ") start--;
      if (result.right(5) == "?=\n  ") {
        start--; result.truncate(result.length() - 1);
      }
      int lastNewLine = result.lastIndexOf("\n ");
      if (!result.mid(lastNewLine).trimmed().isEmpty()
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
      result += _str.mid(pos).toLatin1();
      break;
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
QByteArray KMMsgBase::encodeRFC2231String( const QString& _str,
                                         const QByteArray& charset )
{
  if ( _str.isEmpty() )
    return QByteArray();

  QByteArray cset;
  if ( charset.isEmpty() )
  {
    cset = kmkernel->networkCodec()->name();
    kAsciiToLower( cset.data() );
  }
  else
    cset = charset;
  const QTextCodec *codec = codecForName( cset );
  QByteArray latin;
  if ( charset == "us-ascii" )
    latin = toUsAscii( _str );
  else if ( codec )
    latin = codec->fromUnicode( _str );
  else
    latin = _str.toLocal8Bit();

  char *l;
  for ( l = latin.data(); *l; ++l ) {
    if ( ( ( *l & 0xE0 ) == 0 ) || ( *l & 0x80 ) )
      // *l is control character or 8-bit char
      break;
  }
  if ( !*l )
    return latin;

  QByteArray result = cset + "''";
  for ( l = latin.data(); *l; ++l ) {
    bool needsQuoting = ( *l & 0x80 ) || ( *l == '%' );
    if( !needsQuoting ) {
      int len = especials.length();
      for ( int i = 0; i < len; i++ )
        if ( *l == especials[i] ) {
          needsQuoting = true;
          break;
        }
    }
    if ( needsQuoting ) {
      result += '%';
      unsigned char hexcode;
      hexcode = ( ( *l & 0xF0 ) >> 4 ) + 48;
      if ( hexcode >= 58 )
        hexcode += 7;
      result += hexcode;
      hexcode = ( *l & 0x0F ) + 48;
      if ( hexcode >= 58 )
        hexcode += 7;
      result += hexcode;
    } else {
      result += *l;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
QString KMMsgBase::decodeRFC2231String(const QByteArray& _str)
{
  int p = _str.indexOf('\'');
  if (p < 0) return kmkernel->networkCodec()->toUnicode(_str);

  QByteArray charset = _str.left(p);

  QByteArray st = _str.mid(_str.lastIndexOf('\'') + 1);
  char ch, ch2;
  p = 0;
  while (p < (int)st.length())
  {
    if (st.at(p) == 37)
    {
      // Only try to decode the percent-encoded character if the percent sign
      // is really followed by two other characters, see testcase at bug 163024
      if ( p + 2 < st.length() ) {
        ch = st.at(p+1) - 48;
        if (ch > 16)
          ch -= 7;
        ch2 = st.at(p+2) - 48;
        if (ch2 > 16)
          ch2 -= 7;
        st[p] = ch * 16 + ch2;
        st.remove( p+1, 2 );
      }
    }
    p++;
  }
  QString result;
  const QTextCodec * codec = codecForName( charset );
  if ( !codec )
    codec = kmkernel->networkCodec();
  return codec->toUnicode( st );
}

QByteArray KMMsgBase::extractRFC2231HeaderField( const QByteArray &aStr,
                                                 const QByteArray &field )
{
  int n = -1;
  QByteArray str;
  bool found = false;
  while ( n <= 0 || found ) {
    QString pattern( field );
    // match a literal * after the fieldname, as defined by RFC2231
    pattern += "[*]";
    if ( n >= 0 ) {
      // If n<0, check for fieldname*=..., otherwise for fieldname*n=
      pattern += QString::number( n ) + "[*]?";
    }
    pattern += '=';

    QRegExp fnamePart( pattern, Qt::CaseInsensitive );
    int startPart = fnamePart.indexIn( aStr );
    int endPart;
    found = ( startPart >= 0 );
    if ( found ) {
      startPart += fnamePart.matchedLength();
      // handle empty field:
      if ( startPart == aStr.length() )
        return str;
      assert( startPart < aStr.length() );
      // Quoted values end at the ending quote
      if ( startPart < aStr.size() && aStr[startPart] == '"' ) {
        startPart++; // the double quote isn't part of the filename
        endPart = aStr.indexOf( '"', startPart ) - 1;
      } else {
        endPart = aStr.indexOf( ';', startPart ) - 1;
      }
      if ( endPart < 0 ) {
        endPart = 32767;
      }
      str += aStr.mid( startPart, endPart - startPart + 1 ).trimmed();
    }
    n++;
  }
  return str;
}

QString KMMsgBase::base64EncodedMD5( const QString & s, bool utf8 ) {
  if (s.trimmed().isEmpty()) return "";
  if ( utf8 )
    return base64EncodedMD5( s.trimmed().toUtf8() ); // QCString overload
  else
    return base64EncodedMD5( s.trimmed().toLatin1() ); // const char * overload
}

QString KMMsgBase::base64EncodedMD5( const QByteArray & s ) {
  if (s.trimmed().isEmpty()) return "";
  return base64EncodedMD5( s.trimmed().data() );
}

QString KMMsgBase::base64EncodedMD5( const char * s, int len ) {
  if (!s || !len) return "";
  static const int Base64EncodedMD5Len = 22;
  KMD5 md5( s, len );
  return md5.base64Digest().left( Base64EncodedMD5Len );
}


//-----------------------------------------------------------------------------
unsigned long KMMsgBase::getMsgSerNum() const
{
#if 0 //TODO port to akonadi
  unsigned long msn = MessageProperty::serialCache( this );
  if (msn)
    return msn;
  if (mParent) {
    int index = mParent->find((KMMsgBase*)this);
    msn = KMMsgDict::instance()->getMsgSerNum(mParent, index);
    if (msn)
      MessageProperty::setSerialCache( this, msn );
  }
  return msn;
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}


//-----------------------------------------------------------------------------
KMMsgAttachmentState KMMsgBase::attachmentState() const
{
  if ( mStatus.hasAttachment() )
    return KMMsgHasAttachment;
// FIXME
//  else if (st & KMMsgStatusHasNoAttach)
//    return KMMsgHasNoAttachment;
  else
    return KMMsgAttachmentUnknown;
}

//-----------------------------------------------------------------------------
static void swapEndian( QString &str )
{
  QChar *data = str.data();
  while ( !data->isNull() ) {
    *data = kmail_swap_16( data->unicode() );
    data++;
  }
}

//-----------------------------------------------------------------------------
static int g_chunk_length = 0, g_chunk_offset=0;
static uchar *g_chunk = 0;

namespace {
  template < typename T > void copy_from_stream( T & x ) {
    if( g_chunk_offset + int(sizeof(T)) > g_chunk_length ) {
      g_chunk_offset = g_chunk_length;
      kDebug() << "This should never happen..";
      x = 0;
    } else {
      // the memcpy is optimized out by the compiler for the values
      // of sizeof(T) that is called with
      memcpy( &x, g_chunk + g_chunk_offset, sizeof(T) );
      g_chunk_offset += sizeof(T);
    }
  }
}

//-----------------------------------------------------------------------------
QString KMMsgBase::getStringPart(MsgPartType t) const
{
  if ( !mStringPartCacheBuilt )
    fillStringPartCache();
  return mCachedStringParts[t];
}

//-----------------------------------------------------------------------------
void KMMsgBase::fillStringPartCache() const
{
retry:
#ifdef KMAIL_SQLITE_INDEX
  bool swapByteOrder = false;
  g_chunk = (uchar*)mData;
  g_chunk_length = mIndexLength;
#else
  bool using_mmap = false;
  bool swapByteOrder = storage()->indexSwapByteOrder();
  if (storage()->indexStreamBasePtr()) {
    if (g_chunk)
      free(g_chunk);
    using_mmap = true;
    if ( mIndexOffset > (off_t)storage()->indexStreamLength() ) {
      // This message has not been indexed yet, data would lie
      // outside the index data structures so do not touch it.
      return;
    }
    g_chunk = storage()->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if(!storage()->mIndexStream)
      return;
    if (g_chunk_length < mIndexLength)
      g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    off_t first_off = KDE_ftell(storage()->mIndexStream);
    KDE_fseek(storage()->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, storage()->mIndexStream);
    KDE_fseek(storage()->mIndexStream, first_off, SEEK_SET);
  }
#endif // !KMAIL_SQLITE_INDEX

  MsgPartType type;
  quint16 len;
  for ( g_chunk_offset = 0; g_chunk_offset < mIndexLength; g_chunk_offset += len ) {
    quint32 tmp;
    copy_from_stream(tmp);
    copy_from_stream(len);
    if (swapByteOrder)
    {
       tmp = kmail_swap_32(tmp);
       len = kmail_swap_16(len);
    }
    type = (MsgPartType) tmp;
    if( g_chunk_offset + len > mIndexLength ) {
      kWarning() << "This should never happen..";
#ifndef KMAIL_SQLITE_INDEX
      if( using_mmap ) {
        g_chunk_length = 0;
        g_chunk = 0;
      }
#endif
      if ( !storage()->recreateIndex() )
        return;
      goto retry;
    }

    // Only try to create strings if the part is really a string part, see declaration of
    // MsgPartType
    if ( len && ( ( type >= 1 && type <= 6 ) || type == 11 || type == 14 || type == 15 || type == 19 ) ) {

      // This works because the QString constructor does a memcpy.
      // Otherwise we would need to be concerned about the alignment.
      mCachedStringParts[type] = QString((QChar *)(g_chunk + g_chunk_offset), len/2);

      // Normally we need to swap the byte order because the QStrings are written
      // in the style of Qt2 (MSB -> network ordered).
      // QStrings in Qt3 expect host ordering.
      // On e.g. Intel host ordering is LSB, on e.g. Sparc it is MSB.

#     if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
      // Byte order is little endian (swap is true)
      swapEndian( mCachedStringParts[type] );
#     else
      // Byte order is big endian (swap is false)
#     endif
    }
  } //for
#ifndef KMAIL_SQLITE_INDEX
  if(using_mmap) {
      g_chunk_length = 0;
      g_chunk = 0;
  }
#endif

  mStringPartCacheBuilt = true;
}

//-----------------------------------------------------------------------------
off_t KMMsgBase::getLongPart(MsgPartType t) const
{
retry:
  off_t ret = 0;

#ifdef KMAIL_SQLITE_INDEX
  //todo reenable
  bool swapByteOrder = false;
  int sizeOfLong = sizeof(long);
  g_chunk = (uchar*)mData;
  g_chunk_length = mIndexLength;
#else
  bool using_mmap = false;
  bool swapByteOrder = storage()->indexSwapByteOrder();
  int sizeOfLong = storage()->indexSizeOfLong();
  if (storage()->indexStreamBasePtr()) {
    if (g_chunk)
      free(g_chunk);
    using_mmap = true;
    g_chunk = storage()->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if (!storage()->mIndexStream)
      return ret;
    assert(mIndexLength >= 0);
    if (g_chunk_length < mIndexLength)
      g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    off_t first_off = KDE_ftell(storage()->mIndexStream);
    KDE_fseek(storage()->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, storage()->mIndexStream);
    KDE_fseek(storage()->mIndexStream, first_off, SEEK_SET);
  }
#endif // !KMAIL_SQLITE_INDEX

  quint16 len;
  for ( g_chunk_offset = 0; g_chunk_offset < mIndexLength; g_chunk_offset += len ) {
    quint32 tmp;
    copy_from_stream(tmp);
    copy_from_stream(len);
    if (swapByteOrder)
    {
       tmp = kmail_swap_32(tmp);
       len = kmail_swap_16(len);
    }
    MsgPartType type = (MsgPartType) tmp;

    if (g_chunk_offset + len > mIndexLength) {
      kDebug() << "This should never happen..";
#ifndef KMAIL_SQLITE_INDEX
      if(using_mmap) {
        g_chunk_length = 0;
        g_chunk = 0;
      }
#endif
      if (!storage()->recreateIndex())
        return 0;
      goto retry;
    }
    if(type == t) {
      assert(sizeOfLong == len);
      if (sizeOfLong == sizeof(ret))
      {
        copy_from_stream(ret);
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
         quint32 ret_32;
         copy_from_stream(ret_32);
         if (swapByteOrder)
            ret_32 = kmail_swap_32(ret_32);
         ret = ret_32;
      }
      else if (sizeOfLong == 8)
      {
         // Long is stored as 8 bytes in index file, sizeof(long) = 4
         quint32 ret_1;
         quint32 ret_2;
         copy_from_stream(ret_1);
         copy_from_stream(ret_2);
         if (!swapByteOrder)
         {
            // Index file order is the same as the order of this CPU.
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
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
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
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
  } // for
#ifndef KMAIL_SQLITE_INDEX
  if(using_mmap) {
    g_chunk_length = 0;
    g_chunk = 0;
  }
#endif

  return ret;
}

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
// We need to use swab to swap bytes to network byte order
#define memcpy_networkorder(to, from, len)  swab((char *)(from), (char *)(to), len)
#else
// We're already in network byte order
#define memcpy_networkorder(to, from, len)  memcpy(to, from, len)
#endif

#define STORE_DATA_LEN(type, x, len, network_order) \
  do { \
    int len2 = (len > 256) ? 256 : len; \
    if(csize < (length + (len2 + sizeof(short) + sizeof(MsgPartType)))) \
      ret = (uchar *)realloc(ret, csize += len2+sizeof(short)+sizeof(MsgPartType)); \
    quint32 t = (quint32) type; memcpy(ret+length, &t, sizeof(t)); \
    quint16 l = len2; memcpy(ret+length+sizeof(t), &l, sizeof(l)); \
    if (network_order) \
       memcpy_networkorder(ret+length+sizeof(t)+sizeof(l), x, len2); \
    else \
       memcpy(ret+length+sizeof(t)+sizeof(l), x, len2); \
    length += len2+sizeof(t)+sizeof(l); \
  } while(0)

#define STORE_DATA(type, x) STORE_DATA_LEN(type, &x, sizeof(x), false)

//-----------------------------------------------------------------------------
const uchar *KMMsgBase::asIndexString(int &length) const
{
  unsigned int csize = 256;
  static uchar *ret = 0; //different static buffer here for we may use the other buffer in the functions below
  if(!ret) {
    ret = (uchar *)malloc(csize);
  }
  length = 0;

  unsigned long tmp;
  QString tmp_str;

  //these is at the beginning because it is queried quite often
  tmp_str = msgIdMD5().trimmed();
  STORE_DATA_LEN(MsgIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp = 0;
  STORE_DATA(MsgLegacyStatusPart, tmp);

  //these are completely arbitrary order
  tmp_str = fromStrip().trimmed();
  STORE_DATA_LEN(MsgFromPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = tagString().trimmed();
  STORE_DATA_LEN(MsgTagPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = subject().trimmed();
  STORE_DATA_LEN(MsgSubjectPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = toStrip().trimmed();
  STORE_DATA_LEN(MsgToPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = replyToIdMD5().trimmed();
  STORE_DATA_LEN(MsgReplyToIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = xmark().trimmed();
  STORE_DATA_LEN(MsgXMarkPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = fileName().trimmed();
  STORE_DATA_LEN(MsgFilePart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp = msgSize();
  STORE_DATA(MsgSizePart, tmp);
  tmp = folderOffset();
  STORE_DATA(MsgOffsetPart, tmp);
  tmp = date();
  STORE_DATA(MsgDatePart, tmp);
  tmp = (signatureState() << 16) | encryptionState();
  STORE_DATA(MsgCryptoStatePart, tmp);
  tmp = mdnSentState();
  STORE_DATA(MsgMDNSentPart, tmp);

  tmp_str = replyToAuxIdMD5().trimmed();
  STORE_DATA_LEN(MsgReplyToAuxIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);

  tmp_str = strippedSubjectMD5().trimmed();
  STORE_DATA_LEN(MsgStrippedSubjectMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);

  tmp = mStatus.toQInt32();
  STORE_DATA(MsgStatusPart, tmp);

  tmp = msgSizeServer();
  STORE_DATA(MsgSizeServerPart, tmp);
  tmp = UID();
  STORE_DATA(MsgUIDPart, tmp);

  return ret;
}
#undef STORE_DATA_LEN
#undef STORE_DATA

#ifndef KMAIL_SQLITE_INDEX
bool KMMsgBase::syncIndexString() const
{
  if(!dirty())
    return true;
  int len;
  const uchar *buffer = asIndexString(len);
  if (len != mIndexLength)
    return false;

  Q_ASSERT(storage()->mIndexStream);
  KDE_fseek(storage()->mIndexStream, mIndexOffset, SEEK_SET);
  assert( mIndexOffset > 0 );
  if ( fwrite( buffer, len, 1, storage()->mIndexStream) != 1 )
    return false;
  return true;
}
#endif

static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
static bool sReplaceSubjPrefix, sReplaceForwSubjPrefix;

//-----------------------------------------------------------------------------
void KMMsgBase::readConfig()
{
  KConfigGroup composerGroup( KMKernel::config(), "Composer" );
  sReplySubjPrefixes = composerGroup.readEntry("reply-prefixes", QStringList());
  if (sReplySubjPrefixes.isEmpty())
    sReplySubjPrefixes << "Re\\s*:" << "Re\\[\\d+\\]:" << "Re\\d+:";
  sReplaceSubjPrefix =
      composerGroup.readEntry( "replace-reply-prefix", true );
  sForwardSubjPrefixes = composerGroup.readEntry("forward-prefixes", QStringList());
  if (sForwardSubjPrefixes.isEmpty())
    sForwardSubjPrefixes << "Fwd:" << "FW:";
  sReplaceForwSubjPrefix =
      composerGroup.readEntry( "replace-forward-prefix", true );
}

//-----------------------------------------------------------------------------
// static
QString KMMsgBase::stripOffPrefixes( const QString& str )
{
  return replacePrefixes( str, sReplySubjPrefixes + sForwardSubjPrefixes,
                          true, QString() ).trimmed();
}

//-----------------------------------------------------------------------------
// static
QString KMMsgBase::replacePrefixes( const QString& str,
                                    const QStringList& prefixRegExps,
                                    bool replace,
                                    const QString& newPrefix )
{
  bool recognized = false;
  // construct a big regexp that
  // 1. is anchored to the beginning of str (sans whitespace)
  // 2. matches at least one of the part regexps in prefixRegExps
  QString bigRegExp = QString::fromLatin1("^(?:\\s+|(?:%1))+\\s*")
                      .arg( prefixRegExps.join(")|(?:") );
  QRegExp rx( bigRegExp, Qt::CaseInsensitive );
  if ( !rx.isValid() ) {
    kWarning() << "bigRegExp = \""
                   << bigRegExp << "\"\n"
                   << "prefix regexp is invalid!";
    // try good ole Re/Fwd:
    recognized = str.startsWith( newPrefix );
  } else { // valid rx
    QString tmp = str;
    if ( rx.indexIn( tmp ) == 0 ) {
      recognized = true;
      if ( replace )
        return tmp.replace( 0, rx.matchedLength(), newPrefix + ' ' );
    }
  }
  if ( !recognized )
    return newPrefix + ' ' + str;
  else
    return str;
}

void KMMsgBase::setTagList( const QString &aTagStr )
{
  setTagList( KMMessageTagList::split( ",", aTagStr ) );
}

void KMMsgBase::setTagList( const KMMessageTagList &aTagList )
{
  if ( !mTagList ) {
    mTagList = new KMMessageTagList( aTagList );
  } else {
    if ( aTagList == *mTagList )
      return;
    *mTagList = aTagList;
  }
  mTagList->prioritySort();

  mDirty = true;

  if (storage())
  {
    int idx = storage()->find( this );

    if( mParent )
      mParent->msgTagListChanged( idx );
  }
}

//-----------------------------------------------------------------------------
QString KMMsgBase::cleanSubject() const
{
  return cleanSubject( sReplySubjPrefixes + sForwardSubjPrefixes,
		       true, QString() ).trimmed();
}

//-----------------------------------------------------------------------------
QString KMMsgBase::cleanSubject( const QStringList & prefixRegExps,
                                 bool replace,
                                 const QString & newPrefix ) const
{
  return KMMsgBase::replacePrefixes( subject(), prefixRegExps, replace,
                                     newPrefix );
}

//-----------------------------------------------------------------------------
QString KMMsgBase::forwardSubject() const {
  return cleanSubject( sForwardSubjPrefixes, sReplaceForwSubjPrefix, "Fwd:" );
}

//-----------------------------------------------------------------------------
QString KMMsgBase::replySubject() const {
  return cleanSubject( sReplySubjPrefixes, sReplaceSubjPrefix, "Re:" );
}
