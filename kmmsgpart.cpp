// kmmsgpart.cpp

#include <config.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kmdcodec.h>

#include "kmmsgpart.h"
#include "kmmessage.h"
#include "kmkernel.h"

#include <kmime_charfreq.h>
#include <kmime_codecs.h>
#include <mimelib/enum.h>
#include <mimelib/utility.h>
#include <mimelib/string.h>

#include <kiconloader.h>
#include <qtextcodec.h>

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

  mContentDisposition = mContentDisposition.lower();
  mOriginalContentTypeStr = mOriginalContentTypeStr.upper();

  // set the type
  int sep = mOriginalContentTypeStr.find('/');
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
  mOriginalContentTypeStr = QCString();
  mType = "text";
  mSubtype = "plain";
  mCte = "7bit";
  mContentDescription = QCString();
  mContentDisposition = QCString();
  mBody.truncate( 0 );
  mAdditionalCTypeParamStr = QCString();
  mName = QString::null;
  mParameterAttribute = QCString();
  mParameterValue = QString::null;
  mCharset = QCString();
  mPartSpecifier = QString::null;
  mBodyDecodedSize = 0;
  mParent = 0;
  mLoadHeaders = false;
  mLoadPart = false;
}


//-----------------------------------------------------------------------------
KMMessagePart & KMMessagePart::duplicate( const KMMessagePart & msgPart )
{
  // copy the data of msgPart
  *this = msgPart;
  // detach the explicitely shared QByteArray
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
void KMMessagePart::setBody(const QCString &aStr)
{
  mBody.duplicate( aStr.data(), aStr.length() );

  int enc = cte();
  if (enc == DwMime::kCte7bit || enc == DwMime::kCte8bit || enc == DwMime::kCteBinary)
    mBodyDecodedSize = mBody.size();
  else
    mBodyDecodedSize = -1; // Can't know the decoded size
}

void KMMessagePart::setBodyFromUnicode( const QString & str ) {
  QCString encoding = KMMsgBase::autoDetectCharset( charset(), KMMessage::preferredCharsets(), str );
  if ( encoding.isEmpty() )
    encoding = "utf-8";
  const QTextCodec * codec = KMMsgBase::codecForName( encoding );
  assert( codec );
  QValueList<int> dummy;
  setCharset( encoding );
  setBodyAndGuessCte( codec->fromUnicode( str ), dummy, false /* no 8bit */ );
}

const QTextCodec * KMMessagePart::codec() const {
  const QTextCodec * c = KMMsgBase::codecForName( charset() );
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

void KMMessagePart::setCharset( const QCString & c ) {
  kdWarning( type() != DwMime::kTypeText )
    << "KMMessagePart::setCharset(): trying to set a charset for a non-textual mimetype." << endl
    << "Fix this caller:" << endl
    << "====================================================================" << endl
    << kdBacktrace( 5 ) << endl
    << "====================================================================" << endl;
  mCharset = c;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncoded(const QCString& aStr)
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
      QCString::ConstIterator iit = aStr.data();
      QCString::ConstIterator iend = aStr.data() + mBodyDecodedSize;
      QByteArray::Iterator oit = mBody.begin();
      QByteArray::ConstIterator oend = mBody.end();
      if ( !codec->encode( iit, iend, oit, oend ) )
	kdWarning(5006) << codec->name()
			<< " codec lies about it's maxEncodedSizeFor( "
			<< mBodyDecodedSize << " ). Result truncated!" << endl;
      mBody.truncate( oit - mBody.begin() );
      break;
    }
  default:
    kdWarning(5006) << "setBodyEncoded: unknown encoding '" << cteStr()
		    << "'. Assuming binary." << endl;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody.duplicate( aStr.data(), mBodyDecodedSize );
    break;
  }
}

void KMMessagePart::setBodyAndGuessCte(const QByteArray& aBuf,
				       QValueList<int> & allowedCte,
				       bool allow8Bit,
                                       bool willBeSigned )
{
  mBodyDecodedSize = aBuf.size();

  CharFreq cf( aBuf ); // save to pass null arrays...

  allowedCte = KMMessage::determineAllowedCtes( cf, allow8Bit, willBeSigned );

#ifndef NDEBUG
  DwString dwCte;
  DwCteEnumToStr(allowedCte[0], dwCte);
  kdDebug(5006) << "CharFreq returned " << cf.type() << "/"
	    << cf.printableRatio() << " and I chose "
	    << dwCte.c_str() << endl;
#endif

  setCte( allowedCte[0] ); // choose best fitting
  setBodyEncodedBinary( aBuf );
}

void KMMessagePart::setBodyAndGuessCte(const QCString& aBuf,
				       QValueList<int> & allowedCte,
				       bool allow8Bit,
                                       bool willBeSigned )
{
  mBodyDecodedSize = aBuf.length();

  CharFreq cf( aBuf.data(), mBodyDecodedSize ); // save to pass null strings

  allowedCte = KMMessage::determineAllowedCtes( cf, allow8Bit, willBeSigned );

#ifndef NDEBUG
  DwString dwCte;
  DwCteEnumToStr(allowedCte[0], dwCte);
  kdDebug(5006) << "CharFreq returned " << cf.type() << "/"
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
    kdWarning(5006) << "setBodyEncodedBinary: unknown encoding '" << cteStr()
		    << "'. Assuming binary." << endl;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody.duplicate( aStr );
    break;
  }
}


