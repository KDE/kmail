// kmmessage.cpp


// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#define ALLOW_GUI 1

#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsginfo.h"
#ifndef KRN
#include "kmfolder.h"
#include "kmversion.h"
#endif
#include "kmidentity.h"

#include <kapp.h>
#include <kconfig.h>

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
// Added by KRN to allow internationalized config defaults
extern KLocale* nls;


static DwString emptyString("");
static QString resultStr;

// Values that are set from the config file with KMMessage::readConfig()
static QString sReplyStr, sForwardStr, sReplyAllStr, sIndentPrefixStr;


/* Start functions added for KRN */

//-----------------------------------------------------------------------------
KMMessage::KMMessage(DwMessage* aMsg)
{
    mNeedsAssembly = TRUE;
    mMsg=aMsg;
}

//-----------------------------------------------------------------------------
const QString KMMessage::followup(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasFollowupTo()) return header.FollowupTo().AsString().c_str();
  else
  {
      if (header.HasNewsgroups()) return header.Newsgroups().AsString().c_str();
      else return "";
  }
}

//-----------------------------------------------------------------------------
void KMMessage::setFollowup(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().FollowupTo().FromString(aStr);
  mNeedsAssembly = TRUE;
}

//-----------------------------------------------------------------------------
const QString KMMessage::groups(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasNewsgroups())
      return header.Newsgroups().AsString().c_str();
  else
      return "";
}
//-----------------------------------------------------------------------------
void KMMessage::setGroups(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().Newsgroups().FromString(aStr);
  mNeedsAssembly = TRUE;
}

//-----------------------------------------------------------------------------
const QString KMMessage::references(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasReferences())
      return header.References().AsString().c_str();
  else
      return "";
}
//-----------------------------------------------------------------------------
void KMMessage::setReferences(const QString aStr)
{
  if (!aStr) return;
  mMsg->Headers().References().FromString(aStr);
  mNeedsAssembly = TRUE;
}
//-----------------------------------------------------------------------------
const QString KMMessage::id(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasMessageId())
      return header.MessageId().AsString().c_str();
  else
      return "";
}

//-----------------------------------------------------------------------------
#ifdef KRN
const QString KMMessage::refsAsAnchor(const QString references)
{
    QString refsdata=references;
    QString t,t2,result;
    int count=1;

    while (1)
    {
        int index=refsdata.find('>');
        if (index==-1)
        {
            break;
            refsdata=refsdata.stripWhiteSpace();
            refsdata=refsdata.mid(1,refsdata.length()-2);
            t.setNum(count++);
            t="<a href=\"news:///"+refsdata+"\">"+t+"</a> ";
            result+=t;
        }
        else
        {
            t.setNum(count++);
            t2=refsdata.left(index+1).stripWhiteSpace();
            t2=t2.mid(1,t2.length()-2);
            t="<a href=\"news:///"+t2+"\">"+t+"</a> ";
            refsdata=refsdata.right(refsdata.length()-index-1);
            result+=t;
        }
    }
    return result.data();
}
#endif
/* End of functions added by KRN */


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
void KMMessage::setStatusFields(void)
{
  char str[3];

  str[0] = (char)status();
  str[1] = '\0';

  setHeaderField("Status", status()==KMMsgStatusNew ? "R " : "RO");
  setHeaderField("X-Status", str);
}

