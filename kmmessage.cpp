// kmmessage.cpp


// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#define ALLOW_GUI 1

#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsginfo.h"
#include "kpgp.h"
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
#include <mimelib/field.h>

#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <klocale.h>
#include <kglobal.h>

#if ALLOW_GUI
#include <qmultilineedit.h>
#endif


// Originally in kmglobal.h, but we want to avoid to depend on it here
extern KMIdentity* identity;


static DwString emptyString("");
static QCString result;

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
  if (header.HasFollowupTo())
    return decodeRFC1522String(header.FollowupTo().AsString().c_str());
  else
  {
    if (header.HasNewsgroups()) 
      return decodeRFC1522String(header.Newsgroups().AsString().c_str());
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
  else return "";
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
            t="<a href=\"news:/"+refsdata+"\">"+t+"</a> ";
            result+=t;
        }
        else
        {
            t.setNum(count++);
            t2=refsdata.left(index+1).stripWhiteSpace();
            t2=t2.mid(1,t2.length()-2);
            t="<a href=\"news:/"+t2+"\">"+t+"</a> ";
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
  result = mMsg->AsString().c_str();
  return result;
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
const QString KMMessage::headerAsString(void) const
{
  DwHeaders& header = mMsg->Headers();
  if(header.AsString() != "")
    return header.AsString().c_str();
  return "";
}

//-----------------------------------------------------------------------------
void KMMessage::fromString(const QString aStr, bool aSetStatus)
{
  int len;
  const char* strPos;
  char* resultPos;
  char ch;

  if (mMsg) delete mMsg;
  mMsg = new DwMessage;

  // copy string and throw out obsolete control characters
  len = aStr.length();
  result.resize(len+1);
  strPos = aStr.data();
  resultPos = (char*)result.data();
  if (strPos) for (; (ch=*strPos)!='\0'; strPos++)
  {
    if (ch>=' ' || ch=='\t' || ch=='\n' || ch<='\0')
      *resultPos++ = ch;
  }
  *resultPos = '\0'; // terminate zero for casting
  mMsg->FromString((const char*)result);
  mMsg->Parse();

  if (aSetStatus)
    setStatus(headerField("Status"), headerField("X-Status"));

  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::formatString(const QString aStr) const
{
  QString result, str;
  const char* pos;
  char ch, cstr[64];
  QDateTime datetime;
  time_t tm;
  int i;

  if (aStr.isEmpty()) return aStr;

  for (pos=aStr.data(); *pos; )
  {
    ch = *pos++;
    if (ch=='%')
    {
      ch = *pos++;
      switch (ch)
      {
      case 'D':
	/* I'm not too sure about this change. Is it not possible
	   to have a long form of the date used? I don't
	   like this change to a short XX/XX/YY date format.
	   At least not for the default. -sanders
	datetime.setTime_t(date());
        result += KGlobal::locale()->formatDate(datetime.date());
	*/
	tm = date();
	strftime(cstr, 63, "%a, %d %b %Y", localtime(&tm));
	result += cstr;
	break;
      case 'F':
	result += stripEmailAddr(from());
	break;
      case 'f':
	str = stripEmailAddr(from());
	for (i=0; str[i]>' '; i++)
	  ;
	for (; i < (int)str.length() && str[i] <= ' '; i++)
	  ;
	result += str[0];
	if (str[i]>' ') result += str[i];
	else if (str[1]>' ') result += str[1];
	break;
      case 'S':
	result += subject();
	break;
      case '_':
	result += ' ';
	break;
      case '%':
	result += '%';
	break;
      default:
	result += '%';
	result += ch;
	break;
      }
    }
    else result += ch;
  }
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMessage::asQuotedString(const QString aHeaderStr,
					const QString aIndentStr,
					bool aIncludeAttach) const
{
  QString result;
  QString headerStr;
  KMMessagePart msgPart;
  QRegExp reNL("\\n");
  QString nlIndentStr;
  bool isInline;
  int i;

  nlIndentStr = "\n" + formatString(aIndentStr);
  headerStr = formatString(aHeaderStr);

  // Quote message. Do not quote mime message parts that are of other
  // type than "text".
  if (numBodyParts() == 0)
  {
     Kpgp* pgp = Kpgp::getKpgp();
     assert(pgp != NULL);
     result = bodyDecoded();
     pgp->setMessage(result);
     if(pgp->isEncrypted())
     {
       pgp->decrypt();
       result = QString(pgp->message()).stripWhiteSpace();
     } else {
       result = result.stripWhiteSpace();
     }
     
     result.replace(reNL,nlIndentStr) + '\n';
  }
  else
  {
    result = "";
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
	  result += nlIndentStr;
          Kpgp* pgp = Kpgp::getKpgp();
          assert(pgp != NULL);
	  QString part;
          if ((pgp->setMessage(msgPart.bodyDecoded())) &&
              (pgp->isEncrypted()) &&
              (pgp->decrypt()))
	  {
	    part = QString(pgp->message());
	    part = part.replace(reNL,nlIndentStr);
	  }
          else
	  {
	    part = QString(msgPart.bodyDecoded());
	    //	    debug ("part\n" + part ); inexplicably crashes -sanders
	    part = part.replace(reNL,nlIndentStr);
	  }
	  result += part + '\n';
	}
	else isInline = FALSE;
      }
      if (!isInline && aIncludeAttach)
      {
	result += QString("\n----------------------------------------") +
	  "\nContent-Type: " + msgPart.typeStr() + "/" + msgPart.subtypeStr();
	if (!msgPart.name().isEmpty())
	  result += "; name=\"" + msgPart.name() + '"';

	result += QString("\nContent-Transfer-Encoding: ")+ 
	  msgPart.cteStr() + "\nContent-Description: " + 
	  msgPart.contentDescription() +
	  "\n----------------------------------------\n";
      }
    }
  }

  return headerStr + nlIndentStr + result;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createReply(bool replyToAll)
{
  KMMessage* msg = new KMMessage;
  QString str, replyStr, mailingListStr, replyToStr, toStr, refStr;

  msg->initHeader();

  mailingListStr = headerField("X-Mailing-List");
  replyToStr = replyTo();

  if (replyToAll)
  {
    int i;
    if (!replyToStr.isEmpty()) toStr += replyToStr + ", ";
    if (!from().isEmpty()) toStr += from() + ", ";
    if (!to().isEmpty()) toStr += to() + ", ";
    toStr = toStr.simplifyWhiteSpace() + " ";

    // now try to strip my own e-mail adress:
    QString f = msg->from();
    if((i = f.find("<")) != -1) // just keep <foo@bar.com>
      f = f.right(f.length() + 1 -i );
    if((i = toStr.find(f)) != -1)
    {
      int pos1, pos2;
      pos1 = toStr.findRev(", ", i);
      if( pos1 == -1 ) pos1 = 0;
      pos2 = toStr.find(", ", i);
      toStr = toStr.left(pos1) + toStr.right(toStr.length() - pos2); 
    }
    toStr.truncate(toStr.length()-2);
    // same for the cc field
    QString ccStr = cc().simplifyWhiteSpace() + ", ";
    if((i = ccStr.find(f)) != -1)
    {
      int pos1, pos2;
      pos1 = ccStr.findRev(", ", i);
      if( pos1 == -1 ) pos1 = 0;
      pos2 = ccStr.find(", ", i);
      ccStr = ccStr.left(pos1) + toStr.right(ccStr.length() - pos2 - 1); //Daniel
    }
    ccStr.truncate(ccStr.length()-2);

    // remove leading or trailing "," and spaces - might confuse some MTAs
    if (!ccStr.isEmpty())
      {
        ccStr = ccStr.stripWhiteSpace(); //from start and end
        if (ccStr[0] == ',')  ccStr[0] = ' ';
        ccStr = ccStr.simplifyWhiteSpace(); //mAybe it was ",  "
        if (ccStr[ccStr.length()-1] == ',')
          ccStr.truncate(ccStr.length()-1);
	msg->setCc(ccStr);
      }

  }
  else
  {
    if (!replyToStr.isEmpty()) toStr = replyToStr;
    else if (!from().isEmpty()) toStr = from();
  }

  // remove leading or trailing "," and spaces - might confuse some MTAs
  if (!toStr.isEmpty())
    {
      toStr = toStr.stripWhiteSpace(); //from start and end
      if (toStr[0] == ',')  toStr[0] = ' ';
      toStr = toStr.simplifyWhiteSpace(); //maybe it was ",  "
      if (toStr[toStr.length()-1] == ',')
	toStr.truncate(toStr.length()-1);
      msg->setTo(toStr);
    }
  
  refStr = getRefStr();
  if (!refStr.isEmpty())
    msg->setReferences(refStr);
  //In-Reply-To = original msg-id
  msg->setHeaderField("In-Reply-To", headerField("Message-Id"));

  if (replyToAll || !mailingListStr.isEmpty()) replyStr = sReplyAllStr;
  else replyStr = sReplyStr;

  msg->setBody(asQuotedString(replyStr, sIndentPrefixStr));

  if (strnicmp(subject(), "Re:", 3)!=0)
    msg->setSubject("Re: " + subject());
  else msg->setSubject(subject());
#if defined CHARSETS
  printf("Setting reply charset: %s\n",(const char *)charset());
  msg->setCharset(charset());
#endif
  setStatus(KMMsgStatusReplied);

  return msg;
}


//-----------------------------------------------------------------------------
const QString KMMessage::getRefStr()
{
  QString firstRef, lastRef, refStr, retRefStr;
  int i, j;

  refStr = headerField("References").stripWhiteSpace ();

  if (refStr.isEmpty())
    return headerField("Message-Id");

  i = refStr.find("<");
  j = refStr.find(">");
  firstRef = refStr.mid(i, j-i+1);
  if (!firstRef.isEmpty())
    retRefStr = firstRef + " ";

  i = refStr.findRev("<");
  j = refStr.findRev(">");

  lastRef = refStr.mid(i, j-i+1);
  if (!lastRef.isEmpty() && lastRef != firstRef)
    retRefStr += lastRef + " ";

  retRefStr += headerField("Message-Id");
  return retRefStr;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createForward(void)
{
  KMMessage* msg = new KMMessage;
  KMMessagePart msgPart;
  QString str;
  int i;

  msg->initHeader();

  str = "\n\n----------  " + sForwardStr + "  ----------\n";
  str += "Subject: " + subject() + "\n";
  str += "Date: " + dateStr() + "\n";
  str += "From: " + from() + "\n";
  str += "\n";
  str = asQuotedString(str, "", FALSE);
  str += "\n-------------------------------------------------------\n";

  if (numBodyParts() <= 0)
  {
    msg->setBody(str);
  }
  else
  {
    msgPart.setBody(str);
    msgPart.setTypeStr("text");
    msgPart.setSubtypeStr("plain");
    msg->addBodyPart(&msgPart);

    for (i = 1; i < numBodyParts(); i++)
    {
      bodyPart(i, &msgPart);
      if (stricmp(msgPart.contentDisposition(),"inline")!=0 ||
	  (stricmp(msgPart.typeStr(),"text")!=0 &&
	   stricmp(msgPart.typeStr(),"message")!=0))
      {
	msg->addBodyPart(&msgPart);
      }
    }
  }

  if (strnicmp(subject(), "Fwd:", 4)!=0)
    msg->setSubject("Fwd: " + subject());
  else msg->setSubject(subject());
#if defined CHARSETS
  msg->setCharset(charset());
#endif
  setStatus(KMMsgStatusForwarded);

  return msg;
}


//-----------------------------------------------------------------------------
void KMMessage::initHeader(void)
{
  assert(identity != NULL);
  
  if(identity->fullEmailAddr().isEmpty())
    setFrom("");
  else
    setFrom(identity->fullEmailAddr());

  if(identity->replyToAddr().isEmpty()) 
    setReplyTo("");
  else
    setReplyTo(identity->replyToAddr());

  if (identity->organization().isEmpty())
    removeHeaderField("Organization");
  else
    setHeaderField("Organization", identity->organization());

  setTo("");
  setSubject("");
  setDateToday();

#ifdef KRN
  setHeaderField("X-NewsReader", "KRN http://ultra7.unl.edu.ar");
#else
  setHeaderField("X-Mailer", "KMail [version " KMAIL_VERSION "]");
#endif
// This will allow to change Content-Type:
  setHeaderField("Content-Type","text/plain");
}


//-----------------------------------------------------------------------------
void KMMessage::cleanupHeader(void)
{
  DwHeaders& header = mMsg->Headers();
  DwField* field = header.FirstField();
  DwField* nextField;

  if (mNeedsAssembly) mMsg->Assemble();
  mNeedsAssembly = FALSE;

  while (field)
  {
    nextField = field->Next();
    if (field->FieldBody()->AsString().empty())
    {
      header.RemoveField(field);
      mNeedsAssembly = TRUE;
    }
    field = nextField;
  }
}


//-----------------------------------------------------------------------------
void KMMessage::setAutomaticFields(bool aIsMulti)
{
  DwHeaders& header = mMsg->Headers();
  header.MimeVersion().FromString("1.0");
  header.MessageId().CreateDefault();

  if (aIsMulti || numBodyParts() > 1)
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
  return headerField("Date").stripWhiteSpace();
}


//-----------------------------------------------------------------------------
const QString KMMessage::dateShortStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();

  
  result = ctime(&unixTime);
  
  if (result[result.length()-1]=='\n')
    result.truncate(result.length()-1);

  return result;
}


//-----------------------------------------------------------------------------
time_t KMMessage::date(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasDate()) return header.Date().AsUnixTime();
  return (time_t)-1;
}


