// kmmsgpart.cpp

#include "kmmsgpart.h"
#include "kmkernel.h"
#include "kmmessage.h"
#include "globalsettings.h"

#include <kmime/kmime_charfreq.h>
#include <kmime/kmime_codecs.h>
#include <mimelib/enum.h>
#include <mimelib/utility.h>
#include <mimelib/string.h>

#include <kmimetype.h>
#include <kdebug.h>
#include <kcodecs.h>
#include <kascii.h>
#include <kiconloader.h>

#include <QTextCodec>
#include <QList>

#include <assert.h>

using namespace KMime;

//-----------------------------------------------------------------------------
KMMessagePart::KMMessagePart()
  : mType("text"), mSubtype("plain"), mCte("7bit"), mBodyDecodedSize(0),
    mParent(0), mLoadHeaders(false), mLoadPart(false)
{
}

//-----------------------------------------------------------------------------
KMMessagePart::KMMessagePart( QDataStream & stream )
  : mParent(0), mLoadHeaders(false), mLoadPart(false)
{
  int size;
  stream >> mOriginalContentTypeStr >> mName >> mContentDescription
    >> mContentDisposition >> mCte >> size >> mPartSpecifier;

  kAsciiToLower( mContentDisposition.data() );
  kAsciiToUpper( mOriginalContentTypeStr.data() );

  // set the type
  int sep = mOriginalContentTypeStr.indexOf('/');
  mType = mOriginalContentTypeStr.left(sep);
  mSubtype = mOriginalContentTypeStr.mid(sep+1);

  mBodyDecodedSize = size;
}


//-----------------------------------------------------------------------------
KMMessagePart::~KMMessagePart()
{
}


//-----------------------------------------------------------------------------
void KMMessagePart::clear()
{
  mOriginalContentTypeStr = QByteArray();
  mType = "text";
  mSubtype = "plain";
  mCte = "7bit";
  mContentDescription = QByteArray();
  mContentDisposition = QByteArray();
  mBody.truncate( 0 );
  mAdditionalCTypeParamStr = QByteArray();
  mName.clear();
  mParameterAttribute = QByteArray();
  mParameterValue.clear();
  mCharset = QByteArray();
  mPartSpecifier.clear();
  mBodyDecodedSize = 0;
  mParent = 0;
  mLoadHeaders = false;
  mLoadPart = false;
}


//-----------------------------------------------------------------------------
void KMMessagePart::duplicate( const KMMessagePart & msgPart )
{
  // copy the data of msgPart
  *this = msgPart;
  // detach the explicitly shared QByteArray
  mBody.detach();
}