//----------------------------------------------------------------------------
const QString KMMessage::headerAsString(void)
{
  DwHeaders& header = mMsg->Headers();
  if(header.AsString() != "")
    return header.AsString().c_str();
  return "";
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
  bool isInline;
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
    resultStr = QString(bodyDecoded()).stripWhiteSpace()
                .replace(reNL,nlIndentStr) + '\n';
  }
  else
  {
    resultStr = "";
    for (i = 0; i < numBodyParts(); i++)
    {
      bodyPart(i, &msgPart);

      if (i==0) isInline = TRUE;
      else isInline = (stricmp(msgPart.contentDisposition(),"inline")==0);

      if (isInline)
      {
	if (stricmp(msgPart.typeStr(),"text")==0 || 
	    stricmp(msgPart.typeStr(),"message")==0)
	{
	  resultStr += aIndentStr;
	  resultStr += QString(msgPart.bodyDecoded())
	    .replace(reNL,(const char*)nlIndentStr);
	  resultStr += '\n';
	}
	else isInline = FALSE;
      }
      if (!isInline)
      {
	resultStr += QString("\n----------------------------------------") +
	  "\nContent-Type: " + msgPart.typeStr() + "/" + msgPart.subtypeStr();
	if (!msgPart.name().isEmpty())
	  resultStr += "; name=\"" + msgPart.name() + '"';

	resultStr += QString("\nContent-Transfer-Encoding: ")+ 
	  msgPart.cteStr() + "\nContent-Description: " + 
	  msgPart.contentDescription() +
	  "\n----------------------------------------\n";
      }
    }
  }

  resultStr = headerStr + nlIndentStr + resultStr;
  return resultStr;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createReply(bool replyToAll)
{
  KMMessage* msg = new KMMessage;
  QString str, replyStr, loopToStr, replyToStr, toStr;

  msg->initHeader();

  loopToStr = headerField("X-Loop");
  replyToStr = replyTo();

  if (replyToAll)
  {
    if (!replyToStr.isEmpty()) toStr += replyToStr + ", ";
    else if (!loopToStr.isEmpty()) toStr = loopToStr + ", ";
    if (!from().isEmpty()) toStr += from() + ", ";
    toStr.truncate(toStr.length()-2);
  }
  else
  {
    if (!replyToStr.isEmpty()) toStr = replyToStr;
    else if (!loopToStr.isEmpty()) toStr = loopToStr;
    else if (!from().isEmpty()) toStr = from();
  }

  if (!toStr.isEmpty()) msg->setTo(toStr);

  if (replyToAll || !loopToStr.isEmpty()) replyStr = sReplyAllStr;
  else replyStr = sReplyStr;

  msg->setCc(cc());
  msg->setBody(asQuotedString(replyStr, sIndentPrefixStr));

  if (strnicmp(subject(), "Re:", 3)!=0)
    msg->setSubject("Re: " + subject());
  else msg->setSubject(subject());

  setStatus(KMMsgStatusReplied);

  return msg;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createForward(void)
{
  KMMessage* msg = new KMMessage;
  QString str;

  msg->initHeader();

  str = "\n\n----------  " + sForwardStr + "  ----------\n";
  str += "Subject: " + subject() + "\n";
  str += "Date: " + dateStr() + "\n";
  str += "From: " + from() + "\n";
  str += "\n";
  msg->setBody(asQuotedString(str, ""));

  if (strnicmp(subject(), "Fwd:", 4)!=0)
    msg->setSubject("Fwd: " + subject());
  else msg->setSubject(subject());

  setStatus(KMMsgStatusForwarded);

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
#ifdef KRN
  setHeaderField("X-NewsReader", "KRN http://ultra7.unl.edu.ar");
#else
  setHeaderField("X-Mailer", "KMail [version " KMAIL_VERSION "]");
#endif

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
  QString str;
  int i;

  if (!aName || !header.HasField(aName)) return "";
  str = header.FindField(aName)->AsString().c_str();
  i = str.find(':');
  if (i>0) str.remove(0,i+2);
  return str;
}


//-----------------------------------------------------------------------------
void KMMessage::removeHeaderField(const QString aName)
{
  DwHeaders& header = mMsg->Headers();
  DwField* field;

  field = header.FindField(aName);
  if (!field) return;

  header.RemoveField(field);
  mNeedsAssembly = TRUE;
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
  int len;

  dwsrc = mMsg->Body().AsString().c_str();
  switch (cte())
  {
  case DwMime::kCteBase64:
    DwDecodeBase64(dwsrc, dwstr);
    len = dwstr.size();
    resultStr.resize(len+1);
    memcpy((void*)resultStr.data(), (void*)dwstr.c_str(), len);
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
void KMMessage::setBodyEncoded(const QString aStr)
{
  DwString dwsrc(aStr.data(), aStr.size(), 0, aStr.length());
  DwString dwstr;

  switch (cte())
  {
  case DwMime::kCteBase64:
    DwEncodeBase64(dwsrc, dwstr);
    break;
  case DwMime::kCteQuotedPrintable:
    DwEncodeQuotedPrintable(dwsrc, dwstr);
    break;
  default:
    dwstr = dwsrc;
    break;
  }

  mMsg->Body().FromString(dwstr);
  mNeedsAssembly = TRUE;
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
    if (!headers->ContentType().Name().empty())
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
const QString KMMessage::emailAddrAsAnchor(const QString aEmail, bool stripped)
{
  QString result, addr;
  const char *pos;
  char ch;
  QString email = decodeQuotedPrintableString(aEmail);

  if (email.isEmpty()) return email;

  result = "<A HREF=\"mailto:";
  for (pos=email.data(); *pos; pos++)
  {
    ch = *pos;
    if (ch == '"') addr += "'";
    else if (ch == '<') addr += "&lt;";
    else if (ch == '>') addr += "&gt;";
    else if (ch == '&') addr += "&amp;";
    else if (ch != ',') addr += ch;

    if (ch == ',' || !pos[1])
    {
      result += addr;
      result += "\">";
      if (stripped) result += KMMessage::stripEmailAddr(addr);
      else result += addr;
      result += "</A>";
      if (ch == ',')
      {
	result += ", <A HREF=\"mailto:";
	while (pos[1]==' ') pos++;
      }
      addr[0] = '\0';
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
void KMMessage::readConfig(void)
{

  /* Default values added for KRN otherwise createReply() segfaults*/
  /* They are taken from kmail's dialog */

  KConfig *config=kapp->getConfig();
    
  config->setGroup("KMMessage");
  sReplyStr = config->readEntry("phrase-reply",nls->translate("On %D, you wrote:"));
  sReplyAllStr = config->readEntry("phrase-reply-all",nls->translate("On %D, %F wrote:"));
  sForwardStr = config->readEntry("phrase-forward",nls->translate("Forwarded Message"));
  sIndentPrefixStr = config->readEntry("indent-prefix",">");
}
