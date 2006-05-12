// kmmsgpart.cpp

#include <config.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kcodecs.h>

#include "kmmsgpart.h"
#include "kmkernel.h"
#include "kmmessage.h"
#include "globalsettings.h"

#include <kascii.h>
#include <kmime_charfreq.h>
#include <kmime_codecs.h>
#include <mimelib/enum.h>
#include <mimelib/utility.h>
#include <mimelib/string.h>

#include <kiconloader.h>
#include <QTextCodec>
//Added by qt3to4:
#include <QList>
#include <Q3CString>

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
  unsigned long size;
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
  mOriginalContentTypeStr = Q3CString();
  mType = "text";
  mSubtype = "plain";
  mCte = "7bit";
  mContentDescription = Q3CString();
  mContentDisposition = Q3CString();
  mBody.truncate( 0 );
  mAdditionalCTypeParamStr = Q3CString();
  mName.clear();
  mParameterAttribute = Q3CString();
  mParameterValue.clear();
  mCharset = Q3CString();
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
int KMMessagePart::decodedSize(void) const
{
  if (mBodyDecodedSize < 0)
    mBodyDecodedSize = bodyDecodedBinary().size();
  return mBodyDecodedSize;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setBody(const Q3CString &aStr)
{
  mBody = aStr;

  int enc = cte();
  if (enc == DwMime::kCte7bit || enc == DwMime::kCte8bit || enc == DwMime::kCteBinary)
    mBodyDecodedSize = mBody.size();
  else
    mBodyDecodedSize = -1; // Can't know the decoded size
}

void KMMessagePart::setBodyFromUnicode( const QString & str ) {
  Q3CString encoding = KMMsgBase::autoDetectCharset( charset(), KMMessage::preferredCharsets(), str );
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

void KMMessagePart::setCharset( const Q3CString & c ) {
  if ( type() != DwMime::kTypeText )
    kWarning()
      << "KMMessagePart::setCharset(): trying to set a charset for a non-textual mimetype." << endl
      << "Fix this caller:" << endl
      << "====================================================================" << endl
      << kBacktrace( 5 ) << endl
      << "====================================================================" << endl;
  mCharset = c;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncoded(const Q3CString& aStr)
{
  mBodyDecodedSize = aStr.length();

  switch (cte())
  {
  case DwMime::kCteQuotedPrintable:
  case DwMime::kCteBase64:
    {
      Codec * codec = Codec::codecForName( cteStr() );
      assert( codec );
      // we can't use the convenience function here, since aStr is not
      // a QByteArray...:
      mBody.resize( codec->maxEncodedSizeFor( mBodyDecodedSize ) );
      Q3CString::ConstIterator iit = aStr.data();
      Q3CString::ConstIterator iend = aStr.data() + mBodyDecodedSize;
      QByteArray::Iterator oit = mBody.begin();
      QByteArray::ConstIterator oend = mBody.end();
      if ( !codec->encode( iit, iend, oit, oend ) )
	kWarning(5006) << codec->name()
			<< " codec lies about it's maxEncodedSizeFor( "
			<< mBodyDecodedSize << " ). Result truncated!" << endl;
      mBody.truncate( oit - mBody.begin() );
      break;
    }
  default:
    kWarning(5006) << "setBodyEncoded: unknown encoding '" << cteStr()
		    << "'. Assuming binary." << endl;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody = QByteArray( aStr.data(), mBodyDecodedSize );
    break;
  }
}

void KMMessagePart::setBodyAndGuessCte(const QByteArray& aBuf,
				       QList<int> & allowedCte,
				       bool allow8Bit,
                                       bool willBeSigned )
{
  mBodyDecodedSize = aBuf.size();

  CharFreq cf( aBuf ); // save to pass null arrays...

  allowedCte = KMMessage::determineAllowedCtes( cf, allow8Bit, willBeSigned );

#ifndef NDEBUG
  DwString dwCte;
  DwCteEnumToStr(allowedCte[0], dwCte);
  kDebug(5006) << "CharFreq returned " << cf.type() << "/"
	    << cf.printableRatio() << " and I chose "
	    << dwCte.c_str() << endl;
#endif

  setCte( allowedCte[0] ); // choose best fitting
  setBodyEncodedBinary( aBuf );
}

void KMMessagePart::setBodyAndGuessCte(const Q3CString& aBuf,
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
  kDebug(5006) << "CharFreq returned " << cf.type() << "/"
	    << cf.printableRatio() << " and I chose "
	    << dwCte.c_str() << endl;
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
      break;
    }
  default:
    kWarning(5006) << "setBodyEncodedBinary: unknown encoding '" << cteStr()
		    << "'. Assuming binary." << endl;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody = aStr;
    break;
  }
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
        kWarning(5006) << "bodyDecodedBinary: unknown encoding '" << cteStr()
                        << "'. Assuming binary." << endl;
        result = mBody;
      }
  }

  assert( mBodyDecodedSize < 0 || mBodyDecodedSize == result.size() );
  if ( mBodyDecodedSize < 0 )
    mBodyDecodedSize = result.size(); // cache the decoded size.

  return result;
}

