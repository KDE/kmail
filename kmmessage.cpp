// kmmessage.cpp


// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#define ALLOW_GUI 1
#include "kmglobal.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsginfo.h"
#include "kmreaderwin.h"
#include "kpgp.h"

#ifndef KRN
#include "kmfolder.h"
#include "kmundostack.h"
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
#include <kcharsets.h>
#include <kwin.h>

#include "kmmsgpart.h" // for encodeBase64

#if ALLOW_GUI
#include <qmultilineedit.h>
#endif



static DwString emptyString("");

// Values that are set from the config file with KMMessage::readConfig()
static QString sReplyLanguage, sReplyStr, sReplyAllStr, sIndentPrefixStr;
static bool sSmartQuote, sReplaceSubjPrefix, sReplaceForwSubjPrefix,
               sForceReplyCharset;;
static int sWrapCol;
static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;

QString KMMessage::sForwardStr = "";
int KMMessage::sHdrStyle = KMReaderWin::HdrFancy;

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
    return decodeRFC2047String(header.FollowupTo().AsString().c_str());
  else
  {
    if (header.HasNewsgroups())
      return decodeRFC2047String(header.Newsgroups().AsString().c_str());
    else return "";
  }
}

//-----------------------------------------------------------------------------
void KMMessage::setFollowup(const QString& aStr)
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
void KMMessage::setGroups(const QString& aStr)
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
void KMMessage::setReferences(const QString& aStr)
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
const QString KMMessage::refsAsAnchor(const QString& references)
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
  kernel->undoStack()->msgDestroyed( this );
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
  return mMsg->AsString().c_str();
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
void KMMessage::fromString(const QString& aStr, bool aSetStatus)
{
  int len;
  const char* strPos;
  char* resultPos;
  char ch;
  QCString result;

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
  KMMessageInherited::setDate(date());
}


