// kmmessage.cpp

#include "kmmessage.h"
#include "kmfolder.h"

#include <mimelib/message.h>
#include <mimelib/header.h>
#include <mimelib/text.h>
#include <mimelib/addrlist.h>
#include <mimelib/mboxlist.h>
#include <mimelib/enum.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>


//-----------------------------------------------------------------------------
KMMessage::KMMessage()
{
  mOwner = NULL;
  mStatus = stUnknown;
  mMsg = DwMessage::NewMessage(mMsgStr, 0);
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMFolder* aOwner, DwMessage* aMsg)
{
  mOwner = aOwner;
  if (!aMsg) mMsg = DwMessage::NewMessage(mMsgStr, 0);
}


//-----------------------------------------------------------------------------
KMMessage::~KMMessage()
{
  if (mMsg) delete mMsg;
}


//-----------------------------------------------------------------------------
void KMMessage::takeMessage(DwMessage* aMsg)
{
  if (mMsg) delete mMsg;
  mMsg = aMsg;
}


//-----------------------------------------------------------------------------
const char* KMMessage::asString(void)
{
  mMsg->Assemble();
  return mMsg->AsString().c_str();
}


//-----------------------------------------------------------------------------
void KMMessage::setStatus(Status aStatus)
{
  mStatus = aStatus;

  //if (mOwner) mOwner->setMsgStatus(this, mStatus);
}


//-----------------------------------------------------------------------------
const char* KMMessage::statusToStr(Status aSt)
{
  static char str[2] = " ";
  str[0] = (char)aSt;
  return str;
}


//-----------------------------------------------------------------------------
void KMMessage::setAutomaticFields(void)
{
  DwHeader& header = mMsg->Header();
  header.MimeVersion().FromString("1.0");
  header.MessageId().CreateDefault();

  if (numBodyParts() > 1)
  {
    // Set the type to 'Multipart' and the subtype to 'Mixed'

    DwMediaType& contentType = mMsg->Header().ContentType();
    contentType.SetType(DwMime::kTypeMultipart);
    contentType.SetSubtype(DwMime::kSubtypeMixed);

    // Create a random printable string and set it as the boundary parameter

    contentType.CreateBoundary(0);
  }
}


//-----------------------------------------------------------------------------
const char* KMMessage::dateStr(void) const
{
  // Access the 'Date' header field and return its contents as a string
  
  DwHeader& header = mMsg->Header();
  if (header.HasDate()) return header.Date().AsString().c_str();
  return NULL;
}


//-----------------------------------------------------------------------------
time_t KMMessage::date(void) const
{
  // Access the 'Date' header field and return its contents as a string
  
  DwHeader& header = mMsg->Header();
  if (header.HasDate()) return header.Date().AsUnixTime();
  return (time_t)-1;
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(time_t aDate)
{
  mMsg->Header().Date().FromUnixTime(aDate);
}


//-----------------------------------------------------------------------------
const char* KMMessage::to(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasTo()) return header.To().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const char* aStr)
{
  mMsg->Header().To().FromString(aStr);
}


//-----------------------------------------------------------------------------
const char* KMMessage::replyTo(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasReplyTo()) return header.ReplyTo().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(const char* aStr)
{
  mMsg->Header().ReplyTo().FromString(aStr);
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(KMMessage* aMsg)
{
  mMsg->Header().ReplyTo().FromString(aMsg->from());
}


//-----------------------------------------------------------------------------
const char* KMMessage::cc(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasCc()) return header.Cc().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const char* aStr)
{
  mMsg->Header().Cc().FromString(aStr);
}


//-----------------------------------------------------------------------------
const char* KMMessage::bcc(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasBcc()) return header.Bcc().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const char* aStr)
{
  mMsg->Header().Bcc().FromString(aStr);
}


//-----------------------------------------------------------------------------
const char* KMMessage::from(void) const
{
  DwHeader& header = mMsg->Header();

  if (header.HasFrom()) return header.From().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setFrom(const char* aStr)
{
  mMsg->Header().From().FromString(aStr);
}


//-----------------------------------------------------------------------------
const char* KMMessage::subject(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasSubject()) return header.Subject().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const char* aStr)
{
  mMsg->Header().Subject().FromString(aStr);
}


//-----------------------------------------------------------------------------
const char* KMMessage::typeStr(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasContentType()) return header.ContentType().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::type(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasContentType()) return header.ContentType().Type();
  else return DwMime::kTypeNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setTypeStr(const char* aStr)
{
  mMsg->Header().ContentType().SetTypeStr(aStr);
}


//-----------------------------------------------------------------------------
void KMMessage::setType(int aType)
{
  mMsg->Header().ContentType().SetType(aType);
}



//-----------------------------------------------------------------------------
const char* KMMessage::subtypeStr(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasContentType()) return header.ContentType().SubtypeStr().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::subtype(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasContentType()) return header.ContentType().Subtype();
  else return DwMime::kSubtypeNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtypeStr(const char* aStr)
{
  mMsg->Header().ContentType().SetSubtypeStr(aStr);
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtype(int aSubtype)
{
  mMsg->Header().ContentType().SetSubtype(aSubtype);
}


//-----------------------------------------------------------------------------
const char* KMMessage::contentTransferEncodingStr(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasContentTransferEncoding())
    return header.ContentTransferEncoding().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::contentTransferEncoding(void) const
{
  DwHeader& header = mMsg->Header();
  if (header.HasContentTransferEncoding())
    return header.ContentTransferEncoding().AsEnum();
  else return DwMime::kCteNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncodingStr(const char* aStr)
{
  mMsg->Header().ContentTransferEncoding().FromString(aStr);
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncoding(int aCte)
{
  mMsg->Header().ContentTransferEncoding().FromEnum(aCte);
}


//-----------------------------------------------------------------------------
const char* KMMessage::body(long* len_ret) const
{
  const DwString* str = &mMsg->Body().AsString();
  if (len_ret) *len_ret = str->length();
  return str->c_str();
}


//-----------------------------------------------------------------------------
void KMMessage::setBody(const char* aStr)
{
  mMsg->Body().FromString(aStr);
}


//-----------------------------------------------------------------------------
int KMMessage::numBodyParts(void) const
{
  int count;
  DwBodyPart* part = mMsg->Body().FirstBodyPart();

  for (count=0; part; count++)
    part = part->Next();

  return count;
}
