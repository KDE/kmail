// kmmsgpart.cpp

#include "kmmsgpart.h"
#include "kmmessage.h"

#include <mimelib/enum.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>
#include <mimelib/utility.h>

//-----------------------------------------------------------------------------
KMMessagePart::KMMessagePart() : mType("Text"), mSubtype("Plain"), mCte("7bit")
{
}


//-----------------------------------------------------------------------------
KMMessagePart::~KMMessagePart()
{
}


//-----------------------------------------------------------------------------
const char* KMMessagePart::typeStr(void) const
{
  return mType.c_str();
}


//-----------------------------------------------------------------------------
int KMMessagePart::type(void) const
{
  int type = DwTypeStrToEnum(mType);
  return type;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setTypeStr(const char* aStr)
{
  mType = aStr;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setType(int aType)
{
  DwTypeEnumToStr(aType, mType);
}



//-----------------------------------------------------------------------------
const char* KMMessagePart::subtypeStr(void) const
{
  return mSubtype.c_str();
}


//-----------------------------------------------------------------------------
int KMMessagePart::subtype(void) const
{
  int subtype = DwSubtypeStrToEnum(mSubtype);
  return subtype;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setSubtypeStr(const char* aStr)
{
  mSubtype = aStr;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setSubtype(int aSubtype)
{
  DwSubtypeEnumToStr(aSubtype, mSubtype);
}


//-----------------------------------------------------------------------------
const char* KMMessagePart::contentTransferEncodingStr(void) const
{
  return mCte.c_str();
}


//-----------------------------------------------------------------------------
int KMMessagePart::contentTransferEncoding(void) const
{
  int cte = DwCteStrToEnum(mCte);
  return cte;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncodingStr(const char* aStr)
{
  mCte = aStr;
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentTransferEncoding(int aCte)
{
  DwCteEnumToStr(aCte, mCte);
}


//-----------------------------------------------------------------------------
const char* KMMessagePart::contentDescription(void) const
{
  return mContentDescription.c_str();
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDescription(const char* aStr)
{
  mContentDescription = aStr;
}


//-----------------------------------------------------------------------------
const char* KMMessagePart::contentDisposition(void) const
{
  return mContentDisposition.c_str();
}


//-----------------------------------------------------------------------------
void KMMessagePart::setContentDisposition(const char* aStr)
{
  mContentDisposition = aStr;
}

 
//-----------------------------------------------------------------------------
const char* KMMessagePart::body(long* len_ret) const
{
  if (len_ret) *len_ret = mBody.length();
  return mBody.data();
}


//-----------------------------------------------------------------------------
void KMMessagePart::setBody(const char* aStr)
{
  mBody = aStr;
}
//-----------------------------------------------------------------------------
// Function added by Markus. The new mimelib allows reading/writing  of name
// parameter
const char * KMMessagePart::name(void) const
{
  return mName.c_str();
}

void KMMessagePart::setName(const char *aStr)
{
  mName = aStr;
}



