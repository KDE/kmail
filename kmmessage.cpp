// kmmessage.cpp


// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#define ALLOW_GUI 1


#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmfolder.h"

#include <mimelib/mimepp.h>

#if ALLOW_GUI
#include <qmlined.h>
#endif


static DwString emptyString("");

//-----------------------------------------------------------------------------
KMMessage::KMMessage()
{
  mOwner = NULL;
  mStatus = stUnknown;
  mMsg = DwMessage::NewMessage(mMsgStr, 0);
  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMFolder* aOwner, DwMessage* aMsg)
{
  mOwner = aOwner;
  mNeedsAssembly = FALSE;
  if (!aMsg) mMsg = DwMessage::NewMessage(mMsgStr, 0);
  else mMsg->Parse();
  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
KMMessage::~KMMessage()
{
  if (mMsg) delete mMsg;
}


//-----------------------------------------------------------------------------
void KMMessage::setOwner(KMFolder* aFolder)
{
  mOwner = aFolder;
}


//-----------------------------------------------------------------------------
void KMMessage::takeMessage(DwMessage* aMsg)
{
  if (mMsg) delete mMsg;
  mMsg = aMsg;
  mMsg->Parse();
  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::asString(void)
{
  char* str;
  static QString qstr;
  DwString dwstr;
  int len, num, i;

  if (mNeedsAssembly)
  {
    mNeedsAssembly = FALSE;
    mMsg->Assemble();
  }

  // We have to work around a nasty bug in mimelib that causes a 
  // misplaced '\r' at the end of the header section where a '\n' should 
  // be. This seems to happen when mMsg->Parse() is called.
  str = (char*)mMsg->AsString().c_str();
  len = mMsg->Headers().AsString().length();
  for (num=0, i=len; i>0 && str[i]<=' '; i--)
  {
    if (str[i]=='\r') str[i]='\n';
    num++;
  }
  if (num>=3) str[len-1]='\r';

  mMsgStr = mMsg->AsString();
  return str;
}


//-----------------------------------------------------------------------------
void KMMessage::fromString(const QString aStr)
{
  mMsg->FromString((const char*)aStr);
  mMsg->Parse();
  mMsgStr = mMsg->AsString();
  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
void KMMessage::setStatus(Status aStatus)
{
  mStatus = aStatus;
  if (mOwner) mOwner->setMsgStatus(this, mStatus);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::statusToStr(Status aSt)
{
  static char str[2] = " ";
  str[0] = (char)aSt;
  return str;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::reply(void)
{
  KMMessage* msg = new KMMessage;





  debug("KMMessage::reply() needs implementation !");

  return msg;
}


//-----------------------------------------------------------------------------
void KMMessage::setAutomaticFields(void)
{
  DwHeaders& header = mMsg->Headers();
  header.MimeVersion().FromString("1.0");
  header.MessageId().CreateDefault();

  if (numBodyParts() > 1)
  {
    // Set the type to 'Multipart' and the subtype to 'Mixed'
    DwMediaType& contentType = mMsg->Headers().ContentType();
    contentType.SetType(DwMime::kTypeMultipart);
    contentType.SetSubtype(DwMime::kSubtypeMixed);

    // Create a random printable string and set it as the boundary parameter

    contentType.CreateBoundary(0);
  }
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::dateStr(void) const
{
  // Access the 'Date' header field and return its contents as a string
  
  DwHeaders& header = mMsg->Headers();
  if (header.HasDate()) return header.Date().AsString().c_str();
  return "";
}


//-----------------------------------------------------------------------------
time_t KMMessage::date(void) const
{
  // Access the 'Date' header field and return its contents as a string
  
  DwHeaders& header = mMsg->Headers();
  if (header.HasDate()) return header.Date().AsUnixTime();
  return (time_t)-1;
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(time_t aDate)
{
  mMsg->Headers().Date().FromUnixTime(aDate);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::to(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasTo()) return header.To().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const char* aStr)
{
  if (!aStr) return;
  mMsg->Headers().To().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::replyTo(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasReplyTo()) return header.ReplyTo().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(const char* aStr)
{
  if (!aStr) return;
  mMsg->Headers().ReplyTo().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(KMMessage* aMsg)
{
  mMsg->Headers().ReplyTo().FromString(aMsg->from());
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::cc(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasCc()) return header.Cc().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const char* aStr)
{
  if (!aStr) return;
  mMsg->Headers().Cc().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::bcc(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasBcc()) return header.Bcc().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const char* aStr)
{
  if (!aStr) return;
  mMsg->Headers().Bcc().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::from(void) const
{
  DwHeaders& header = mMsg->Headers();

  if (header.HasFrom()) return header.From().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setFrom(const char* aStr)
{
  mMsg->Headers().From().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::subject(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasSubject()) return header.Subject().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const char* aStr)
{
  if (!aStr) return;
  mMsg->Headers().Subject().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::headerField(const char* aName) const
{
  DwHeaders& header = mMsg->Headers();

  if (!aName || !header.HasField(aName)) return "";
  return header.FindField(aName)->AsString().c_str();
}


//-----------------------------------------------------------------------------
void KMMessage::setHeaderField(const char* aName, const char* aValue)
{
  DwHeaders& header = mMsg->Headers();
  DwString str;
  DwField* field;

  if (!aName) return;
  if (!aValue) aValue="";

  str = aName;
  if (str[str.length()-1] != ':') str += ": ";
  else str += " ";
  str += aValue;

  field = new DwField(str, mMsg);
  field->Parse();

  header.AddOrReplaceField(field);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::typeStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::type(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().Type();
  else return DwMime::kTypeNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setTypeStr(const char* aStr)
{
  mMsg->Headers().ContentType().SetTypeStr(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setType(int aType)
{
  mMsg->Headers().ContentType().SetType(aType);
  mNeedsAssembly = TRUE;
}



//-----------------------------------------------------------------------------
const char* KMMessage::subtypeStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().SubtypeStr().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::subtype(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().Subtype();
  else return DwMime::kSubtypeNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtypeStr(const char* aStr)
{
  mMsg->Headers().ContentType().SetSubtypeStr(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtype(int aSubtype)
{
  mMsg->Headers().ContentType().SetSubtype(aSubtype);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::contentTransferEncodingStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentTransferEncoding())
    return header.ContentTransferEncoding().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::contentTransferEncoding(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentTransferEncoding())
    return header.ContentTransferEncoding().AsEnum();
  else return DwMime::kCteNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncodingStr(const char* aStr)
{
  mMsg->Headers().ContentTransferEncoding().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncoding(int aCte)
{
  mMsg->Headers().ContentTransferEncoding().FromEnum(aCte);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMMessage::body(long* len_ret) const
{
  static DwString str;

  str = mMsg->Body().AsString();
  if (len_ret) *len_ret = str.length();
  return str.c_str();
}


//-----------------------------------------------------------------------------
void KMMessage::setBody(const char* aStr)
{
  mMsg->Body().FromString(aStr);
  mNeedsAssembly = TRUE;
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


//-----------------------------------------------------------------------------
void KMMessage::bodyPart(int aIdx, KMMessagePart* aPart)
{
  DwBodyPart* part;
  DwHeaders* headers;
  int curIdx;

  // Get the DwBodyPart for this index
  part = mMsg->Body().FirstBodyPart();
  for (curIdx=0; curIdx < aIdx && part; ++curIdx)
    part = part->Next();

  // If the DwBodyPart was found get the header fields and body
  if (part)
  {
    headers = &part->Headers();

    // Content-type
    if (headers->HasContentType())
    {
      aPart->setTypeStr(headers->ContentType().TypeStr().c_str());
      aPart->setSubtypeStr(headers->ContentType().SubtypeStr().c_str());
    }
    else
    {
      aPart->setTypeStr("Text");      // Set to defaults
      aPart->setSubtypeStr("Plain");
    }
    // Modification by Markus
    if(headers->ContentType().Name().c_str() != "" )
      aPart->setName(headers->ContentType().Name().c_str());
    else
      aPart->setName("unnamed");

    // Content-transfer-encoding
    if (headers->HasContentTransferEncoding())
      aPart->setCteStr(headers->ContentTransferEncoding().AsString().c_str());
    else 
      aPart->setCteStr("7bit");

    // Content-description
    if (headers->HasContentDescription())
      aPart->setContentDescription(headers->ContentDescription().AsString().c_str());
    else 
      aPart->setContentDescription("");

    // Content-disposition
    if (headers->HasContentDisposition())
      aPart->setContentDisposition(headers->ContentDisposition().AsString().c_str());
    else 
      aPart->setContentDisposition("");

    // Body
    aPart->setBody(part->Body().AsString().c_str());
  }

  // If the body part was not found, set all MultipartBodyPart attributes
  // to empty values.  This only happens if you don't pay attention to
  // the value returned from NumberOfParts().
  else
  {
    aPart->setTypeStr("");
    aPart->setSubtypeStr("");
    aPart->setCteStr("");
    aPart->setName("");
    aPart->setContentDescription("");
    aPart->setContentDisposition("");
    aPart->setBody("");
  }
}


//-----------------------------------------------------------------------------
void KMMessage::setBodyPart(int aIdx, const KMMessagePart* aPart)
{
  DwBody&     body = mMsg->Body();
  DwBodyPart* part = 0;
  int         numParts = numBodyParts();

  assert(aIdx >= 0);
  assert(aPart != NULL);

  // If indexed part exists already, just replace its values
  if (aIdx < numParts)
  {
    part = body.FirstBodyPart();
    for (int curIdx=0; curIdx < aIdx; ++curIdx)
      part = part->Next();
  }
  // Otherwise, add as many new parts as necessary.
  else if (numParts <= aIdx)
  {
    while (numParts <= aIdx)
    {
      part = DwBodyPart::NewBodyPart(emptyString, 0);
      body.AddBodyPart(part);
      ++numParts;
    }
  }

  const DwString type     = aPart->typeStr();
  const DwString subtype  = aPart->subtypeStr();
  const DwString cte      = aPart->cteStr();
  const DwString contDesc = aPart->contentDescription();
  const DwString contDisp = aPart->contentDisposition();
  const DwString bodyStr  = aPart->body();

  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr(type);
    headers.ContentType().SetSubtypeStr(subtype);
  }
  if (cte != "")
    headers.Cte().FromString(cte);

  if (contDesc != "")
    headers.ContentDescription().FromString(contDesc);

  if (contDisp != "")
    headers.ContentDisposition().FromString(contDisp);

  part->Body().FromString(bodyStr); 
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::addBodyPart(const KMMessagePart* aPart)
{
  DwBodyPart* part = DwBodyPart::NewBodyPart(emptyString, 0);

  const DwString type     = aPart->typeStr();
  const DwString subtype  = aPart->subtypeStr();
  const DwString cte      = aPart->cteStr();
  const DwString contDesc = aPart->contentDescription();
  const DwString contDisp = aPart->contentDisposition();
  const DwString bodyStr  = aPart->body();
  const DwString name     = aPart->name();

  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr(type);
    headers.ContentType().SetSubtypeStr(subtype);
  }

  if(name != "")
    headers.ContentType().SetName(name);

  if (cte != "")
    headers.Cte().FromString(cte);

  if (contDesc != "")
    headers.ContentDescription().FromString(contDesc);

  if (contDisp != "")
    headers.ContentDisposition().FromString(contDisp);

  part->Body().FromString(bodyStr); 
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::viewSource(const char* aCaption) const
{
  const char* str = ((KMMessage*)this)->asString();

#if ALLOW_GUI
  QMultiLineEdit* edt;

  edt = new QMultiLineEdit;
  if (aCaption) edt->setCaption(aCaption);

  edt->insertLine(str);
  edt->setReadOnly(TRUE);
  edt->show();

#else //not ALLOW_GUI
  debug("Message source: %s\n%s\n--- end of message ---", 
	aCaption ? aCaption : "", str);

#endif
}