//-----------------------------------------------------------------------------
int KMMessagePart::decodedSize() const
{
  if (mBodyDecodedSize < 0)
    mBodyDecodedSize = bodyDecodedBinary().size();
  return mBodyDecodedSize;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setBody(const QByteArray &aStr)
{
  mBody = aStr;

  int enc = cte();
  if (enc == DwMime::kCte7bit || enc == DwMime::kCte8bit || enc == DwMime::kCteBinary)
    mBodyDecodedSize = mBody.size();
  else
    mBodyDecodedSize = -1; // Can't know the decoded size
}

void KMMessagePart::setBodyFromUnicode( const QString & str ) {
  QByteArray encoding = KMMsgBase::autoDetectCharset( charset(), KMMessage::preferredCharsets(), str );
  if ( encoding.isEmpty() )
    encoding = "utf-8";
  const QTextCodec * codec = KMMsgBase::codecForName( encoding );
  assert( codec );
  QList<int> dummy;
  setCharset( encoding );
  setBodyAndGuessCte( codec->fromUnicode( str ), dummy, false /* no 8bit */ );
}

const QTextCodec * KMMessagePart::codec() const {
  const QTextCodec * c = KMMsgBase::codecForName( charset() );

  if ( !c ) {
    // Ok, no override and nothing in the message, let's use the fallback
    // the user configured
    c = KMMsgBase::codecForName( GlobalSettings::self()->fallbackCharacterEncoding().toLatin1() );
  }
  if ( !c )
    // no charset means us-ascii (RFC 2045), so using local encoding should
    // be okay
    c = kmkernel->networkCodec();
  assert( c );
  return c;
}

QString KMMessagePart::bodyToUnicode(const QTextCodec* codec) const {
  if ( !codec )
    // No codec was given, so try the charset in the mail
    codec = this->codec();
  assert( codec );

  return codec->toUnicode( bodyDecoded() );
}

void KMMessagePart::setCharset( const QByteArray & c ) {
  if ( type() != DwMime::kTypeText )
    kWarning(5006)
      << "KMMessagePart::setCharset(): trying to set a charset for a non-textual mimetype." << endl
      << "Fix this caller:" << endl
      << "====================================================================" << endl
      << kBacktrace( 5 ) << endl
      << "====================================================================";
  mCharset = c;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncoded(const QByteArray& aStr)
{
  // Qt4: QCString and QByteArray have been merged; this method can be cleaned up
  setBodyEncodedBinary( aStr );
}

void KMMessagePart::setBodyAndGuessCte(const QByteArray& aBuf,
				       QList<int> & allowedCte,
				       bool allow8Bit,
                                       bool willBeSigned )
{
  mBodyDecodedSize = aBuf.length();

  CharFreq cf( aBuf.data(), mBodyDecodedSize ); // save to pass null strings

  allowedCte = KMMessage::determineAllowedCtes( cf, allow8Bit, willBeSigned );

#ifndef NDEBUG
  DwString dwCte;
  DwCteEnumToStr(allowedCte[0], dwCte);
  kDebug(5006) <<"CharFreq returned" << cf.type() <<"/"
	    << cf.printableRatio() << "and I chose"
	    << dwCte.c_str();
#endif

  setCte( allowedCte[0] ); // choose best fitting
  setBodyEncoded( aBuf );
}

//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncodedBinary(const QByteArray& aStr)
{
  mBodyDecodedSize = aStr.size();
  if (aStr.isEmpty())
  {
    mBody.resize(0);
    return;
  }

  switch (cte())
  {
  case DwMime::kCteQuotedPrintable:
  case DwMime::kCteBase64:
    {
      Codec * codec = Codec::codecForName( cteStr() );
      assert( codec );
      // Nice: We can use the convenience function :-)
      mBody = codec->encode( aStr );
      // QP encoding does CRLF -> LF conversion, which can change the size after decoding again
      // and a size mismatch triggers an assert in various other methods
      mBodyDecodedSize = -1;
      break;
    }
  default:
    kWarning(5006) <<"setBodyEncodedBinary: unknown encoding '" << cteStr()
		    << "'. Assuming binary.";
    // fall through
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody = aStr;
    break;
  }
}

//-----------------------------------------------------------------------------
void KMMessagePart::setMessageBody( const QByteArray &aBuf )
{
  CharFreq cf( aBuf ); // it's safe to pass null arrays
  mBodyDecodedSize = aBuf.size();

  int cte;
  switch ( cf.type() ) {
  case CharFreq::SevenBitText:
  case CharFreq::SevenBitData:
    cte = DwMime::kCte7bit;
    break;
  case CharFreq::EightBitText:
  case CharFreq::EightBitData:
    cte = DwMime::kCte8bit;
    break;
  default:
    kWarning(5006) <<"Calling"
                   << "with something containing neither 7 nor 8 bit text!"
                   << "Fix this caller:" << kBacktrace();
    cte = 0;
  }
  setCte( cte );
  setBodyEncodedBinary( aBuf );
}

//-----------------------------------------------------------------------------
QByteArray KMMessagePart::bodyDecodedBinary() const
{
  if (mBody.isEmpty()) return QByteArray();
  QByteArray result;

  switch (cte())
  {
    case DwMime::kCte7bit:
    case DwMime::kCte8bit:
    case DwMime::kCteBinary:
      result = mBody;
      break;
    default:
      if ( const Codec * codec = Codec::codecForName( cteStr() ) )
        // Nice: we can use the convenience function :-)
        result = codec->decode( mBody );
      else {
        kWarning(5006) <<"bodyDecodedBinary: unknown encoding '" << cteStr()
                        << "'. Assuming binary.";
        result = mBody;
      }
  }

  assert( mBodyDecodedSize < 0 || mBodyDecodedSize == result.size() );
  if ( mBodyDecodedSize < 0 )
    mBodyDecodedSize = result.size(); // cache the decoded size.

  return result;
}

QByteArray KMMessagePart::bodyDecoded(void) const
{
  if (mBody.isEmpty()) return QByteArray("");

  QByteArray result = bodyDecodedBinary();

  //kWarning( qstrlen(result) != len, 5006 )
  //  << "KMMessagePart::bodyDecoded(): body is binary but used as text!";

  result = result.replace( "\r\n", "\n" ); // CRLF -> LF conversion

  return result;
}


//-----------------------------------------------------------------------------
void KMMessagePart::magicSetType(bool aAutoDecode)
{
  const QByteArray body = ( aAutoDecode ) ? bodyDecodedBinary() : mBody ;
  KMimeType::Ptr mime = KMimeType::findByContent( body );

  QString mimetype = mime->name();
  const int sep = mimetype.indexOf('/');
  mType = mimetype.left(sep).toLatin1();
  mSubtype = mimetype.mid(sep+1).toLatin1();
}


//-----------------------------------------------------------------------------
QString KMMessagePart::iconName() const
{
  QByteArray mimeType( mType + '/' + mSubtype );
  kAsciiToLower( mimeType.data() );

  QString fileName;
  KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
  if (mime) {
    fileName = mime->iconName();
  } else {
    kWarning(5006) <<"unknown mimetype" << mimeType;
  }

  if ( fileName.isEmpty() )
  {
    fileName = this->fileName();
    if ( fileName.isEmpty() ) fileName = this->name();
    if ( !fileName.isEmpty() )
    {
      fileName = KMimeType::findByPath( "/tmp/"+fileName, 0, true )->iconName();
    }
  }

  fileName =
    KIconLoader::global()->iconPath( fileName, KIconLoader::Desktop );
  return fileName;
}


//-----------------------------------------------------------------------------
int KMMessagePart::type() const {
  return DwTypeStrToEnum(DwString(mType));
}


//-----------------------------------------------------------------------------
void KMMessagePart::setType(int aType)
{
  DwString dwType;
  DwTypeEnumToStr(aType, dwType);
  mType = dwType.c_str();
}

//-----------------------------------------------------------------------------
int KMMessagePart::subtype() const {
  return DwSubtypeStrToEnum(DwString(mSubtype));
}


//-----------------------------------------------------------------------------
void KMMessagePart::setSubtype(int aSubtype)
{
  DwString dwSubtype;
  DwSubtypeEnumToStr(aSubtype, dwSubtype);
  mSubtype = dwSubtype.c_str();
}

//-----------------------------------------------------------------------------
QByteArray KMMessagePart::parameterAttribute(void) const
{
  return mParameterAttribute;
}

//-----------------------------------------------------------------------------
QString KMMessagePart::parameterValue(void) const
{
  return mParameterValue;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setParameter(const QByteArray &attribute,
                                 const QString &value)
{
  mParameterAttribute = attribute;
  mParameterValue = value;
}

//-----------------------------------------------------------------------------
QByteArray KMMessagePart::contentTransferEncodingStr(void) const
{
  return mCte;
}


//-----------------------------------------------------------------------------
int KMMessagePart::contentTransferEncoding(void) const
{
  return DwCteStrToEnum(DwString(mCte));
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncodingStr(const QByteArray &aStr)
{
    mCte = aStr;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncoding(int aCte)
{
  DwString dwCte;
  DwCteEnumToStr(aCte, dwCte);
  mCte = dwCte.c_str();

}


//-----------------------------------------------------------------------------
QString KMMessagePart::contentDescription( void ) const
{
  return KMMsgBase::decodeRFC2047String( mContentDescription, charset() );;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDescription( const QString &aStr )
{
  QByteArray encoding =
    KMMsgBase::autoDetectCharset( charset(),
                                  KMMessage::preferredCharsets(), aStr );
  if ( encoding.isEmpty() ) {
    encoding = "utf-8";
  }
  mContentDescription = KMMsgBase::encodeRFC2047String( aStr, encoding );
}


//-----------------------------------------------------------------------------
QString KMMessagePart::fileName( void ) const
{
  bool bRFC2231encoded = false;

  // search the start of the filename
  QString cd( mContentDisposition );
  int startOfFilename = cd.indexOf( "filename*=" );
  if ( startOfFilename >= 0 ) {
    bRFC2231encoded = true;
    startOfFilename += 10;
  } else {
    startOfFilename = cd.indexOf( "filename=" );
    if ( startOfFilename < 0 ) {
      return QString();
    }
    startOfFilename += 9;
  }

  // search the end of the filename
  int endOfFilename;
  if ( '"' == mContentDisposition[startOfFilename] ) {
    startOfFilename++; // the double quote isn't part of the filename
    endOfFilename = mContentDisposition.indexOf( '"', startOfFilename ) - 1;
  } else {
    endOfFilename = mContentDisposition.indexOf( ';', startOfFilename ) - 1;
  }
  if ( endOfFilename < 0 ) {
    endOfFilename = 32767;
  }

  const QByteArray str =
    mContentDisposition.mid( startOfFilename,
                             endOfFilename-startOfFilename + 1 ).trimmed();

  if ( bRFC2231encoded ) {
    return KMMsgBase::decodeRFC2231String( str );
  } else {
    return KMMsgBase::decodeRFC2047String( str, charset() );
  }
}



QByteArray KMMessagePart::body() const
{
  return mBody;
}