//-----------------------------------------------------------------------------
QByteArray KMMessagePart::bodyDecodedBinary() const
{
  if (mBody.isEmpty()) return QByteArray();
  QByteArray result;

  if ( const Codec * codec = Codec::codecForName( cteStr() ) )
    // Nice: we can use the convenience function :-)
    result = codec->decode( mBody );
  else
    switch (cte())
    {
    default:
      kdWarning(5006) << "bodyDecodedBinary: unknown encoding '" << cteStr()
		      << "'. Assuming binary." << endl;
    case DwMime::kCte7bit:
    case DwMime::kCte8bit:
    case DwMime::kCteBinary:
      result.duplicate(mBody);
      break;
    }

  assert( mBodyDecodedSize < 0
	  || (unsigned int)mBodyDecodedSize == result.size() );
  if ( mBodyDecodedSize < 0 )
    mBodyDecodedSize = result.size(); // cache the decoded size.

  return result;
}

QCString KMMessagePart::bodyDecoded(void) const
{
  if (mBody.isEmpty()) return QCString("");
  QCString result;
  int len;

  if ( const Codec * codec = Codec::codecForName( cteStr() ) ) {
    // We can't use the codec convenience functions, since we must
    // return a QCString, not a QByteArray:
    int bufSize = codec->maxDecodedSizeFor( mBody.size() ) + 1; // trailing NUL
    result.resize( bufSize );
    QByteArray::ConstIterator iit = mBody.begin();
    QCString::Iterator oit = result.begin();
    QCString::ConstIterator oend = result.begin() + bufSize;
    if ( !codec->decode( iit, mBody.end(), oit, oend ) )
      kdWarning(5006) << codec->name()
		      << " lies about it's maxDecodedSizeFor( "
		      << mBody.size() << " ). Result truncated!" << endl;
    len = oit - result.begin();
    result.truncate( len ); // adds trailing NUL
  } else
    switch (cte())
    {
    default:
      kdWarning(5006) << "bodyDecoded: unknown encoding '" << cteStr()
		      << "'. Assuming binary." << endl;
    case DwMime::kCte7bit:
    case DwMime::kCte8bit:
    case DwMime::kCteBinary:
    {
      len = mBody.size();
      result.resize( len+1 /* trailing NUL */ );
      memcpy(result.data(), mBody.data(), len);
      result[len] = 0;
      break;
    }
  }
  kdWarning( result.length() != (unsigned int)len, 5006 )
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
  const int sep = mimetype.find('/');
  mType = mimetype.left(sep).latin1();
  mSubtype = mimetype.mid(sep+1).latin1();
}


//-----------------------------------------------------------------------------
QString KMMessagePart::iconName(const QString& mimeType) const
{
  QString fileName = KMimeType::mimeType(mimeType.isEmpty() ?
    (mType + "/" + mSubtype).lower() : mimeType.lower())->icon(QString::null,FALSE);
  fileName = KGlobal::instance()->iconLoader()->iconPath( fileName,
    KIcon::Desktop );
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
QCString KMMessagePart::parameterAttribute(void) const
{
  return mParameterAttribute;
}

//-----------------------------------------------------------------------------
QString KMMessagePart::parameterValue(void) const
{
  return mParameterValue;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setParameter(const QCString &attribute,
                                 const QString &value)
{
  mParameterAttribute = attribute;
  mParameterValue = value;
}

//-----------------------------------------------------------------------------
QCString KMMessagePart::contentTransferEncodingStr(void) const
{
  return mCte;
}


//-----------------------------------------------------------------------------
int KMMessagePart::contentTransferEncoding(void) const
{
  return DwCteStrToEnum(DwString(mCte));
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncodingStr(const QCString &aStr)
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
  QCString encoding = KMMsgBase::autoDetectCharset(charset(),
    KMMessage::preferredCharsets(), aStr);
  if (encoding.isEmpty()) encoding = "utf-8";
  mContentDescription = KMMsgBase::encodeRFC2047String(aStr, encoding);
}


//-----------------------------------------------------------------------------
QString KMMessagePart::fileName(void) const
{
  bool bRFC2231encoded = false;

  // search the start of the filename
  int startOfFilename = mContentDisposition.find("filename*=", 0, FALSE);
  if (startOfFilename >= 0) {
    bRFC2231encoded = true;
    startOfFilename += 10;
  }
  else {
    startOfFilename = mContentDisposition.find("filename=", 0, FALSE);
    if (startOfFilename < 0)
      return QString::null;
    startOfFilename += 9;
  }

  // search the end of the filename
  int endOfFilename;
  if ( '"' == mContentDisposition[startOfFilename] ) {
    startOfFilename++; // the double quote isn't part of the filename
    endOfFilename = mContentDisposition.find('"', startOfFilename) - 1;
  }
  else {
    endOfFilename = mContentDisposition.find(';', startOfFilename) - 1;
  }
  if (endOfFilename < 0)
    endOfFilename = 32767;

  const QCString str = mContentDisposition.mid(startOfFilename,
                                endOfFilename-startOfFilename+1)
                           .stripWhiteSpace();

  if (bRFC2231encoded)
    return KMMsgBase::decodeRFC2231String(str);
  else
    return KMMsgBase::decodeRFC2047String(str);
}



QCString KMMessagePart::body() const
{
  return QCString( mBody.data(), mBody.size() + 1 ); // space for trailing NUL
}