Q3CString KMMessagePart::bodyDecoded(void) const
{
  if (mBody.isEmpty()) return Q3CString("");
  bool decodeBinary = false;
  Q3CString result;
  int len;

  switch (cte())
  {
    case DwMime::kCte7bit:
    case DwMime::kCte8bit:
    case DwMime::kCteBinary:
    {
      decodeBinary = true;
      break;
    }
    default:
      if ( const Codec * codec = Codec::codecForName( cteStr() ) ) {
        // We can't use the codec convenience functions, since we must
        // return a QCString, not a QByteArray:
        int bufSize = codec->maxDecodedSizeFor( mBody.size() ) + 1; // trailing NUL
        result.resize( bufSize );
        QByteArray::ConstIterator iit = mBody.begin();
        Q3CString::Iterator oit = result.begin();
        Q3CString::ConstIterator oend = result.begin() + bufSize;
        if ( !codec->decode( iit, mBody.end(), oit, oend ) )
          kWarning(5006) << codec->name()
                          << " lies about it's maxDecodedSizeFor( "
                          << mBody.size() << " ). Result truncated!" << endl;
        len = oit - result.begin();
        result.truncate( len ); // adds trailing NUL
      } else {
        kWarning(5006) << "bodyDecoded: unknown encoding '" << cteStr()
                        << "'. Assuming binary." << endl;
        decodeBinary = true;
      }
  }

  if ( decodeBinary ) {
    len = mBody.size();
    result.resize( len+1 /* trailing NUL */ );
    memcpy(result.data(), mBody.data(), len);
    result[len] = 0;
  }

  kWarning( result.length() != len, 5006 )
    << "KMMessagePart::bodyDecoded(): body is binary but used as text!" << endl;

  result = result.replace( "\r\n", "\n" ); // CRLF -> LF conversion

  assert( mBodyDecodedSize < 0 || mBodyDecodedSize == len );
  if ( mBodyDecodedSize < 0 )
    mBodyDecodedSize = len; // cache decoded size

  return result;
}


//-----------------------------------------------------------------------------
void KMMessagePart::magicSetType(bool aAutoDecode)
{
  KMimeMagic::self()->setFollowLinks( true ); // is it necessary ?

  const QByteArray body = ( aAutoDecode ) ? bodyDecodedBinary() : mBody ;
  KMimeMagicResult * result = KMimeMagic::self()->findBufferType( body );

  QString mimetype = result->mimeType();
  const int sep = mimetype.indexOf('/');
  mType = mimetype.left(sep).toLatin1();
  mSubtype = mimetype.mid(sep+1).toLatin1();
}


//-----------------------------------------------------------------------------
QString KMMessagePart::iconName() const
{
  Q3CString mimeType( mType + '/' + mSubtype );
  kAsciiToLower( mimeType.data() );
  QString fileName =
    KMimeType::mimeType( mimeType )->icon( QString() );
  fileName =
    KGlobal::instance()->iconLoader()->iconPath( fileName, K3Icon::Desktop );
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
Q3CString KMMessagePart::parameterAttribute(void) const
{
  return mParameterAttribute;
}

//-----------------------------------------------------------------------------
QString KMMessagePart::parameterValue(void) const
{
  return mParameterValue;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setParameter(const Q3CString &attribute,
                                 const QString &value)
{
  mParameterAttribute = attribute;
  mParameterValue = value;
}

//-----------------------------------------------------------------------------
Q3CString KMMessagePart::contentTransferEncodingStr(void) const
{
  return mCte;
}


//-----------------------------------------------------------------------------
int KMMessagePart::contentTransferEncoding(void) const
{
  return DwCteStrToEnum(DwString(mCte));
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncodingStr(const Q3CString &aStr)
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
QString KMMessagePart::contentDescription(void) const
{
  return KMMsgBase::decodeRFC2047String(mContentDescription);
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDescription(const QString &aStr)
{
  Q3CString encoding = KMMsgBase::autoDetectCharset(charset(),
    KMMessage::preferredCharsets(), aStr);
  if (encoding.isEmpty()) encoding = "utf-8";
  mContentDescription = KMMsgBase::encodeRFC2047String(aStr, encoding);
}


//-----------------------------------------------------------------------------
QString KMMessagePart::fileName(void) const
{
  bool bRFC2231encoded = false;

  // search the start of the filename
  QString cd( mContentDisposition );
  int startOfFilename = cd.indexOf("filename*=");
  if (startOfFilename >= 0) {
    bRFC2231encoded = true;
    startOfFilename += 10;
  }
  else {
    startOfFilename = cd.indexOf("filename=");
    if (startOfFilename < 0)
      return QString();
    startOfFilename += 9;
  }

  // search the end of the filename
  int endOfFilename;
  if ( '"' == mContentDisposition[startOfFilename] ) {
    startOfFilename++; // the double quote isn't part of the filename
    endOfFilename = mContentDisposition.indexOf('"', startOfFilename) - 1;
  }
  else {
    endOfFilename = mContentDisposition.indexOf(';', startOfFilename) - 1;
  }
  if (endOfFilename < 0)
    endOfFilename = 32767;

  const Q3CString str = mContentDisposition.mid(startOfFilename,
                                endOfFilename-startOfFilename+1)
                           .trimmed();

  if (bRFC2231encoded)
    return KMMsgBase::decodeRFC2231String(str);
  else
    return KMMsgBase::decodeRFC2047String(str);
}



Q3CString KMMessagePart::body() const
{
  return Q3CString( mBody.data(), mBody.size() + 1 ); // space for trailing NUL
}

