// kmmsgpart.cpp

#include <qdir.h>

#include "kmmsgpart.h"
#include "kmmessage.h"
#include <kmimemagic.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kiconloader.h>
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
      bodyDecoded().size() - 1;
  }
  return mBodySize;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setBody(const QString aStr)
{
  int encoding = contentTransferEncoding();

  mBody = aStr.copy();

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
  int len;

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
  len = dwResult.size();
  result = QString( dwResult.c_str() );
  result.truncate(22);
  return result;
}

//-----------------------------------------------------------------------------
void KMMessagePart::setBodyEncoded(const QByteArray& aStr)
{
  DwString dwResult, dwSrc;
  int encoding = contentTransferEncoding();
  int len;

  mBodySize = aStr.size();

  switch (encoding)
  {
  case DwMime::kCteQuotedPrintable:
    dwSrc = DwString(aStr.data(), aStr.size());
    DwEncodeQuotedPrintable(dwSrc, dwResult);
    len = dwResult.size();
    mBody.truncate(len);
    memcpy(mBody.data(), dwResult.c_str(), len);
    break;
  case DwMime::kCteBase64:
    dwSrc = DwString(aStr.data(), aStr.size());
    DwEncodeBase64(dwSrc, dwResult);
    len = dwResult.size();
    mBody.truncate(len);
    memcpy(mBody.data(), dwResult.c_str(), len);
    break;
  default:
    debug("WARNING -- unknown encoding `%s'. Assuming 8bit.", 
	  (const char*)cteStr());
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    mBody.duplicate( aStr );
    break;
  }
}


//-----------------------------------------------------------------------------
QByteArray KMMessagePart::bodyDecoded(void) const
{
  DwString dwResult, dwSrc;
  QByteArray result;
  int encoding = contentTransferEncoding();
  int len;

  switch (encoding)
  {
  case DwMime::kCteQuotedPrintable:
    dwSrc = DwString(mBody.data(), mBody.size());
    DwDecodeQuotedPrintable(dwSrc, dwResult);
    len = dwResult.size();
    result.resize(len);
    memcpy((void*)result.data(), (void*)dwResult.c_str(), len);
    break;
  case DwMime::kCteBase64:
    dwSrc = DwString(mBody.data(), mBody.size());
    DwDecodeBase64(dwSrc, dwResult);
    len = dwResult.size();
    result.resize(len);
    memcpy((void*)result.data(), (void*)dwResult.c_str(), len);
    break;
  default:
    debug("WARNING -- unknown encoding `%s'. Assuming 8bit.", 
	  (const char*)cteStr());
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    result.duplicate( mBody );
    break;
  }

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
  return mContentDescription;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDescription(const QString aStr)
{
  mContentDescription = aStr.copy();
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::fileName(void) const
{
  int i, j, len;
  QString str;

  i = mContentDisposition.find("filename=", 0, FALSE);
  if (i < 0) return QString::null;
  j = mContentDisposition.find(';', i+9);

  if (j < 0) j = 32767;
  str = mContentDisposition.mid(i+9, j-i-9).stripWhiteSpace();

  len = str.length();
  if (str[0]=='"' && str[len-1]=='"') 
      str = str.mid(1, len-2);

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
  mName = KMMsgBase::decodeQuotedPrintableString(aStr);
}


#if defined CHARSETS
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
#endif



