// kmmessage.cpp


// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#define ALLOW_GUI 1

#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsginfo.h"
#include "kmfolder.h"
#include "kmidentity.h"
#include "kmversion.h"

// we need access to the protected member DwBody::DeleteBodyParts()...
#define protected public
#include <mimelib/body.h>
#undef protected

#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#if ALLOW_GUI
#include <qmlined.h>
#endif


// Originally in kmglobal.h, but we want to avoid to depend on it here
extern KMIdentity* identity;


static DwString emptyString("");
static QString resultStr;



//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMFolder* parent): KMMessageInherited(parent)
{
  mNeedsAssembly = FALSE;
  mMsg = new DwMessage;
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(const KMMsgInfo& msgInfo): KMMessageInherited()
{
  mNeedsAssembly = FALSE;
  mMsg = new DwMessage;

  assign(&msgInfo);
}


//-----------------------------------------------------------------------------
KMMessage::~KMMessage()
{
  if (mMsg) delete mMsg;
}


//-----------------------------------------------------------------------------
bool KMMessage::isMessage(void) const
{
  return TRUE;
}
   

//-----------------------------------------------------------------------------
const QString KMMessage::asString(void)
{
  if (mNeedsAssembly)
  {
    mNeedsAssembly = FALSE;
    mMsg->Assemble();
  }
  resultStr = mMsg->AsString().c_str();
  return resultStr;
}


//-----------------------------------------------------------------------------
void KMMessage::fromString(const QString aStr)
{
  if (mMsg) delete mMsg;
  mMsg = new DwMessage;

  resultStr = aStr;
  mMsg->FromString((const char*)aStr);
  mMsg->Parse();
  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::asQuotedString(const QString aHeaderStr,
					const QString aIndentStr) const
{
  QString headerStr(256);
  KMMessagePart msgPart;
  QRegExp reNL("\\n");
  QString nlIndentStr;
  char   cstr[64];
  char ch;
  int i;
  time_t tm;

  nlIndentStr = "\n" + aIndentStr;

  // insert fields into wildcards of header-string
  headerStr = "";
  for (i=0; (ch=aHeaderStr[i]) != '\0'; i++)
  {
    if (ch=='%')
    {
      i++;
      ch = aHeaderStr[i];
      switch (ch)
      {
      case 'D':
	tm = date();
	strftime(cstr, 63, "%a, %d %b %Y", localtime(&tm));
	headerStr += cstr;
	break;
      case 'F':
	headerStr += stripEmailAddr(from());
	break;
      case 'S':
	headerStr += subject();
	break;
      case '%':
	headerStr += '%';
	break;
      default:
	headerStr += '%';
	headerStr += ch;
	break;
      }
    }
    else headerStr += ch;
  }

  // Quote message. Do not quote mime message parts that are of other
  // type than "text".
  if (numBodyParts() == 0)
  {
    resultStr = QString(body()).stripWhiteSpace().replace(reNL,nlIndentStr) +
                aIndentStr;
  }
  else
  {
    resultStr = "";
    for (i=0; i<numBodyParts(); i++)
    {
      bodyPart(i, &msgPart);
      if (msgPart.typeStr()=="text")
      {
	resultStr += aIndentStr;
	resultStr += QString(msgPart.body()).replace(reNL,(const char*)nlIndentStr);
	resultStr += aIndentStr;
      }
      else
      {
	resultStr += "\n"+aIndentStr+"["+msgPart.name()+"] "+
	             msgPart.contentDescription()+nlIndentStr+"\n";
      }
    }
  }

  resultStr = headerStr + nlIndentStr + resultStr;
  return resultStr;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createReply(bool replyToAll) const
{
  KMMessage* msg = new KMMessage;
  QString str;

  msg->initHeader();
  if (!replyTo().isEmpty()) 
  {
    str = replyTo().copy();
    if (replyToAll)
    {
      str += ", ";
      str += from();
    }
    msg->setTo(str);
  }
  else msg->setTo(from());
  msg->setCc(cc());
  msg->setBody(asQuotedString("On %D, %F wrote:", "> "));

  if (strnicmp(subject(), "Re:", 3)!=0)
    msg->setSubject("Re: " + subject());
  else msg->setSubject(subject());

  return msg;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createForward(void) const
{
  KMMessage* msg = new KMMessage;
  QString str;

  msg->initHeader();

  str = "\n\n----------  Forwarded message  ----------\n";
  str += "Subject: " + subject() + "\n";
  str += "Date: " + dateStr() + "\n";
  str += "From: " + from() + "\n";
  str += "\n";
  msg->setBody(asQuotedString(str, ""));

  if (strnicmp(subject(), "Fwd:", 4)!=0)
    msg->setSubject("Fwd: " + subject());
  else msg->setSubject(subject());

  return msg;
}


//-----------------------------------------------------------------------------
void KMMessage::initHeader(void)
{
  struct timeval tval;

  assert(identity != NULL);
  setFrom(identity->fullEmailAddr());
  setTo("");
  setSubject("");
  if (!identity->replyToAddr().isEmpty()) setReplyTo(identity->replyToAddr());
  setHeaderField("X-Mailer", "KMail [version " KMAIL_VERSION "]");

  gettimeofday(&tval, NULL);
  setDate((time_t)tval.tv_sec);
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
const QString KMMessage::dateStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasDate()) return header.Date().AsString().c_str();
  return "";
}


//-----------------------------------------------------------------------------
const QString KMMessage::dateShortStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();
  return ctime(&unixTime);
}


//-----------------------------------------------------------------------------
time_t KMMessage::date(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasDate()) return header.Date().AsUnixTime();
  return (time_t)-1;
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(time_t aDate)
{
  KMMessageInherited::setDate(aDate);
  mMsg->Headers().Date().FromUnixTime(aDate);
  mNeedsAssembly = TRUE;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(const QString aStr)
{
  DwHeaders& header = mMsg->Headers();

  header.Date().FromString(aStr);
  header.Date().Parse();
  mNeedsAssembly = TRUE;
  mDirty = TRUE;

  if (header.HasDate())
    KMMessageInherited::setDate(header.Date().AsUnixTime());
}


//-----------------------------------------------------------------------------
const QString KMMessage::to(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasTo()) return header.To().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().To().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::replyTo(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasReplyTo()) return header.ReplyTo().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(const QString aStr)
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
const QString KMMessage::cc(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasCc()) return header.Cc().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().Cc().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::bcc(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasBcc()) return header.Bcc().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().Bcc().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::from(void) const
{
  DwHeaders& header = mMsg->Headers();

  if (header.HasFrom()) return header.From().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setFrom(const QString aStr)
{
  mMsg->Headers().From().FromString(aStr);
  mNeedsAssembly = TRUE;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::subject(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasSubject()) return header.Subject().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().Subject().FromString(aStr);
  mNeedsAssembly = TRUE;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::headerField(const QString aName) const
{
  DwHeaders& header = mMsg->Headers();

  if (!aName || !header.HasField(aName)) return "";
  return header.FindField(aName)->AsString().c_str();
}


//-----------------------------------------------------------------------------
void KMMessage::setHeaderField(const QString aName, const QString aValue)
{
  DwHeaders& header = mMsg->Headers();
  DwString str;
  DwField* field;

  if (aName.isEmpty()) return;

  str = aName;
  if (str[str.length()-1] != ':') str += ": ";
  else str += " ";
  str += aValue;
  if (aValue.right(1)!="\n") str += "\n";

  field = new DwField(str, mMsg);
  field->Parse();

  header.AddOrReplaceField(field);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::typeStr(void) const
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
void KMMessage::setTypeStr(const QString aStr)
{
  mMsg->Headers().ContentType().SetTypeStr((const char*)aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setType(int aType)
{
  mMsg->Headers().ContentType().SetType(aType);
  mNeedsAssembly = TRUE;
}



//-----------------------------------------------------------------------------
const QString KMMessage::subtypeStr(void) const
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
void KMMessage::setSubtypeStr(const QString aStr)
{
  mMsg->Headers().ContentType().SetSubtypeStr((const char*)aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtype(int aSubtype)
{
  mMsg->Headers().ContentType().SetSubtype(aSubtype);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::contentTransferEncodingStr(void) const
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
void KMMessage::setContentTransferEncodingStr(const QString aStr)
{
  mMsg->Headers().ContentTransferEncoding().FromString((const char*)aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncoding(int aCte)
{
  mMsg->Headers().ContentTransferEncoding().FromEnum(aCte);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::body(void) const
{
  QString str;
  str = mMsg->Body().AsString().c_str();
  return str;
}


//-----------------------------------------------------------------------------
const QString KMMessage::bodyDecoded(void) const
{
  DwString dwsrc, dwstr;

  dwsrc = mMsg->Body().AsString().c_str();
  switch (cte())
  {
  case DwMime::kCteBase64:
    DwDecodeBase64(dwsrc, dwstr);
    resultStr = dwstr.c_str();
    break;
  case DwMime::kCteQuotedPrintable:
    DwDecodeQuotedPrintable(dwsrc, dwstr);
    resultStr = dwstr.c_str();
    break;
  default:
    resultStr = dwsrc.c_str();
    break;
  }

  return resultStr;
}


//-----------------------------------------------------------------------------
void KMMessage::setBody(const QString aStr)
{
  mMsg->Body().FromString((const char*)aStr);
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
void KMMessage::bodyPart(int aIdx, KMMessagePart* aPart) const
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

  const DwString type     = (const char*)aPart->typeStr();
  const DwString subtype  = (const char*)aPart->subtypeStr();
  const DwString cte      = (const char*)aPart->cteStr();
  const DwString contDesc = (const char*)aPart->contentDescription();
  const DwString contDisp = (const char*)aPart->contentDisposition();
  const DwString bodyStr  = (const char*)aPart->body();

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
void KMMessage::deleteBodyParts(void)
{
  mMsg->Body().DeleteBodyParts();
}


//-----------------------------------------------------------------------------
void KMMessage::addBodyPart(const KMMessagePart* aPart)
{
  DwBodyPart* part = DwBodyPart::NewBodyPart(emptyString, 0);

  QString type     = aPart->typeStr();
  QString subtype  = aPart->subtypeStr();
  QString cte      = aPart->cteStr();
  QString contDesc = aPart->contentDescription();
  QString contDisp = aPart->contentDisposition();
  QString name     = aPart->name();

  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr((const char*)type);
    headers.ContentType().SetSubtypeStr((const char*)subtype);
  }

  if(!name.isEmpty())
    headers.ContentType().SetName((const char*)name);

  if (!cte.isEmpty())
    headers.Cte().FromString(cte);

  if (!contDesc.isEmpty())
    headers.ContentDescription().FromString(contDesc);

  if (!contDisp.isEmpty())
    headers.ContentDisposition().FromString(contDisp);

  part->Body().FromString(aPart->body());
  mMsg->Body().AddBodyPart(part);

  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::viewSource(const QString aCaption) const
{
  QString str = ((KMMessage*)this)->asString();

#if ALLOW_GUI
  QMultiLineEdit* edt;

  edt = new QMultiLineEdit;
  if (aCaption) edt->setCaption(aCaption);

  edt->insertLine(str);
  edt->setReadOnly(TRUE);
  edt->show();

#else //not ALLOW_GUI
  debug("Message source: %s\n%s\n--- end of message ---", 
	aCaption.isEmpty() ? "" : (const char*)aCaption, str);

#endif
}


//-----------------------------------------------------------------------------
const QString KMMessage::stripEmailAddr(const QString aStr)
{
  int i, j;
  QString partA, partB;
  char endCh = '>';

  i = aStr.find('<');
  if (i<0)
  {
    i = aStr.find('(');
    endCh = ')';
  }
  if (i<0) return aStr;
  partA = aStr.left(i);
  j = aStr.find(endCh,i);
  if (j<0) return aStr;
  partB = aStr.mid(i+1, j-i-1);

  if (partA.find('@') >= 0) return partB.stripWhiteSpace();
  return partA.stripWhiteSpace();
}


//-----------------------------------------------------------------------------
const QString KMMessage::emailAddrAsAnchor(const QString email, bool stripped)
{
  QString result;

  result = email.copy();
  result.replace(QRegExp("\""), "`");
  result.replace(QRegExp("<"), "&lt:");
  result.replace(QRegExp(">"), "&gt:");
  result.prepend("<A HREF=\"mailto:");
  result.append("\">");
  if (stripped) result += KMMessage::stripEmailAddr(email);
  else result += email;
  result += "</A>";
  return result;
}