//-----------------------------------------------------------------------------
void KMMessage::setDateToday(void)
{
  struct timeval tval;
  gettimeofday(&tval, NULL);
  setDate((time_t)tval.tv_sec);
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(time_t aDate)
{
  KMMessageInherited::setDate(aDate);
  mMsg->Headers().Date().FromCalendarTime(aDate);
  mMsg->Headers().Date().Assemble();
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
  return headerField("To");
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const QString aStr)
{
  setHeaderField("To", aStr);
}


//-----------------------------------------------------------------------------
const QString KMMessage::replyTo(void) const
{
  return headerField("Reply-To");
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(const QString aStr)
{
  setHeaderField("Reply-To", aStr);
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(KMMessage* aMsg)
{
  setHeaderField("Reply-To", aMsg->from());
}


//-----------------------------------------------------------------------------
const QString KMMessage::cc(void) const
{
  return headerField("Cc");
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const QString aStr)
{
  setHeaderField("Cc",aStr);
}


//-----------------------------------------------------------------------------
const QString KMMessage::bcc(void) const
{
  return headerField("Bcc");
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const QString aStr)
{
  setHeaderField("Bcc", aStr);
}


//-----------------------------------------------------------------------------
const QString KMMessage::who(void) const
{
  const char* whoField;

  if (mParent) whoField = mParent->whoField();
  else whoField = "From";

  return headerField(whoField);
}


//-----------------------------------------------------------------------------
const QString KMMessage::from(void) const
{
  return headerField("From");
}


//-----------------------------------------------------------------------------
void KMMessage::setFrom(const QString aStr)
{
  setHeaderField("From", aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::subject(void) const
{
  return headerField("Subject").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const QString aStr)
{
  setHeaderField("Subject",aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::xmark(void) const
{
  return headerField("X-KMail-Mark");
}


//-----------------------------------------------------------------------------
void KMMessage::setXMark(const QString aStr)
{
  setHeaderField("X-KMail-Mark", aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QStrList KMMessage::headerAddrField(const QString aName) const
{
  static QStrList resultList;
  DwHeaders& header = mMsg->Headers();
  DwAddressList* addrList;
  DwAddress* addr;
  QString str;

  if (aName.isEmpty()) return resultList;
  addrList = (DwAddressList*)&header.FieldBody((const char*)aName);

  resultList.clear();
  for (addr=addrList->FirstAddress(); addr; addr=addr->Next())
  {
    resultList.append(decodeRFC1522String(addr->AsString().c_str()));
  }

  if (resultList.count()==0)
  {
    str = headerField(aName);
    if (!str.isEmpty()) resultList.append(str);
  }

  return resultList;
}


//-----------------------------------------------------------------------------
const QString KMMessage::headerField(const QString aName) const
{
  DwHeaders& header = mMsg->Headers();
  DwField* field;

  if (aName.isEmpty() || !(field = header.FindField((const char*)aName)))
    result = "";
  else 
    result = decodeRFC1522String(header.FieldBody((const char*)aName).
                    AsString().c_str());  
  return result;
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
  mMsg->Headers().ContentType().Parse();
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
  mMsg->Headers().ContentType().Parse();
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
  mMsg->Headers().ContentTransferEncoding().Parse();
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
  QString result;

  dwsrc = mMsg->Body().AsString().c_str();
  switch (cte())
  {
  case DwMime::kCteBase64:
    DwDecodeBase64(dwsrc, dwstr);
    break;
  case DwMime::kCteQuotedPrintable:
    DwDecodeQuotedPrintable(dwsrc, dwstr);
    break;
  default:
    dwstr = dwsrc;
    break;
  }
  // Should probably be returning a QByteArray, if it may contain NUL
  QCString ba(dwstr.c_str(), dwstr.size() + 1);
  return QString((const QByteArray&)ba);
}


//-----------------------------------------------------------------------------
void KMMessage::setBodyEncoded(const QString aStr)
{
  int len = aStr.length();
  DwString dwSrc(aStr.data(), len);
  DwString dwResult;

  switch (cte())
  {
  case DwMime::kCteBase64:
    DwEncodeBase64(dwSrc, dwResult);
    break;
  case DwMime::kCteQuotedPrintable:
    DwEncodeQuotedPrintable(dwSrc, dwResult);
    break;
  default:
    dwResult = dwSrc;
    break;
  }

  mMsg->Body().FromString(dwResult);
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
#if defined CHARSETS
      DwParameter *param=headers->ContentType().FirstParameter();
      while(param)
          if (param->Attribute()=="charset") break;
          else param=param->Next(); 
      if (param) aPart->setCharset(param->Value().c_str());
#endif       
    }
    else
    {
      aPart->setTypeStr("text");      // Set to defaults
      aPart->setSubtypeStr("plain");
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
#if defined CHARSETS
  const DwString charset  = (const char*)aPart->charset();
#endif
  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr(type);
    headers.ContentType().SetSubtypeStr(subtype);
#if defined CHARSETS
    if (!charset.empty())
    {
      DwParameter *param=headers.ContentType().FirstParameter();
      while(param)
      {
	if (param->Attribute()=="charset") break;
	else param=param->Next();
      }
      if (!param)
      {
	param=new DwParameter;
	param->SetAttribute("charset");
	headers.ContentType().AddParameter(param);
      }   
      param->SetValue(charset);
    }
#endif
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
#if defined CHARSETS
   QString charset  = aPart->charset();
#endif

  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr((const char*)type);
    headers.ContentType().SetSubtypeStr((const char*)subtype);
#if defined CHARSETS
    if (!charset.isEmpty()){
         DwParameter *param;
         param=new DwParameter;
         param->SetAttribute("charset");
         param->SetValue((const char *)charset);
         headers.ContentType().AddParameter(param);
    }
#endif
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
  int i, j, len;
  QString partA, partB, result;
  char endCh = '>';

  i = aStr.find('<');
  if (i<0)
  {
    i = aStr.find('(');
    endCh = ')';
  }
  if (i<0) return aStr;
  partA = aStr.left(i).stripWhiteSpace();
  j = aStr.find(endCh,i+1);
  if (j<0) return aStr;
  partB = aStr.mid(i+1, j-i-1).stripWhiteSpace();

  if (partA.find('@') >= 0 && !partB.isEmpty()) result = partB;
  else if (!partA.isEmpty()) result = partA;
  else result = aStr;

  len = result.length();
  if (result[0]=='"' && result[len-1]=='"')
    result = result.mid(1, result.length()-2);
  else if (result[0]=='<' && result[len-1]=='>')
    result = result.mid(1, result.length()-2);
  else if (result[0]=='(' && result[len-1]==')')
    result = result.mid(1, result.length()-2);
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMessage::emailAddrAsAnchor(const QString aEmail, bool stripped)
{
  QString result, addr, tmp;
  const char *pos;
  char ch;
  QString email = decodeRFC1522String(aEmail);

  if (email.isEmpty()) return email;

  result = "<A HREF=\"mailto:";
  for (pos=email.data(); *pos; pos++)
  {
    ch = *pos;
    if (ch == '<') addr += "&lt;";
    else if (ch == '>') addr += "&gt;";
    else if (ch == '&') addr += "&amp;";
    else if (ch != ',') addr += ch;

    if (ch == ',' || !pos[1])
    {
      tmp = addr.copy();
      result += tmp.replace(QRegExp("\""),"");
      result += "\">";
      if (stripped) result += KMMessage::stripEmailAddr(aEmail);
      else result += addr;
      result += "</A>";
      if (ch == ',')
      {
	result += ", <A HREF=\"mailto:";
	while (pos[1]==' ') pos++;
      }
      addr = "";
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
void KMMessage::readConfig(void)
{

  /* Default values added for KRN otherwise createReply() segfaults*/
  /* They are taken from kmail's dialog */

  KConfig *config=kapp->config();
    
  config->setGroup("KMMessage");
  sReplyStr = config->readEntry("phrase-reply",i18n("On %D, you wrote:"));
  sReplyAllStr = config->readEntry("phrase-reply-all",i18n("On %D, %F wrote:"));
  sForwardStr = config->readEntry("phrase-forward",i18n("Forwarded Message"));
  sIndentPrefixStr = config->readEntry("indent-prefix",">%_");
}

#if defined CHARSETS
//-----------------------------------------------------------------------------
const QString KMMessage::charset(void) const
{
   printf("Checking charset...\n");
   DwMediaType &mType=mMsg->Headers().ContentType();
   mType.Parse();
   DwParameter *param=mType.FirstParameter();
   while(param){
      printf("%s=%s\n",param->Attribute().c_str(),param->Value().c_str());
      if (param->Attribute()=="charset")
        return QString(param->Value().c_str());
      else param=param->Next(); 
   }   
   return ""; // us-ascii, but we don't have to specify it
}

//-----------------------------------------------------------------------------
void KMMessage::setCharset(const QString aStr)
{
   printf("Setting charset to: %s\n",(const char *)aStr);
   DwMediaType &mType=mMsg->Headers().ContentType();
   mType.Parse();
   printf("mType: %s\n",mType.AsString().c_str());
   DwParameter *param=mType.FirstParameter();
   while(param)
      if (param->Attribute()=="charset") break;
      else param=param->Next(); 
   if (!param){
      param=new DwParameter;
      param->SetAttribute("charset");
      mType.AddParameter(param);
   }
   param->SetValue((const char *)aStr);
   mType.Assemble();
   printf("mType: %s\n",mType.AsString().c_str());
}		
#endif
