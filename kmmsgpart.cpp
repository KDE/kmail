// kmmsgpart.cpp

#include <qdir.h>

#include <kmimemagic.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kiconloader.h>
#include <kdebug.h>

#include "kmmsgbase.h"
#include "kmmsgpart.h"
#include "kmmessage.h"

extern "C" {
#include "md5.h"
};

#include <mimelib/enum.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>
#include <mimelib/utility.h>

//-----------------------------------------------------------------------------
KMMessagePart::KMMessagePart() : 
  mType("text"), mSubtype("plain"), mCte("7bit"), mContentDescription(),
  mContentDisposition(), mBody(), mName()
{
  mBodySize = 0;
}


//-----------------------------------------------------------------------------
KMMessagePart::~KMMessagePart()
{
}


//-----------------------------------------------------------------------------
int KMMessagePart::size(void) const
{
  if (mBodySize < 0)
  {
    ((KMMessagePart*)this)->mBodySize = 
      bodyDecodedBinary().size();
  }
  return mBodySize;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setBody(const QString aStr)
// aStr may contain binary data ! See KMMessage::BodyPart()
// However here we assume aStr is not binary and append hex 00. FIXME
{
  int encoding = contentTransferEncoding();
  int len = aStr.length();

  mBody.truncate(len+1);
  memcpy(mBody.data(), (const char *)aStr, len);
  mBody[len] = 0;

  if (encoding!=DwMime::kCteQuotedPrintable &&
      encoding!=DwMime::kCteBase64)
  {
    mBodySize = mBody.size() - 1;
  }
  else mBodySize = -1;
}

//-----------------------------------------------------------------------------
// Returns Base64 encoded MD5 digest of a QString
QString KMMessagePart::encodeBase64(const QString& aStr)
{
  char *c = const_cast<char *>(aStr.data());
  unsigned char digest[16];
  Bin_MD5Context ctx;
  DwString dwResult, dwSrc;
  QString result;

  if (aStr.isEmpty())
    return QString();

  // Generate digest
  Bin_MD5Init(&ctx);
  Bin_MD5Update(&ctx,
		(unsigned char *)c,
		(unsigned)strlen(c));
  Bin_MD5Final(digest, &ctx);
  dwSrc = DwString((const char*)digest, 16);
  DwEncodeBase64(dwSrc, dwResult);
  result = QString( dwResult.c_str() );
  result.truncate(22);
  return result;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncoded(const QCString& aStr)
{
  QByteArray bBody(aStr);
  if (aStr.length()+1==aStr.size()) {
    // if this really is a text string:
    bBody.resize(bBody.size() - 1);
  } else {
    kdDebug() << "WARNING -- body is binary instead of text" << endl;
  }
  setBodyEncodedBinary(bBody);
}

//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncodedBinary(const QByteArray& aStr)
{
  DwString dwResult, dwSrc;
  int encoding = contentTransferEncoding();
  int len;

  mBodySize = aStr.size();

  switch (encoding)
  {
  case DwMime::kCteQuotedPrintable:
  case DwMime::kCteBase64:
    dwSrc = DwString(aStr.data(), aStr.size());
    if (encoding == DwMime::kCteQuotedPrintable)
      DwEncodeQuotedPrintable(dwSrc, dwResult);
    else
      DwEncodeBase64(dwSrc, dwResult);
    len = dwResult.size();
    mBody.truncate(len);
    memcpy(mBody.data(), dwResult.data(), len);
    break;
  default:
    kdDebug() << "WARNING -- unknown encoding `" << (const char*)cteStr() << "'. Assuming 8bit." << endl;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody.duplicate( aStr );
    break;
  }
}


//-----------------------------------------------------------------------------
QByteArray KMMessagePart::bodyDecodedBinary(void) const
{
  DwString dwResult, dwSrc;
  QByteArray result;
  int encoding = contentTransferEncoding();
  int len;

  switch (encoding)
  {
  case DwMime::kCteQuotedPrintable:
  case DwMime::kCteBase64:
    dwSrc = DwString(mBody.data(), mBody.size());
    if (encoding == DwMime::kCteQuotedPrintable)
      DwDecodeQuotedPrintable(dwSrc, dwResult);
    else
      DwDecodeBase64(dwSrc, dwResult);
    len = dwResult.size();
    result.resize(len);
    memcpy(result.data(), dwResult.c_str(), len);
    break;
  default:
    kdDebug() << "WARNING -- unknown encoding `" << (const char*)cteStr() << "'. Assuming 8bit." << endl;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    len = mBody.size() - 1;
    result.resize(len);
    if (len) memcpy(result.data(), mBody.data(), len);
    break;
  }
// do this for cases where bodyDecoded instead of bodyDecodedText
// is called. This does not warrant a trailing 0 - realloc might
// change the address. Maybe this is not really needed.
  result.resize(len+1);
  result[len] = 0;
  result.resize(len);

  return result;
}

QCString KMMessagePart::bodyDecoded(void) const
{
    QByteArray b(bodyDecodedBinary());
    QCString result(b.size()+1); // with additional trailing 0
    memcpy(result.data(),b.data(),b.size());
    result.truncate(result.length());
    if (result.length()<(b.size()-1))
      kdDebug() << "WARNING -- body is binary but used as text" << endl;
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
    body = bodyDecoded();
  else 
    body = mBody;
 
  result = KMimeMagic::self()->findBufferType( body );
  mimetype = result->mimeType();
  sep = mimetype.find('/');
  mType = mimetype.left(sep);
  mSubtype = mimetype.mid(sep+1, 64);
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::iconName(void) const
{
  QString fileName;
  fileName = KGlobal::instance()->iconLoader()->iconPath( mType.lower(), KIcon::Desktop );
  if (fileName.isEmpty())
    fileName = KGlobal::instance()->iconLoader()->iconPath( "unknown", KIcon::Desktop );
  return fileName;
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::typeStr(void) const
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
void KMMessagePart::setTypeStr(const QString aStr)
{
  mType = aStr.copy();
}


//-----------------------------------------------------------------------------
void KMMessagePart::setType(int aType)
{
  DwString dwType;
  DwTypeEnumToStr(aType, dwType);
  mType = dwType.c_str();
  
}



//-----------------------------------------------------------------------------
const QString KMMessagePart::subtypeStr(void) const
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
void KMMessagePart::setSubtypeStr(const QString aStr)
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
const QString KMMessagePart::contentTransferEncodingStr(void) const
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
void KMMessagePart::setContentTransferEncodingStr(const QString aStr)
{
  mCte = aStr.copy();
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncoding(int aCte)
{
  DwString dwCte;
  DwCteEnumToStr(aCte, dwCte);
  mCte = dwCte.c_str();
  
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::contentDescription(void) const
{
  return KMMsgBase::decodeRFC2047String(mContentDescription);
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDescription(const QString aStr)
{
  mContentDescription = KMMsgBase::encodeRFC2047String(aStr, charset());
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::fileName(void) const
{
  int i, j, len;
  QString str;
  int RFC2231encoded = 0;

  i = mContentDisposition.find("filename*=", 0, FALSE);
  if (i >= 0) { RFC2231encoded = 1; }
  else {
    i = mContentDisposition.find("filename=", 0, FALSE);
    if (i < 0) return QString::null;
  }
  j = mContentDisposition.find(';', i+9);

  if (j < 0) j = 32767;
  str = mContentDisposition.mid(i+9+RFC2231encoded, j-i-9-RFC2231encoded).
    stripWhiteSpace();

  len = str.length();
  if (str[0]=='"' && str[len-1]=='"') 
      str = str.mid(1, len-2);

  if (RFC2231encoded)
    str = KMMsgBase::decodeRFC2231String(str);
  else
    str = KMMsgBase::decodeQuotedPrintableString(str);

  return str;
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::contentDisposition(void) const
{
  return mContentDisposition;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDisposition(const QString aStr)
{
  mContentDisposition = aStr.copy();
}

 
//-----------------------------------------------------------------------------
const QString KMMessagePart::body(void) const
{
  return mBody;
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::name(void) const
{
  return mName;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setName(const QString aStr)
{
  mName = aStr;
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::charset(void) const
{
   return mCharset;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setCharset(const QString aStr)
{
  mCharset=aStr;
}



