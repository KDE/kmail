// kmmsgpart.cpp

#include <config.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kmdcodec.h>

#include "kmmsgpart.h"
#include "kmmessage.h"

#include <kmime_charfreq.h>
#include <kmime_codecs.h>
#include <mimelib/enum.h>
#include <mimelib/utility.h>
#include <mimelib/string.h>

#include <kiconloader.h>

#if QT_VERSION <= 0x030100
#include <qregexp.h>
#endif

#include <assert.h>

using namespace KMime;

//-----------------------------------------------------------------------------
KMMessagePart::KMMessagePart()
  : mType("text"), mSubtype("plain"), mCte("7bit"), mBodyDecodedSize(0)
{
}


//-----------------------------------------------------------------------------
KMMessagePart::~KMMessagePart()
{
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

//-----------------------------------------------------------------------------
// Returns Base64 encoded MD5 digest of a QString
QString KMMessagePart::encodeBase64(const QString& aStr)
{
  DwString dwResult, dwSrc;
  QString result;

  if (aStr.isEmpty())
    return QString();

  // Generate digest
  KMD5 context(aStr.latin1());

  dwSrc = DwString((const char*)context.rawDigest(), 16);
  DwEncodeBase64(dwSrc, dwResult);
  result = QString( dwResult.c_str() );
  result.truncate(22);
  return result;
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
				       bool allow8Bit )
{
  allowedCte.clear();

  mBodyDecodedSize = aBuf.size();

  CharFreq cf( aBuf ); // save to pass null arrays...

  switch ( cf.type() ) {
  case CharFreq::SevenBitText:
    allowedCte << DwMime::kCte7bit;
    if ( allow8Bit )
      allowedCte << DwMime::kCte8bit;
  case CharFreq::SevenBitData:
    if ( cf.printableRatio() > 5.0/6.0 ) {
      // let n the length of data and p the number of printable chars.
      // Then base64 \approx 4n/3; qp \approx p + 3(n-p)
      // => qp < base64 iff p > 5n/6.
      allowedCte << DwMime::kCteQp;
      allowedCte << DwMime::kCteBase64;
    } else {
      allowedCte << DwMime::kCteBase64;
      allowedCte << DwMime::kCteQp;
    }
    break;
  case CharFreq::EightBitText:
    if ( allow8Bit )
      allowedCte << DwMime::kCte8bit;
    if ( cf.printableRatio() > 5.0/6.0 ) {
      allowedCte << DwMime::kCteQp;
      allowedCte << DwMime::kCteBase64;
    } else {
      allowedCte << DwMime::kCteBase64;
      allowedCte << DwMime::kCteQp;
    }
    break;
  case CharFreq::EightBitData:
    allowedCte << DwMime::kCteBase64;
    break;
  case CharFreq::None:
  default:
      //just nothing
      break;
  }

  kdDebug() << "CharFreq returned " << cf.type() << "/"
	    << cf.printableRatio() << " and I chose "
	    << allowedCte[0] << endl;
  setCte( allowedCte[0] ); // choose best fitting
  setBodyEncodedBinary( aBuf );
}

void KMMessagePart::setBodyAndGuessCte(const QCString& aBuf,
				       QValueList<int> & allowedCte,
				       bool allow8Bit )
{
  allowedCte.clear();

  mBodyDecodedSize = aBuf.length();

  CharFreq cf( aBuf.data(), mBodyDecodedSize ); // save to pass null strings

  switch ( cf.type() ) {
  case CharFreq::SevenBitText:
    allowedCte << DwMime::kCte7bit;
    if ( allow8Bit )
      allowedCte << DwMime::kCte8bit;
  case CharFreq::SevenBitData:
    if ( cf.printableRatio() > 5.0/6.0 ) {
      // let n the length of data and p the number of printable chars.
      // Then base64 \approx 4n/3; qp \approx p + 3(n-p)
      // => qp < base64 iff p > 5n/6.
      allowedCte << DwMime::kCteQp;
      allowedCte << DwMime::kCteBase64;
    } else {
      allowedCte << DwMime::kCteBase64;
      allowedCte << DwMime::kCteQp;
    }
    break;
  case CharFreq::EightBitText:
    if ( allow8Bit )
      allowedCte << DwMime::kCte8bit;
    if ( cf.printableRatio() > 5.0/6.0 ) {
      allowedCte << DwMime::kCteQp;
      allowedCte << DwMime::kCteBase64;
    } else {
      allowedCte << DwMime::kCteBase64;
      allowedCte << DwMime::kCteQp;
    }
    break;
  case CharFreq::EightBitData:
    allowedCte << DwMime::kCteBase64;
    break;
  case CharFreq::None:
  default:
      //just nothing
      break;
  }

  kdDebug() << "CharFreq returned " << cf.type() << "/"
	    << cf.printableRatio() << " and I chose "
	    << allowedCte[0] << endl;
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

  switch (cte())
  {
  case DwMime::kCteQuotedPrintable:
  case DwMime::kCteBase64:
    {
      Codec * codec = Codec::codecForName( cteStr() );
      assert( codec );
      // Nice: we can use the convenience function :-)
      result = codec->decode( mBody );
      break;
    }
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

  switch (cte())
  {
  case DwMime::kCteQuotedPrintable:
  case DwMime::kCteBase64:
    {
      Codec * codec = Codec::codecForName( cteStr() );
      assert( codec );
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
#if QT_VERSION >= 0x030100
      result = result.replace( "\r\n", "\n" );
#else
      result = result.replace( QRegExp("\r\n"), "\n" );
#endif
      break;
    }
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

  assert( mBodyDecodedSize < 0 || mBodyDecodedSize == len );
  if ( mBodyDecodedSize < 0 )
    mBodyDecodedSize = len; // cache decoded size

  return result;
}


//-----------------------------------------------------------------------------
void KMMessagePart::magicSetType(bool aAutoDecode)
{
  QString mimetype;
  QByteArray body;
  KMimeMagicResult *result;

  int sep;

  KMimeMagic::self()->setFollowLinks(TRUE); // is it necessary ?

  if (aAutoDecode)
    body = bodyDecodedBinary();
  else
    body = mBody;

  result = KMimeMagic::self()->findBufferType( body );
  mimetype = result->mimeType();
  sep = mimetype.find('/');
  mType = mimetype.left(sep).latin1();
  mSubtype = mimetype.mid(sep+1).latin1();
}


//-----------------------------------------------------------------------------
QString KMMessagePart::iconName(const QString& mimeType) const
{
  QString fileName = KMimeType::mimeType(mimeType.isEmpty() ?
    (mType + "/" + mSubtype).lower() : mimeType.lower())->icon(QString(),FALSE);
  fileName = KGlobal::instance()->iconLoader()->iconPath( fileName,
    KIcon::Desktop );
  return fileName;
}


//-----------------------------------------------------------------------------
QCString KMMessagePart::typeStr(void) const
{
  return mType;
}


//-----------------------------------------------------------------------------
int KMMessagePart::type(void) const
{
  int type = DwTypeStrToEnum(DwString(mType));
  return type;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setTypeStr(const QCString &aStr)
{
    mType = aStr;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setType(int aType)
{
  DwString dwType;
  DwTypeEnumToStr(aType, dwType);
  mType = dwType.c_str();

}

//-----------------------------------------------------------------------------
QCString KMMessagePart::subtypeStr(void) const
{
  return mSubtype;
}


//-----------------------------------------------------------------------------
int KMMessagePart::subtype(void) const
{
  int subtype = DwSubtypeStrToEnum(DwString(mSubtype));
  return subtype;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setSubtypeStr(const QCString &aStr)
{
  mSubtype = aStr;
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
  int cte = DwCteStrToEnum(DwString(mCte));
  return cte;
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
  QCString encoding = KMMessage::autoDetectCharset(charset(),
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

  QCString str;
  str = mContentDisposition.mid(startOfFilename,
                                endOfFilename-startOfFilename+1)
                           .stripWhiteSpace();

  if (bRFC2231encoded)
    return KMMsgBase::decodeRFC2231String(str);
  else
    return KMMsgBase::decodeRFC2047String(str);
}


//-----------------------------------------------------------------------------
QCString KMMessagePart::contentDisposition(void) const
{
  return mContentDisposition;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDisposition(const QCString &aStr)
{
  mContentDisposition = aStr;
}


//-----------------------------------------------------------------------------
QCString KMMessagePart::body(void) const
{
  return QCString( mBody.data(), mBody.size() + 1 ); // space for trailing NUL
}


//-----------------------------------------------------------------------------
QString KMMessagePart::name(void) const
{
  return mName;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setName(const QString &aStr)
{
  mName = aStr;
}


//-----------------------------------------------------------------------------
QCString KMMessagePart::charset(void) const
{
   return mCharset;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setCharset(const QCString &aStr)
{
  mCharset=aStr;
}