//-----------------------------------------------------------------------------
const QString KMMessage::formatString(const QString& aStr) const
{
  QString result, str;
  QChar ch;
  uint j;

  if (aStr.isEmpty())
    return aStr;

  for (uint i=0; i<aStr.length();) {
    ch = aStr[i++];
    if (ch == '%') {
      ch = aStr[i++];
      QString langSave = KGlobal::locale()->language();
      switch ((char)ch) {
      case 'D':
	/* I'm not too sure about this change. Is it not possible
	   to have a long form of the date used? I don't
	   like this change to a short XX/XX/YY date format.
	   At least not for the default. -sanders */
        {
          QDateTime datetime;
          datetime.setTime_t(date());
          KLocale locale("kmail");
          locale.setLanguage(sReplyLanguage);
          result += locale.formatDateTime(datetime, false);
        }
        break;
      case 'F':
        result += stripEmailAddr(from());
        break;
      case 'f':
        str = stripEmailAddr(from());

        for (j=0; str[j]>' '; j++)
          ;
        for (; j < str.length() && str[j] <= ' '; j++)
          ;
        result += str[0];
        if (str[j]>' ')
          result += str[j];
        else
          if (str[1]>' ')
            result += str[1];
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
    } else
      result += ch;
  }
  return result;
}

static void removeTrailingSpace( QString &line )
{
   int i = line.length()-1;
   while( (i >= 0) && ((line[i] == ' ') || (line[i] == '\t')))
      i--;
   line.truncate( i+1);
}

static QString splitLine( QString &line)
{
    removeTrailingSpace( line );
    int i = 0;
    int j = -1;
    int l = line.length();

    // TODO: Replace tabs with spaces first.

    while(i < l)
    {
       QChar c = line[i];
       if ((c == '>') || (c == ':') || (c == '|'))
          j = i+1;
       else if ((c != ' ') && (c != '\t'))
          break;
       i++;
    }

    if ( j <= 0 )
    {
       return "";
    }
    if ( i == l )
    {
       QString result = line.left(j);
       line = QString::null;
       return result;
    }

    QString result = line.left(j);
    line = line.mid(j);
    return result;
}

static QString flowText(QString &text, const QString& indent, int maxLength)
{
// printf("flowText: \"%s\"\n", text.ascii());
   maxLength--;
   if (text.isEmpty())
   {
      return indent+"<NULL>\n";
   }
   QString result;
   while (1)
   {
      int i;
      if ((int) text.length() > maxLength)
      {
         i = maxLength;
         while( (i >= 0) && (text[i] != ' '))
            i--;
         if (i <= 0)
         {
            // Couldn't break before maxLength.
            i = maxLength;
//            while( (i < (int) text.length()) && (text[i] != ' '))
//               i++;
         }
      }
      else
      {
         i = text.length();
      }

      QString line = text.left(i);
      if (i < (int) text.length())
         text = text.mid(i);
      else
         text = QString::null;

      result += indent + line + '\n';

      if (text.isEmpty())
         return result;
   }
}

static bool flushPart(QString &msg, QStringList &part,
                      const QString &indent, int maxLength)
{
   maxLength -= indent.length();
   if (maxLength < 20) maxLength = 20;

   // Remove empty lines at end of quote
   while ((part.begin() != part.end()) && part.last().isEmpty())
   {
      part.remove(part.fromLast());
   }

//printf("Start of part.\n");

   QString text;
   for(QStringList::Iterator it2 = part.begin();
       it2 != part.end();
       it2++)
   {
      QString line = (*it2);

      if (line.isEmpty())
      {
         if (!text.isEmpty())
            msg += flowText(text, indent, maxLength);
         msg += indent + "\n";
      }
      else
      {
         if (text.isEmpty())
            text = line;
         else
            text += " "+line.stripWhiteSpace();

         if (((int) text.length() < maxLength) || ((int) line.length() < (maxLength-10)))
            msg += flowText(text, indent, maxLength);
      }
   }
   if (!text.isEmpty())
      msg += flowText(text, indent, maxLength);
//printf("End of of part.\n");

   bool appendEmptyLine = true;
   if (!part.count())
     appendEmptyLine = false;

   part.clear();
   return appendEmptyLine;
}


static void smartQuote( QString &msg, const QString &ownIndent,
                        int maxLength, bool aStripSignature )
{
  QStringList part;
  QString startOfSig1;
  QString startOfSig2 = ownIndent+"-- ";
  QString oldIndent;
  bool firstPart = true;
  int i = 0;
  int l = 0;

//printf("Smart Quoting.\n");

  while( startOfSig2.left(1) == "\n")
     startOfSig2 = startOfSig2.mid(1);
  startOfSig1 = startOfSig2 + ' ';

  QStringList lines;
  while ((i = msg.find('\n', l)) != -1)
  {
     if (i == l)
        lines.append(QString::null);
     else
        lines.append(msg.mid(l, i-l));
     l = i+1;
  }
  if (l <= (int)msg.length())
    lines.append(msg.mid(l));
  msg = QString::null;
  for(QStringList::Iterator it = lines.begin();
      it != lines.end();
      it++)
  {
     QString line = *it;

     if (aStripSignature)
     {
        if (line == startOfSig1) break; // Start of signature
        if (line == startOfSig2) break; // Start of malformed signature
     }

     QString indent = splitLine( line );

//     printf("Quoted Line = \"%s\" \"%s\"\n", line.ascii(), indent.ascii());
     if ( line.isEmpty())
     {
        if (!firstPart)
           part.append(QString::null);
        continue;
     };

     if (firstPart)
     {
        oldIndent = indent;
        firstPart = false;
     }

     if (oldIndent != indent)
     {
        QString fromLine;
        // Search if the last non-blank line could be "From" line
        if (part.count() && (oldIndent.length() < indent.length()))
        {
           QStringList::Iterator it2 = part.fromLast();
           while( (it2 != part.end()) && (*it2).isEmpty())
             it2--;

           if ((it2 != part.end()) && ((*it2).right(1) == ":"))
           {
              fromLine = oldIndent + (*it2) + "\n";
              part.remove(it2);
           }
        }
        if (flushPart( msg, part, oldIndent, maxLength))
        {
           if (oldIndent.length() > indent.length())
              msg += indent + "\n";
           else
              msg += oldIndent + "\n";
        }
        if (!fromLine.isEmpty())
        {
           msg += fromLine;
//printf("From = %s", fromLine.ascii());
        }
        oldIndent = indent;
     }
     part.append(line);
  }
  flushPart( msg, part, oldIndent, maxLength);
}


//-----------------------------------------------------------------------------
const QCString KMMessage::asQuotedString(const QString& aHeaderStr,
					const QString& aIndentStr,
					bool aIncludeAttach,
                                        bool aStripSignature) const
{
  QString result;
  QString headerStr;
  KMMessagePart msgPart;
  QRegExp reNL("\\n");
  QString indentStr;
  bool isInline;
  int i;

  QString cset = charset();
  QTextCodec* codec;
  if (!cset.isEmpty() && !cset.isNull())
    codec = KGlobal::charsets()->codecForName(charset());
  else
    codec = QTextCodec::codecForLocale();

  indentStr = formatString(aIndentStr);
  headerStr = formatString(aHeaderStr);


  // Quote message. Do not quote mime message parts that are of other
  // type than "text".
  if (numBodyParts() == 0) {
    Kpgp* pgp = Kpgp::getKpgp();
    assert(pgp != NULL);
    result = codec->toUnicode(bodyDecoded());

    pgp->setMessage(result);
    if(pgp->isEncrypted()) {
      pgp->decrypt();
      result = pgp->message().stripWhiteSpace();
    } else
      result = result.stripWhiteSpace();

    result.replace(reNL, '\n' + indentStr);
    result = indentStr + result + '\n';

    if (sSmartQuote)
      smartQuote(result, '\n' + indentStr, sWrapCol, aStripSignature);
  } else {
    result = "";
    for (i = 0; i < numBodyParts(); i++) {
      bodyPart(i, &msgPart);

      if (i==0)
        isInline = TRUE;
      else
        isInline = (stricmp(msgPart.contentDisposition(), "inline") == 0);

      if (isInline) {
        if (stricmp(msgPart.typeStr(),"text") == 0 ||
            stricmp(msgPart.typeStr(),"message") == 0) {
          Kpgp* pgp = Kpgp::getKpgp();
          assert(pgp != NULL);
          QString part;
          if ((pgp->setMessage(codec->toUnicode(msgPart.bodyDecoded()))) &&
              (pgp->isEncrypted()) &&
              (pgp->decrypt())) {
            part = pgp->message();
          } else {
            part = codec->toUnicode(msgPart.bodyDecoded());
            //	    debug ("part\n" + part ); inexplicably crashes -sanders
          }
          part.replace(reNL, '\n' + indentStr);
          part = indentStr + part + '\n';
          if (sSmartQuote)
            smartQuote(part, '\n' + indentStr, sWrapCol, aStripSignature);
          result += part;
        } else
          isInline = FALSE;
      }
      if (!isInline && aIncludeAttach) {
        result += QString("\n----------------------------------------") +
                  "\nContent-Type: " + msgPart.typeStr() + "/" + msgPart.subtypeStr();
        if (!msgPart.charset().isEmpty())
          result += "; charset=\"" + msgPart.charset() + '"';

        if (!msgPart.name().isEmpty())
          result += "; name=\"" + msgPart.name() + '"';

        result += QString("\nContent-Transfer-Encoding: ")+
                  msgPart.cteStr() + "\nContent-Description: " +
                  msgPart.contentDescription() +
                  "\n----------------------------------------\n";
      }
    }
  }

  QCString c = codec->fromUnicode(headerStr + "\n" + result);

  return c;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createReply(bool replyToAll, bool replyToList)
{
  KMMessage* msg = new KMMessage;
  QString str, replyStr, mailingListStr, replyToStr, toStr, refStr;

  msg->initHeader(headerField("X-KMail-Identity"));
  if (!headerField("X-KMail-Transport").isEmpty())
    msg->setHeaderField("X-KMail-Transport", headerField("X-KMail-Transport"));

  mailingListStr = headerField("X-Mailing-List");
  replyToStr = replyTo();

  msg->setCharset(charset());

  if (replyToList && parent()->isMailingList())
  {
      // Reply to mailing-list posting address
      toStr = parent()->mailingListPostAddress();
  }
  else if (replyToAll)
  {
    int i;
    // sanders only include the replyTo address if it's != to the from address
    QString rep = replyToStr;
    if((i = rep.find("<")) != -1) // just keep <foo@bar.com>
      rep = rep.right(rep.length() + 1 -i );
    if(!replyToStr.isEmpty() && (rep.isEmpty() || from().find(rep) == -1))
      toStr += replyToStr + ", ";

    if (!from().isEmpty()) toStr += from() + ", ";

    // -sanders only include the to address if it's != replyTo address
    if(!to().isEmpty() && (rep.isEmpty() || to().find(rep) == -1))
      toStr += to() + ", ";

    toStr = toStr.simplifyWhiteSpace() + " ";

    // now try to strip my own e-mail adress:
    QString f = msg->from();
    if((i = f.find("<")) != -1) // just keep <foo@bar.com>
      f = f.right(f.length() + 1 -i );
    if((i = toStr.find(f)) != -1)
    {
      int pos1, pos2, quot;
      quot = toStr.findRev("\"", i);
      pos1 = toStr.findRev(", ", i);
      if (pos1 < quot)
      { 
        quot = toStr.findRev("\"", quot - 1);
        pos1 = toStr.findRev(", ", quot);
      }
      if( pos1 == -1 ) pos1 = 0;
      pos2 = toStr.find(", ", i);
      toStr = toStr.left(pos1) + toStr.right(toStr.length() - pos2);
    }
    else
      toStr.truncate(toStr.length()-2);
    // same for the cc field
    QString ccStr = cc().simplifyWhiteSpace() + ", ";
    if((i = ccStr.find(f)) != -1)
    {
      int pos1, pos2, quot;
      quot = ccStr.findRev("\"", i);
      pos1 = ccStr.findRev(", ", i);
      if (pos1 < quot)
      {
        quot = ccStr.findRev("\"", quot - 1);
        pos1 = ccStr.findRev(", ", quot);
      }
      if( pos1 == -1 ) pos1 = 0;
      pos2 = ccStr.find(", ", i);
      ccStr = ccStr.left(pos1) + ccStr.right(ccStr.length() - pos2 - 1); //Daniel
    }
    else
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

  QStringList::Iterator it;
  bool recognized = false;
  for (it = sReplySubjPrefixes.begin(); !recognized && (it != sReplySubjPrefixes.end()); ++it)
  {
    QString prefix = subject().left((*it).length());
    if (prefix.lower() == (*it).lower()) //recognized
    {
      if (!sReplaceSubjPrefix || (prefix == "Re:"))
        msg->setSubject(subject());
      else
      {
        //replace recognized prefix with "Re: "
        //handle crappy subjects Re:  blah blah (note double space)
        int subjStart = (*it).length();
        while (subject()[subjStart].isSpace()) //strip only from beginning
          subjStart++;
        msg->setSubject("Re: " + subject().mid(subjStart,
                                   subject().length() - subjStart));
      }
      recognized = true;
    }
  }
  if (!recognized)
    msg->setSubject("Re: " + subject());

  if ( sForceReplyCharset )
    msg->setCharset("");
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


KMMessage* KMMessage::createRedirect(void)
{
  KMMessage* msg = new KMMessage;
  KMMessagePart msgPart;
  QString str = "";
  int i;

  str = asQuotedString(str, "", FALSE, false);

  msg->setBody(str.latin1());
  if (numBodyParts() > 0)
  {
    msgPart.setBody(str.latin1());
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

//TODO: insert sender here
  msg->setHeaderField("X-KMail-Redirect-From", from());
  msg->setSubject(subject());
  msg->setFrom(from());
  msg->setCharset(charset());
  setStatus(KMMsgStatusForwarded);

  return msg;
}


//-----------------------------------------------------------------------------
KMMessage* KMMessage::createForward(void)
{
  KMMessage* msg = new KMMessage;
  KMMessagePart msgPart;
  QCString str;
  QString s;
  int i;

  msg->initHeader(headerField("X-KMail-Identity"));

  if (sHdrStyle == KMReaderWin::HdrAll) {
    s = "\n\n----------  " + sForwardStr + "  ----------\n";
    s += asString();
    str = asQuotedString(s, "", FALSE, false);
    str += "\n-------------------------------------------------------\n";
  } else {
    s = "\n\n----------  " + sForwardStr + "  ----------\n";
    s += "Subject: " + subject() + "\n";
    s += "Date: " + dateStr() + "\n";
    s += "From: " + from() + "\n";
    s += "To: " + to() + "\n";
    s += "\n";
    str = asQuotedString(s, "", FALSE, false);
    str += "\n-------------------------------------------------------\n";
  }

  if (!charset().isEmpty()) msg->setCharset(charset());
  msg->setBody(str);

  if (numBodyParts() > 0)
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

  QStringList::Iterator it;
  bool recognized = false;
  for (it = sForwardSubjPrefixes.begin(); !recognized && (it != sForwardSubjPrefixes.end()); ++it)
  {
    QString prefix = subject().left((*it).length());
    if (prefix.lower() == (*it).lower()) //recognized
    {
      if (!sReplaceForwSubjPrefix || (prefix == "Fwd:"))
        msg->setSubject(subject());
      else
      {
        //replace recognized prefix with "Fwd: "
        //handle crappy subjects Fwd:  blah blah (note double space)
        int subjStart = (*it).length();
        while (subject()[subjStart].isSpace()) //strip only from beginning
          subjStart++;
        msg->setSubject("Fwd: " + subject().mid(subjStart,
                                   subject().length() - subjStart));
      }
      recognized = true;
    }
  }
  if (!recognized)
    msg->setSubject("Fwd: " + subject());
  setStatus(KMMsgStatusForwarded);

  return msg;
}


//-----------------------------------------------------------------------------
void KMMessage::initHeader( QString id )
{
  QString identStr = i18n( "Default" );
  if (!id.isEmpty() && KMIdentity::identities().contains(id))
    identStr = id;

  KMIdentity ident( identStr );
  ident.readConfig();
  if(ident.fullEmailAddr().isEmpty())
    setFrom("");
  else
    setFrom(ident.fullEmailAddr());

  if(ident.replyToAddr().isEmpty())
    setReplyTo("");
  else
    setReplyTo(ident.replyToAddr());

  if (ident.organization().isEmpty())
    removeHeaderField("Organization");
  else
    setHeaderField("Organization", ident.organization());

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

  QCString result = ctime(&unixTime);

  if (result[result.length()-1]=='\n')
    result.truncate(result.length()-1);

  return result;
}


//-----------------------------------------------------------------------------
const QString KMMessage::dateIsoStr(void) const
{
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();

  char cstr[64];
  strftime(cstr, 63, "%Y-%m-%d %H:%M:%S", localtime(&unixTime));
  return QString(cstr);
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
void KMMessage::setDate(const QString& aStr)
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
  return headerField("To").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const QString& aStr)
{
  setHeaderField("To", aStr);
}

//-----------------------------------------------------------------------------
const QString KMMessage::toStrip(void) const
{
  return stripEmailAddr(headerField("To"));
}

//-----------------------------------------------------------------------------
const QString KMMessage::replyTo(void) const
{
  return headerField("Reply-To").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(const QString& aStr)
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
  return headerField("Cc").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const QString& aStr)
{
  setHeaderField("Cc",aStr);
}


//-----------------------------------------------------------------------------
const QString KMMessage::bcc(void) const
{
  return headerField("Bcc").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const QString& aStr)
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
  return headerField("From").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setFrom(const QString& bStr)
{
  QString aStr = bStr;
  if (aStr.isNull())
      aStr = "";
  setHeaderField("From", aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::fromStrip(void) const
{
  return stripEmailAddr(headerField("From"));
}

//-----------------------------------------------------------------------------
const QString KMMessage::fromEmail(void) const
{
  return getEmailAddr(headerField("From"));
}


//-----------------------------------------------------------------------------
const QString KMMessage::subject(void) const
{
  return headerField("Subject").simplifyWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const QString& aStr)
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
void KMMessage::setXMark(const QString& aStr)
{
  setHeaderField("X-KMail-Mark", aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::replyToId(void) const
{
  int leftAngle, rightAngle;
  QString replyTo, references;

  replyTo = headerField("In-Reply-To");
  rightAngle = replyTo.find( '>' );
  if (rightAngle != -1)
    replyTo.truncate( rightAngle + 1 );

  references = headerField("References");
  leftAngle = references.findRev( '<' );
  if (leftAngle != -1)
    references = references.mid( leftAngle );
  rightAngle = references.find( '>' );
  if (rightAngle != -1)
    references.truncate( rightAngle + 1 );

  if ((replyTo.isEmpty() || replyTo[0] != '<') &&
      !references.isEmpty() && references[0] == '<')
    replyTo = references;

  return replyTo;
}


//-----------------------------------------------------------------------------
const QString KMMessage::replyToIdMD5(void) const
{
  //  QString result = KMMessagePart::encodeBase64( decodeRFC2047String(replyToId()) );
  QString result = KMMessagePart::encodeBase64( replyToId() );
  return result;
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyToId(const QString& aStr)
{
  setHeaderField("In-Reply-To", aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QString KMMessage::msgId(void) const
{
  int rightAngle;
  QString msgId = headerField("Message-Id");

  rightAngle = msgId.find( '>' );
  if (rightAngle != -1)
    msgId.truncate( rightAngle + 1 );
  return msgId;
}


//-----------------------------------------------------------------------------
const QString KMMessage::msgIdMD5(void) const
{
  //  QString result = KMMessagePart::encodeBase64(  decodeRFC2047String(msgId()) );
  QString result = KMMessagePart::encodeBase64( msgId() );
  return result;
}


//-----------------------------------------------------------------------------
void KMMessage::setMsgId(const QString& aStr)
{
  setHeaderField("Message-Id", aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const QStrList KMMessage::headerAddrField(const QString& aName) const
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
    resultList.append(addr->AsString().c_str());
  }

  if (resultList.count()==0)
  {
    str = headerField(aName);
    if (!str.isEmpty()) resultList.append(str);
  }

  return resultList;
}


//-----------------------------------------------------------------------------
const QString KMMessage::headerField(const QString& aName) const
{
  DwHeaders& header = mMsg->Headers();
  DwField* field;
  QString result;

  if (aName.isEmpty() || !(field = header.FindField((const char*)aName)))
    result = "";
  else
    result = decodeRFC2047String(header.FieldBody((const char*)aName).
                    AsString().c_str());
  return result;
}


//-----------------------------------------------------------------------------
void KMMessage::removeHeaderField(const QString& aName)
{
  DwHeaders& header = mMsg->Headers();
  DwField* field;

  field = header.FindField(aName);
  if (!field) return;

  header.RemoveField(field);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setHeaderField(const QString& aName, const QString& bValue)
{
  DwHeaders& header = mMsg->Headers();
  DwString str;
  DwField* field;
  QString aValue = "";
  if (!bValue.isEmpty())
    aValue = encodeRFC2047String(bValue, charset());

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
void KMMessage::setTypeStr(const QString& aStr)
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
void KMMessage::setSubtypeStr(const QString& aStr)
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
void KMMessage::setContentTransferEncodingStr(const QString& aStr)
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
QCString KMMessage::body(void) const
{
  QCString str;
  str = mMsg->Body().AsString().c_str();
  return str;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::bodyDecodedBinary(void) const
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

  int len=dwstr.size();
  QByteArray ba(len);
  memcpy(ba.data(),dwstr.data(),len);
  return ba;
}


//-----------------------------------------------------------------------------
QCString KMMessage::bodyDecoded(void) const
{
  QByteArray raw(bodyDecodedBinary());
  int len=raw.size();
  QCString ba(len+1);
  memcpy(ba.data(),raw.data(),len);
  ba[len] = 0;
  return ba;
}


//-----------------------------------------------------------------------------
void KMMessage::setBodyEncoded(const QCString& aStr)
{
  int len = aStr.length();
  QByteArray ba(len);
  if (len > 0)
    memcpy(ba.data(),aStr.data(),len);
  setBodyEncodedBinary(ba);
}

//-----------------------------------------------------------------------------
void KMMessage::setBodyEncodedBinary(const QByteArray& aStr)
{
  DwString dwSrc(aStr.data(), aStr.size());
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
void KMMessage::setBody(const QCString& aStr)
{
  mMsg->Body().FromString(aStr.data());
  mNeedsAssembly = TRUE;
}


// Patched by Daniel Moisset <dmoisset@grulic.org.ar>
// modified numbodyparts, bodypart to take nested body parts as
// a linear sequence.
// third revision, Sep 26 2000

// this is support structure for traversing tree without recursion

//-----------------------------------------------------------------------------
int KMMessage::numBodyParts(void) const
{
  int count = 0;
  DwBodyPart* part = mMsg->Body().FirstBodyPart();
  QList< DwBodyPart > parts;
  QString mp = "multipart";

  while (part) 
  {
     //dive into multipart messages
     while ( part && part->Headers().HasContentType() &&
             (mp == part->Headers().ContentType().TypeStr().c_str()) )
     {
	 parts.append( part );
	 part = part->Body().FirstBodyPart();
     }
     // this is where currPart->msgPart contains a leaf message part
     count++;
     // go up in the tree until reaching a node with next
     // (or the last top-level node)
     while (part && !(part->Next()) && !(parts.isEmpty()))
     {
	part = parts.getLast();
	parts.removeLast();
     } ;
     
     if (part)
	 part = part->Next();
  }

  return count;
}


//-----------------------------------------------------------------------------
void KMMessage::bodyPart(int aIdx, KMMessagePart* aPart) const
{
  DwBodyPart *part, *curpart;
  QList< DwBodyPart > parts;
  QString mp = "multipart";
  DwHeaders* headers;
  int curIdx = 0;
  // Get the DwBodyPart for this index

  curpart = mMsg->Body().FirstBodyPart();
  part = 0;

  while (curpart && !part) {
     //dive into multipart messages
     while ( curpart && curpart->Headers().HasContentType() &&
             (mp == curpart->Headers().ContentType().TypeStr().c_str()) )
     {
	 parts.append( curpart );
	 curpart = curpart->Body().FirstBodyPart();
     }
     // this is where currPart->msgPart contains a leaf message part
     if (curIdx==aIdx)
        part = curpart;
     curIdx++;
     // go up in the tree until reaching a node with next
     // (or the last top-level node)
     while (curpart && !(curpart->Next()) && !(parts.isEmpty()))
     {
	curpart = parts.getLast();
	parts.removeLast();
     } ;
     if (curpart)
	 curpart = curpart->Next();
  }

  // If the DwBodyPart was found get the header fields and body
  if (part)
  {
    headers = &part->Headers();

    // Content-type
    if (headers->HasContentType())
    {
      aPart->setTypeStr(headers->ContentType().TypeStr().c_str());
      aPart->setSubtypeStr(headers->ContentType().SubtypeStr().c_str());
      DwParameter *param=headers->ContentType().FirstParameter();
      while(param)
      {
        if (QString(param->Attribute().c_str())=="charset")
          aPart->setCharset(param->Value().c_str());
        else if (QString(param->Attribute().c_str())=="name*")
          aPart->setName(KMMsgBase::decodeRFC2231String(
            param->Value().c_str()));
        param=param->Next();
      }
    }
    else
    {
      aPart->setTypeStr("text");      // Set to defaults
      aPart->setSubtypeStr("plain");
    }
    // Modification by Markus
    if (aPart->name().isEmpty())
    {
      if (!headers->ContentType().Name().empty())
        aPart->setName(KMMsgBase::decodeQuotedPrintableString(headers->
          ContentType().Name().c_str()) );
      else if (!headers->Subject().AsString().empty())
        aPart->setName( KMMsgBase::decodeQuotedPrintableString(headers->
          Subject().AsString().c_str()) );
      else
        aPart->setName( i18n("Attachment: ") + QString( "%1" ).arg( aIdx ) );
    }

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
  const DwString charset  = (const char*)aPart->charset();
  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr(type);
    headers.ContentType().SetSubtypeStr(subtype);
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
  QString charset  = aPart->charset();

  DwBodyPart* part = DwBodyPart::NewBodyPart(emptyString, 0);

  QString type     = aPart->typeStr();
  QString subtype  = aPart->subtypeStr();
  QString cte      = aPart->cteStr();
  QString contDesc = KMMsgBase::encodeRFC2047String(aPart->
    contentDescription(), charset);
  QString contDisp = aPart->contentDisposition();
  QString name     = KMMsgBase::encodeRFC2231String(aPart->name(), charset);
  bool RFC2231encoded = aPart->name() != name;

  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr((const char*)type);
    headers.ContentType().SetSubtypeStr((const char*)subtype);
    if (!charset.isEmpty()){
         DwParameter *param;
         param=new DwParameter;
         param->SetAttribute("charset");
         param->SetValue((const char *)charset);
         headers.ContentType().AddParameter(param);
    }
  }

  if (RFC2231encoded)
  {
    DwParameter *nameParam;
    nameParam = new DwParameter;
    nameParam->SetAttribute("name*");
    nameParam->SetValue((const char*)name);
    headers.ContentType().AddParameter(nameParam);
  } else {
    if(!name.isEmpty())
      headers.ContentType().SetName((const char*)name);
  }

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
void KMMessage::viewSource(const QString& aCaption, QTextCodec *codec) const
{
  QString str = ((KMMessage*)this)->asString();
  if (codec) str=codec->toUnicode(str);

#if ALLOW_GUI
  QMultiLineEdit* edt;

  edt = new QMultiLineEdit;
  KWin::setIcons(edt->winId(), kapp->icon(), kapp->miniIcon());
  if (aCaption) edt->setCaption(aCaption);

  edt->insertLine(str);
  edt->setReadOnly(TRUE);
  edt->resize(KApplication::desktop()->width()/2,
	      2*KApplication::desktop()->height()/3);
  edt->setCursorPosition(0, 0);  edt->show();

#else //not ALLOW_GUI
  kdDebug() << "Message source: " << (aCaption.isEmpty() ? "" : (const char*)aCaption) << "\n" << str << "\n--- end of message ---" << endl;

#endif
}


//-----------------------------------------------------------------------------
const QString KMMessage::stripEmailAddr(const QString& aStr)
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
const QString KMMessage::getEmailAddr(const QString& aStr)
{
  int a, i, j, len, found = 0;
  QChar c;
  QString result;
  // Find the '@' in the email address:
  a = aStr.find('@');
  if (a<0) return aStr;
  // Loop backwards until we find '<', '(', ' ', or beginning of string.
  for (i = a - 1; i >= 0; i--) {
    c = aStr[i];
    if (c == '<' || c == '(' || c == ' ') found = 1;
    if (found) break;
  }
  // Reset found for next loop.
  found = 0;
  // Loop forwards until we find '>', ')', ' ', or end of string.
  for (j = a + 1; j < (int)aStr.length(); j++) {
    c = aStr[j];
    if (c == '>' || c == ')' || c == ' ') found = 1;
    if (found) break;
  }
  // Calculate the length and return the result.
  len = j - (i + 1);
  result = aStr.mid(i+1,len);
  return result;
}


//-----------------------------------------------------------------------------
const QString KMMessage::emailAddrAsAnchor(const QString& aEmail, bool stripped)
{
  QCString result, addr, tmp2;
  const char *pos;
  char ch;
  bool insideQuote = false;

  // FIXME: use unicode instead of utf8
  QCString email = aEmail.utf8();

  if (email.isEmpty()) return email;

  result = "<a href='mailto:";
  for (pos=email.data(); *pos; pos++)
  {
    ch = *pos;
    if (ch == '"') insideQuote = !insideQuote;
    if (ch == '<') addr += "&lt;";
    else if (ch == '>') addr += "&gt;";
    else if (ch == '&') addr += "&amp;";
    else if (ch == '\'') addr += "&#39;";
    else if (ch != ',' || insideQuote) addr += ch;

    if (ch != ',' || insideQuote)
      tmp2 += ch;

    if ((ch == ',' && !insideQuote) || !pos[1])
    {
      result += addr;
      result = result.replace(QRegExp("\n"),"");
      result += "'>";
      if (stripped) result += KMMessage::stripEmailAddr(tmp2);
      else result += addr;
      tmp2 = "";
      result += "</a>";
      if (ch == ',')
      {
	result += ", <a href='mailto:";
	while (pos[1]==' ') pos++;
      }
      addr = "";
    }
  }
  return QString::fromUtf8(result);
}


//-----------------------------------------------------------------------------
void KMMessage::readConfig(void)
{
  KConfig *config=kapp->config();

  config->setGroup("General");
  int languageNr = config->readNumEntry("reply-current-language",0);
//  int replyLanguages = config->readNumEntry("reply-languages",1);

  /* Default values added for KRN otherwise createReply() segfaults*/
  /* They are taken from kmail's dialog */

//  for (int i=0; i<replyLanguages; i++)
  {
    config->setGroup(QString("KMMessage #%1").arg(languageNr));
//    if (i == replyLanguages - 1 || config->readEntry("language") == sLanguage)
    {
      sReplyLanguage = config->readEntry("language",KGlobal::locale()->language());
      sReplyStr = config->readEntry("phrase-reply",
        i18n("On %D, you wrote:"));
      sReplyAllStr = config->readEntry("phrase-reply-all",
        i18n("On %D, %F wrote:"));
      sForwardStr = config->readEntry("phrase-forward",
        i18n("Forwarded Message"));
      sIndentPrefixStr = config->readEntry("indent-prefix",">%_");
//      break;
    }
  }
  config->setGroup("Composer");
  sReplySubjPrefixes = config->readListEntry("reply-prefixes", ',');
  if (sReplySubjPrefixes.count() == 0)
    sReplySubjPrefixes.append("Re:");
  sReplaceSubjPrefix = config->readBoolEntry("replace-reply-prefix", true);
  sForwardSubjPrefixes = config->readListEntry("forward-prefixes", ',');
  if (sForwardSubjPrefixes.count() == 0)
    sForwardSubjPrefixes.append("Fwd:");
  sReplaceForwSubjPrefix = config->readBoolEntry("replace-forward-prefix", true);
  sForceReplyCharset = config->readBoolEntry("force-reply-charset", false );

  config->setGroup("Reader");
  sHdrStyle = config->readNumEntry("hdr-style", KMReaderWin::HdrFancy);

  config->setGroup("Composer");
  sSmartQuote = config->readBoolEntry("smart-quote", true);
  sWrapCol = config->readNumEntry("break-at", 78);
  if ((sWrapCol == 0) || (sWrapCol > 78))
     sWrapCol = 78;
  if (sWrapCol < 60)
     sWrapCol = 60;
}


//-----------------------------------------------------------------------------
const QString KMMessage::charset(void) const
{
   DwMediaType &mType=mMsg->Headers().ContentType();
   mType.Parse();
   DwParameter *param=mType.FirstParameter();
   while(param){
      if (param->Attribute()=="charset")
        return QString(param->Value().c_str());
      else param=param->Next();
   }
   return ""; // us-ascii, but we don't have to specify it
}

//-----------------------------------------------------------------------------
void KMMessage::setCharset(const QString& bStr)
{
   QString aStr = bStr;
   if (aStr.isNull())
       aStr = "";
   DwMediaType &mType=mMsg->Headers().ContentType();
   mType.Parse();
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
}
