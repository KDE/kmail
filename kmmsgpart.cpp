// kmmsgpart.cpp

#include "kmmsgpart.h"
#include "kmmessage.h"

#include <mimelib/enum.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>
#include <mimelib/utility.h>

//-----------------------------------------------------------------------------
KMMessagePart::KMMessagePart() : 
  mType("text"), mSubtype("plain"), mCte("7bit"), mContentDescription(),
  mContentDisposition(), mBody(), mName()
{
}


//-----------------------------------------------------------------------------
KMMessagePart::~KMMessagePart()
{
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::bodyEncoded(void) const
{
  DwString dwResult, dwSrc;
  QString result;
  int encoding = contentTransferEncoding();

  switch (encoding)
  {
  case DwMime::kCteQuotedPrintable:
    dwSrc = mBody;
    DwEncodeQuotedPrintable(dwSrc, dwResult);
    result = dwResult.c_str();
    result.detach();
    break;
  case DwMime::kCteBase64:
    dwSrc = mBody;
    DwEncodeBase64(dwSrc, dwResult);
    result = dwResult.c_str();
    result.detach();
    break;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    result = mBody;
    break;
  default:
    debug("WARNING -- unknown encoding `%s'. Assuming 8bit.", 
	  (const char*)cteStr());
    result = mBody;
  }
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::bodyDecoded(void) const
{
  DwString dwResult, dwSrc;
  QString result;
  int encoding = contentTransferEncoding();

  switch (encoding)
  {
  case DwMime::kCteQuotedPrintable:
    dwSrc = mBody;
    DwDecodeQuotedPrintable(dwSrc, dwResult);
    result = dwResult.c_str();
    result.detach();
    break;
  case DwMime::kCteBase64:
    dwSrc = mBody;
    DwDecodeBase64(dwSrc, dwResult);
    result = dwResult.c_str();
    result.detach();
    break;
  case DwMime::kCte7bit:
  case DwMime::kCte8bit:
  case DwMime::kCteBinary:
    result = mBody;
    break;
  default:
    debug("WARNING -- unknown encoding `%s'. Assuming 8bit.", 
	  (const char*)cteStr());
    result = mBody;
  }
  return result;
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
  mType.detach();
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
  mSubtype.detach();
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
  mCte.detach();
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
void KMMessagePart::setBody(const QString aStr)
{
  mBody = aStr.copy();
  debug("body part: %s\n", (const char*)mBody);
}


//-----------------------------------------------------------------------------
const QString KMMessagePart::name(void) const
{
  return mName;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setName(const QString aStr)
{
  mName = aStr.copy();
}



