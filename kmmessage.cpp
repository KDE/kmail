// kmmessage.cpp


// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#define ALLOW_GUI 1
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmreaderwin.h"
#include <kpgp.h>
#include <kdebug.h>

#include "kmfolder.h"
#include "kmundostack.h"
#include "kmversion.h"
#include "kmidentity.h"

#include <kapplication.h>
#include <kglobalsettings.h>
#include <khtml_part.h>

// we need access to the protected member DwBody::DeleteBodyParts()...
#define protected public
#include <mimelib/body.h>
#undef protected
#include <mimelib/field.h>

#include <mimelib/mimepp.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <klocale.h>
#include <kglobal.h>
#include <kwin.h>


#if ALLOW_GUI
#include <kmessagebox.h>
#include <qmultilineedit.h>
#endif

static DwString emptyString("");

// Values that are set from the config file with KMMessage::readConfig()
static QString sReplyLanguage, sReplyStr, sReplyAllStr, sIndentPrefixStr, sMessageIdSuffix;
static bool sSmartQuote, sReplaceSubjPrefix, sReplaceForwSubjPrefix, sCreateOwnMessageIdHeaders;
static int sWrapCol;
static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
static QStringList sPrefCharsets;

QString KMMessage::sForwardStr = "";
int KMMessage::sHdrStyle = KMReaderWin::HdrFancy;


//-----------------------------------------------------------------------------
#if 0
KMMessage::KMMessage(DwMessage* aMsg)
  : mMsg(aMsg),
    mNeedsAssembly(true),
    mIsComplete(false),
    mTransferInProgress(false),
    mDecodeHTML(false),
    mCodec(0)
{
}
#endif

//-----------------------------------------------------------------------------
void KMMessage::setReferences(const QCString& aStr)
{
  if (!aStr) return;
  mMsg->Headers().References().FromString(aStr);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
QCString KMMessage::id(void) const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasMessageId())
    return header.MessageId().AsString().c_str();
  else
    return "";
}


//-----------------------------------------------------------------------------
unsigned long KMMessage::getMsgSerNum() const
{
  if (mMsgSerNum)
    return mMsgSerNum;
  return KMMsgBase::getMsgSerNum();
}


//-----------------------------------------------------------------------------
void KMMessage::setMsgSerNum(unsigned long newMsgSerNum)
{
  if (newMsgSerNum)
    mMsgSerNum = newMsgSerNum;
  else if (!mMsgSerNum)
      mMsgSerNum = getMsgSerNum();
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMFolder* parent): KMMessageInherited(parent)
{
  mNeedsAssembly = FALSE;
  mMsg = new DwMessage;
  mCodec = NULL;
  mDecodeHTML = FALSE;
  mIsComplete = FALSE;
  mTransferInProgress = FALSE;
  mMsgSize = 0;
  mMsgLength = 0;
  mFolderOffset = 0;
  mStatus  = KMMsgStatusNew;
  mDate    = 0;
  mFileName = "";
  mMsgSerNum = 0;
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMMsgInfo& msgInfo): KMMessageInherited()
{
  mNeedsAssembly = FALSE;
  mMsg = new DwMessage;
  mCodec = NULL;
  mDecodeHTML = FALSE;
  mIsComplete = FALSE;
  mTransferInProgress = FALSE;
  mMsgSize = msgInfo.msgSize();
  mMsgLength = 0;
  mFolderOffset = msgInfo.folderOffset();
  mStatus = msgInfo.status();
  mDate = msgInfo.date();
  mFileName = msgInfo.fileName();
  mMsgSerNum = msgInfo.getMsgSerNum();
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
QCString KMMessage::asString(void)
{
  if (mNeedsAssembly)
  {
    mNeedsAssembly = FALSE;
    mMsg->Assemble();
  }
  return mMsg->AsString().c_str();
}


//-----------------------------------------------------------------------------
QCString KMMessage::asSendableString()
{
  KMMessage msg;
  msg.fromString(asString());
  msg.removeHeaderField("Status");
  msg.removeHeaderField("X-Status");
  msg.removeHeaderField("X-KMail-Transport");
  msg.removeHeaderField("X-KMail-Identity");
  msg.removeHeaderField("X-KMail-Fcc");
  msg.removeHeaderField("X-KMail-Redirect-From");
  msg.removeHeaderField("X-KMail-Link-Message");
  msg.removeHeaderField("X-KMail-Link-Type");
  msg.removeHeaderField("Bcc");
  return msg.asString();
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
QString KMMessage::headerAsString(void) const
{
  DwHeaders& header = mMsg->Headers();
  if(header.AsString() != "")
    return header.AsString().c_str();
  return "";
}


//-----------------------------------------------------------------------------
void KMMessage::fromString(const QCString& aStr, bool aSetStatus)
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
  mMsgLength = len;
  result.resize(len+1);
  strPos = aStr.data();
  resultPos = (char*)result.data();
  if (strPos) for (; (ch=*strPos)!='\0'; strPos++)
  {
//  Mail header charset(iso-2022-jp) is using all most E-mail system in Japan.
//  ISO-2022-JP code consists of ESC(0x1b) character and 7Bit character which
//  used from '!' character to  '~' character.  toyo
    if ((ch>=' ' || ch=='\t' || ch=='\n' || ch<='\0' || ch == 0x1b)
       && !(ch=='>' && strPos > aStr.data()
            && qstrncmp(strPos-1, "\n>From", 6) == 0))
      *resultPos++ = ch;
  }
  *resultPos = '\0'; // terminate zero for casting
  mMsg->FromString((const char*)result);
  mMsg->Parse();

  if (aSetStatus)
    setStatus(headerField("Status").latin1(), headerField("X-Status").latin1());

  mNeedsAssembly = FALSE;
    mDate = date();

  // Convert messages with a binary body into a message with attachment.
  QCString ct = mMsg->Headers().ContentType().TypeStr().c_str();
  ct = ct.lower();
  if (ct.isEmpty() || ct == "text" || ct == "multipart")
    return;
  KMMessagePart textPart;
  textPart.setTypeStr("text");
  textPart.setSubtypeStr("plain");
  textPart.setBody("\n");
  KMMessagePart bodyPart;
  bodyPart.setTypeStr(ct);
  bodyPart.setSubtypeStr(subtypeStr());
  bodyPart.setContentDisposition(headerField("Content-Disposition").latin1());
  bodyPart.setCteStr(contentTransferEncodingStr());
  bodyPart.setContentDisposition(headerField("Content-Disposition").latin1());
  bodyPart.setBodyEncodedBinary(bodyDecodedBinary());
  addBodyPart(&textPart);
  addBodyPart(&bodyPart);
  mNeedsAssembly = FALSE;
}


//-----------------------------------------------------------------------------
QString KMMessage::formatString(const QString& aStr) const
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
      case 'L':
        result += "\n";
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
  QString startOfSig2 = ownIndent+"--";
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
QCString KMMessage::asQuotedString(const QString& aHeaderStr,
                                   const QString& aIndentStr,
                                   const QString& selection,
                                   bool aStripSignature,
                                   bool allowDecryption) const
{
  QString result;
  QCString cStr;
  QString headerStr;
  KMMessagePart msgPart;
  QRegExp reNL("\\n");
  QString indentStr;
  int i;

  QTextCodec *codec = mCodec;
  if (!codec)
  {
    QCString cset = charset();
    if (!cset.isEmpty())
      codec = KMMsgBase::codecForName(cset);
    if (!codec) codec = QTextCodec::codecForLocale();
  }

  indentStr = formatString(aIndentStr);
  headerStr = formatString(aHeaderStr);


  // Quote message. Do not quote mime message parts that are of other
  // type than "text".
  if (numBodyParts() == 0 || !selection.isEmpty() ) {
    if( !selection.isEmpty() ) {
      result = selection;
    } else {
      cStr = bodyDecoded();
      Kpgp::Module* pgp = Kpgp::Module::getKpgp();
      assert(pgp != NULL);

      if(allowDecryption && (pgp->setMessage(cStr) &&
        ((pgp->isEncrypted() && pgp->decrypt()) || pgp->isSigned())))
      {
        result = codec->toUnicode(pgp->frontmatter())
               + codec->toUnicode(pgp->message())
               + codec->toUnicode(pgp->backmatter());
      } else {
        result = codec->toUnicode(cStr);
      }
      if (mDecodeHTML && qstrnicmp(typeStr(),"text/html",9) == 0)
      {
        KHTMLPart htmlPart;
        htmlPart.setOnlyLocalReferences(true);
        htmlPart.setMetaRefreshEnabled(false);
        htmlPart.setPluginsEnabled(false);
        htmlPart.setJScriptEnabled(false);
        htmlPart.setJavaEnabled(false);
        htmlPart.begin();
        htmlPart.write(result);
        htmlPart.end();
        htmlPart.selectAll();
        result = htmlPart.selectedText();
      }
    }

    // Remove blank lines at the beginning
    for( i = 0; i < (int)result.length() && result[i] <= ' '; i++ );
    while (i > 0 && result[i-1] == ' ') i--;
    result.remove(0,i);

    QString result2((QChar*)NULL, result.length()
      + result.contains('\n') * indentStr.length());
    for (i = 0; i < (int)result.length(); i++)
    {
      result2 += result[i];
      if (result[i] == '\n') result2 += indentStr;
    }
    result = indentStr + result2 + '\n';

    if (sSmartQuote)
      smartQuote(result, '\n' + indentStr, sWrapCol, aStripSignature);
  } else {
    result = "";
    bodyPart(0, &msgPart);

    if (qstricmp(msgPart.typeStr(),"text") == 0 ||
      msgPart.typeStr().isEmpty())
    {
      Kpgp::Module* pgp = Kpgp::Module::getKpgp();
      assert(pgp != NULL);
      QString part;
      if (allowDecryption && ((pgp->setMessage(msgPart.bodyDecoded())) &&
         ((pgp->isEncrypted()) && (pgp->decrypt()) || pgp->isSigned())))
      {
        part = codec->toUnicode(pgp->frontmatter())
             + codec->toUnicode(pgp->message())
             + codec->toUnicode(pgp->backmatter());
      } else {
        part = codec->toUnicode(msgPart.bodyDecoded());
        //	    debug ("part\n" + part ); inexplicably crashes -sanders
      }
      part.replace(reNL, '\n' + indentStr);
      part = indentStr + part + '\n';
      if (sSmartQuote)
        smartQuote(part, '\n' + indentStr, sWrapCol, aStripSignature);
      result += part;
    }
  }

  QCString c = QString(headerStr + result).utf8();

  return c;
}

//-----------------------------------------------------------------------------
KMMessage* KMMessage::createReply(bool replyToAll, bool replyToList,
  QString selection, bool noQuote, bool allowDecryption)
{
  KMMessage* msg = new KMMessage;
  QString str, replyStr, mailingListStr, replyToStr, toStr;
  QCString refStr;

  msg->initFromMessage(this);

  mailingListStr = headerField("X-Mailing-List");
  replyToStr = replyTo();

  msg->setCharset("utf-8");

  if (replyToList && parent()->isMailingList())
  {
      // Reply to mailing-list posting address
      toStr = parent()->mailingListPostAddress();
  }
  else if (replyToAll)
  {
    QStringList recipients;

    // add addresses from the Reply-To header to the list of recipients
    if (!replyToStr.isEmpty())
      recipients += splitEmailAddrList(replyToStr);

    // add From address to the list of recipients if it's not already there
    if (!from().isEmpty())
      if (recipients.grep(getEmailAddr(from()), false).isEmpty()) {
        recipients += from();
        kdDebug(5006) << "Added " << from() << " to the list of recipients"
                      << endl;
      }

    // add only new addresses from the To header to the list of recipients
    if (!to().isEmpty()) {
      QStringList list = splitEmailAddrList(to());
      for (QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if (recipients.grep(getEmailAddr(*it), false).isEmpty()) {
          recipients += *it;
          kdDebug(5006) << "Added " << *it << " to the list of recipients"
                        << endl;
        }
      }
    }

    // strip my own address from the list of recipients
    QString myAddr = getEmailAddr(msg->from());
    for (QStringList::Iterator it = recipients.begin();
         it != recipients.end(); ) {
      if ((*it).find(myAddr,0,false) != -1) {
        kdDebug(5006) << "Removing " << *it << " from the list of recipients"
                      << endl;
        it = recipients.remove(it);
      }
      else
        ++it;
    }

    toStr = recipients.join(", ");

    // the same for the cc field
    if (!cc().isEmpty()) {
      recipients = splitEmailAddrList(cc());

      // strip my own address
      for (QStringList::Iterator it = recipients.begin();
           it != recipients.end(); ) {
        if ((*it).find(myAddr,0,false) != -1) {
          kdDebug(5006) << "Removing " << *it << " from the cc recipients"
                        << endl;
          it = recipients.remove(it);
        }
        else
          ++it;
      }

      msg->setCc(recipients.join(", "));
    }

  }
  else
  {
    if (!replyToStr.isEmpty()) toStr = replyToStr;
    else if (!from().isEmpty()) toStr = from();
  }

  msg->setTo(toStr);

  refStr = getRefStr();
  if (!refStr.isEmpty())
    msg->setReferences(refStr);
  //In-Reply-To = original msg-id
  msg->setReplyToId(msgId());

  if (replyToAll || replyToList || !mailingListStr.isEmpty())
    replyStr = sReplyAllStr;
  else replyStr = sReplyStr;
  replyStr += "\n";

  if (!noQuote)
  msg->setBody(asQuotedString(replyStr, sIndentPrefixStr, selection, true, allowDecryption));

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

  // setStatus(KMMsgStatusReplied);
  msg->link(this, KMMsgStatusReplied);

  return msg;
}


//-----------------------------------------------------------------------------
QCString KMMessage::getRefStr()
{
  QCString firstRef, lastRef, refStr, retRefStr;
  int i, j;

  refStr = headerField("References").stripWhiteSpace().latin1();

  if (refStr.isEmpty())
    return headerField("Message-Id").latin1();

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

  retRefStr += headerField("Message-Id").latin1();
  return retRefStr;
}


KMMessage* KMMessage::createRedirect(void)
{
  KMMessage* msg = new KMMessage;
  KMMessagePart msgPart;
  int i;

  msg->initFromMessage(this);

  /// ### FIXME: The message should be redirected with the same Content-Type
  /// ###        as the original message
  /// ### FIXME: ??Add some Resent-* headers?? (c.f. RFC2822 3.6.6)

  QString st = QString::fromUtf8(asQuotedString("", "", QString::null,
    false, false));
  QCString encoding = autoDetectCharset(charset(), sPrefCharsets, st);
  if (encoding.isEmpty()) encoding = "utf-8";
  QCString str = codecForName(encoding)->fromUnicode(st);

  msg->setCharset(encoding);
  msg->setBody(str);

  if (numBodyParts() > 0)
  {
    msgPart.setBody(str);
    msgPart.setTypeStr("text");
    msgPart.setSubtypeStr("plain");
    msgPart.setCharset(encoding);
    msg->addBodyPart(&msgPart);

    for (i = 0; i < numBodyParts(); i++)
    {
      bodyPart(i, &msgPart);
      if ((qstricmp(msgPart.contentDisposition(),"inline")!=0 && i > 0) ||
	  (qstricmp(msgPart.typeStr(),"text")!=0 &&
	   qstricmp(msgPart.typeStr(),"message")!=0))
      {
	msg->addBodyPart(&msgPart);
      }
    }
  }

//TODO: insert sender here
  msg->setHeaderField("X-KMail-Redirect-From", from());
  msg->setSubject(subject());
  msg->setFrom(from());
  msg->cleanupHeader();
  
  // setStatus(KMMsgStatusForwarded);
  msg->link(this, KMMsgStatusForwarded);

  return msg;
}

#if ALLOW_GUI
KMMessage* KMMessage::createBounce( bool withUI )
#else
KMMessage* KMMessage::createBounce( bool )
#endif
{
  QString fromStr, bodyStr, senderStr;
  int atIdx, i;

  const char* fromFields[] = { "Errors-To", "Return-Path", "Resent-From",
			       "Resent-Sender", "From", "Sender", 0 };

  // Find email address of sender
  for (i=0; fromFields[i]; i++)
  {
    senderStr = headerField(fromFields[i]);
    if (!senderStr.isEmpty()) break;
  }
  if (senderStr.isEmpty())
  {
#if ALLOW_GUI
    if ( withUI )
      KMessageBox::sorry(0 /*app-global modal*/,
			 i18n("The message has no sender set"),
			 i18n("Bounce Message"));
#endif
    return 0;
  }

  QString receiver = headerField("Received");
  int a = -1, b = -1;
  a = receiver.find("from");
  if (a != -1) a = receiver.find("by", a);
  if (a != -1) a = receiver.find("for", a);
  if (a != -1) a = receiver.find('<', a);
  if (a != -1) b = receiver.find('>', a);
  if (a != -1 && b != -1) receiver = receiver.mid(a+1, b-a-1);
  else receiver = getEmailAddr(to());

#if ALLOW_GUI
  if ( withUI )
    // No composer appears. So better ask before sending.
    if (KMessageBox::warningContinueCancel(0 /*app-global modal*/,
        i18n("Return the message to the sender as undeliverable?\n"
	     "This will only work if the email address of the sender,\n"
	     "%1, is valid.\n"
             "The failing address will be reported to be\n%2.")
        .arg(senderStr).arg(receiver),
	i18n("Bounce Message"), i18n("Continue")) == KMessageBox::Cancel)
    {
      return 0;
    }
#endif

  KMMessage *msg = new KMMessage;
  msg->initFromMessage(this, FALSE);
  msg->setTo( senderStr );
  msg->setDateToday();
  msg->setSubject( "mail failed, returning to sender" );

  fromStr = receiver;
  atIdx = fromStr.find('@');
  msg->setFrom( fromStr.replace( 0, atIdx, "MAILER-DAEMON" ) );
  msg->setReferences( id() );

  bodyStr = "|------------------------- Message log follows: -------------------------|\n"
        "no valid recipients were found for this message\n"
	"|------------------------- Failed addresses follow: ---------------------|\n";
  bodyStr += receiver;
  bodyStr += "\n|------------------------- Message text follows: ------------------------|\n";
  bodyStr += asSendableString();

  msg->setBody( bodyStr.latin1() );
  msg->cleanupHeader();

  return msg;
}


//-----------------------------------------------------------------------------
QCString KMMessage::createForwardBody(void)
{
  QString s;
  QCString str;

  if (sHdrStyle == KMReaderWin::HdrAll) {
    s = "\n\n----------  " + sForwardStr + "  ----------\n\n";
    s += headerAsString();
    str = asQuotedString(s, "", QString::null, false, false);
    str += "\n-------------------------------------------------------\n";
  } else {
    s = "\n\n----------  " + sForwardStr + "  ----------\n\n";
    s += "Subject: " + subject() + "\n";
    s += "Date: " + dateStr() + "\n";
    s += "From: " + from() + "\n";
    s += "To: " + to() + "\n";
    if (!cc().isEmpty()) s += "Cc: " + cc() + "\n";
    s += "\n";
    str = asQuotedString(s, "", QString::null, false, false);
    str += "\n-------------------------------------------------------\n";
  }
  
  return str;
}

//-----------------------------------------------------------------------------
KMMessage* KMMessage::createForward(void)
{
  KMMessage* msg = new KMMessage;
  KMMessagePart msgPart;
  QString id;
  int i;

  msg->initFromMessage(this);
  
  QString st = QString::fromUtf8(createForwardBody());
  QCString encoding = autoDetectCharset(charset(), sPrefCharsets, st);
  if (encoding.isEmpty()) encoding = "utf-8";
  QCString str = codecForName(encoding)->fromUnicode(st);

  msg->setCharset(encoding);
  msg->setBody(str);

  if (numBodyParts() > 0)
  {
    msgPart.setTypeStr("text");
    msgPart.setSubtypeStr("plain");
    msgPart.setCharset(encoding);
    msgPart.setBody(str);
    msg->addBodyPart(&msgPart);

    for (i = 0; i < numBodyParts(); i++)
    {
      bodyPart(i, &msgPart);
      if (i > 0 || qstricmp(msgPart.typeStr(),"text") != 0)
        msg->addBodyPart(&msgPart);
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
  msg->cleanupHeader();

  // setStatus(KMMsgStatusForwarded);
  msg->link(this, KMMsgStatusForwarded);

  return msg;
}

KMMessage* KMMessage::createDeliveryReceipt() const
{
  QString str, receiptTo;
  KMMessage *receipt;

  receiptTo = headerField("Disposition-Notification-To");
  if ( receiptTo.stripWhiteSpace().isEmpty() ) return 0;
  receiptTo.replace(QRegExp("\\n"),"");

  receipt = new KMMessage;
  receipt->initFromMessage(this);
  receipt->setTo(receiptTo);
  receipt->setSubject(i18n("Receipt: ") + subject());

  str  = "Your message was successfully delivered.";
  str += "\n\n---------- Message header follows ----------\n";
  str += headerAsString();
  str += "--------------------------------------------\n";
  // Conversion to latin1 is correct here as Mail headers should contain
  // ascii only
  receipt->setBody(str.latin1());
  receipt->setAutomaticFields();

  return receipt;
}

//-----------------------------------------------------------------------------
void KMMessage::initHeader( const QString & id )
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

  if (identStr == i18n("Default"))
    removeHeaderField("X-KMail-Identity");
  else
    setHeaderField("X-KMail-Identity", identStr);

  if (ident.transport().isEmpty())
    removeHeaderField("X-KMail-Transport");
  else
    setHeaderField("X-KMail-Transport", ident.transport());

  if (ident.fcc().isEmpty())
    setFcc( QString::null );
  else
    setFcc( ident.fcc() );

  if (ident.drafts().isEmpty())
    setDrafts( QString::null );
  else
    setDrafts( ident.drafts() );

  if ( !sCreateOwnMessageIdHeaders
       || sMessageIdSuffix.isEmpty()
       || sMessageIdSuffix.isNull() )
    removeHeaderField("Message-Id");
  else {
    QDate date( QDate::currentDate() );
    QTime time( QTime::currentTime() );
    QString msgIdStr;
    msgIdStr = "<";
    msgIdStr += QString::number( date.year() );
    if( 10 > date.month() ) msgIdStr += "0";
    msgIdStr += QString::number( date.month() );
    if( 10 > date.day() )   msgIdStr += "0";
    msgIdStr += QString::number( date.day() );
    if( 10 > time.hour() )  msgIdStr += "0";
    msgIdStr += QString::number( time.hour() );
    if( 10 > time.minute()) msgIdStr += "0";
    msgIdStr += QString::number( time.minute() );
    msgIdStr += ".";
    if( 10 > time.second()) msgIdStr += "0";
    msgIdStr += QString::number( time.second() );
    if( 10 > time.msec() ) {
      if(100 > time.msec() )  msgIdStr += "0";
      msgIdStr += "0";
    }
    msgIdStr += QString::number( time.msec() );
    msgIdStr += "@";
    msgIdStr += sMessageIdSuffix;
    msgIdStr += ">";
    setHeaderField("Message-Id", msgIdStr );
  }


  setTo("");
  setSubject("");
  setDateToday();

  setHeaderField("X-Mailer", "KMail [version " KMAIL_VERSION "]");
// This will allow to change Content-Type:
  setHeaderField("Content-Type","text/plain");
}


//-----------------------------------------------------------------------------
void KMMessage::initFromMessage(const KMMessage *msg, bool idHeaders)
{
  QString id = msg->headerField("X-KMail-Identity");
  if ( id.isEmpty() )
    id = KMIdentity::matchIdentity(msg->to() + " " + msg->cc());
  if ( id.isEmpty() && msg->parent() )
    id = msg->parent()->identity();
  if (idHeaders) initHeader(id);
  else setHeaderField("X-KMail-Identity", id);
  if (!msg->headerField("X-KMail-Transport").isEmpty())
    setHeaderField("X-KMail-Transport", msg->headerField("X-KMail-Transport"));
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
QString KMMessage::dateStr(void) const
{
    return headerField("Date").stripWhiteSpace();
}


//-----------------------------------------------------------------------------
QCString KMMessage::dateShortStr(void) const
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
QString KMMessage::dateIsoStr(void) const
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
    mDate = aDate;
  mMsg->Headers().Date().FromCalendarTime(aDate);
  mMsg->Headers().Date().Assemble();
  mNeedsAssembly = TRUE;
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(const QCString& aStr)
{
  DwHeaders& header = mMsg->Headers();

  header.Date().FromString(aStr);
  header.Date().Parse();
  mNeedsAssembly = TRUE;
  mDirty = TRUE;

  if (header.HasDate())
	mDate = header.Date().AsUnixTime();
}


//-----------------------------------------------------------------------------
QString KMMessage::to(void) const
{
  return headerField("To");
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const QString& aStr)
{
  setHeaderField("To", aStr);
}

//-----------------------------------------------------------------------------
QString KMMessage::toStrip(void) const
{
  return stripEmailAddr(headerField("To"));
}

//-----------------------------------------------------------------------------
QString KMMessage::replyTo(void) const
{
  return headerField("Reply-To");
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
QString KMMessage::cc(void) const
{
  return headerField("Cc");
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const QString& aStr)
{
  setHeaderField("Cc",aStr);
}


//-----------------------------------------------------------------------------
QString KMMessage::bcc(void) const
{
  return headerField("Bcc");
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const QString& aStr)
{
  setHeaderField("Bcc", aStr);
}

//-----------------------------------------------------------------------------
QString KMMessage::fcc(void) const
{
    // we return whatever has something: variable or header. We
    // do it because if the message was in the Outbox, it'd lost
    // the contents of the variable but the header remains.
    QString tmp = headerField( "X-KMail-Fcc" );
    if ( tmp.isEmpty() )
        return mFcc;
    return tmp;
}


//-----------------------------------------------------------------------------
void KMMessage::setFcc(const QString& aStr)
{
  kdDebug(5006) << "KMMessage::setFcc: setting mFcc to " << aStr << endl;
  // we keep this information in the header _and_ in a member variable
  // because eventually the message may be put in the outbox where the
  // variable will be lost. OTOH, when we send the message to the
  // filter, this header will disappear so we must have a variable.
  mFcc = aStr;
  setHeaderField( "X-KMail-Fcc", mFcc );
}

//-----------------------------------------------------------------------------
void KMMessage::setDrafts(const QString& aStr)
{
  mDrafts = aStr;
  kdDebug(5006) << "KMMessage::setDrafts " << aStr << endl;
}

//-----------------------------------------------------------------------------
QString KMMessage::who(void) const
{
  const char* whoField;

  if (mParent) whoField = mParent->whoField();
  else whoField = "From";

  return headerField(whoField);
}


//-----------------------------------------------------------------------------
QString KMMessage::from(void) const
{
  return headerField("From");
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
QString KMMessage::fromStrip(void) const
{
  return stripEmailAddr(headerField("From"));
}

//-----------------------------------------------------------------------------
QCString KMMessage::fromEmail(void) const
{
  return getEmailAddr(headerField("From"));
}


//-----------------------------------------------------------------------------
QString KMMessage::subject(void) const
{
  return headerField("Subject");
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const QString& aStr)
{
  setHeaderField("Subject",aStr);
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
QString KMMessage::xmark(void) const
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
QString KMMessage::replyToId(void) const
{
  int leftAngle, rightAngle;
  QString replyTo, references;

  replyTo = headerField("In-Reply-To");
  // search the end of the (first) message id in the In-Reply-To header
  rightAngle = replyTo.find( '>' );
  if (rightAngle != -1)
    replyTo.truncate( rightAngle + 1 );
  // now search the start of the message id
  leftAngle = replyTo.findRev( '<' );
  if (leftAngle != -1)
    replyTo = replyTo.mid( leftAngle );

  // if we have found a good message id we can return immediately
  if (!replyTo.isEmpty() && (replyTo[0] == '<'))
    return replyTo;

  references = headerField("References");
  leftAngle = references.findRev( '<' );
  if (leftAngle != -1)
    references = references.mid( leftAngle );
  rightAngle = references.find( '>' );
  if (rightAngle != -1)
    references.truncate( rightAngle + 1 );

  // if we found a good message id in the References header return it
  if (!references.isEmpty() && references[0] == '<')
    return references;
  // else return the broken message id we found in the In-Reply-To header
  else
    return replyTo;
}


//-----------------------------------------------------------------------------
QString KMMessage::replyToIdMD5(void) const
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
QString KMMessage::msgId(void) const
{
  int leftAngle, rightAngle;
  QString msgId = headerField("Message-Id");

  // search the end of the message id
  rightAngle = msgId.find( '>' );
  if (rightAngle != -1)
    msgId.truncate( rightAngle + 1 );
  // now search the start of the message id
  leftAngle = msgId.findRev( '<' );
  if (leftAngle != -1)
    msgId = msgId.mid( leftAngle );
  return msgId;
}


//-----------------------------------------------------------------------------
QString KMMessage::msgIdMD5(void) const
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
QStrList KMMessage::headerAddrField(const QCString& aName) const
{
  QString header = headerField(aName);
  QStringList list = splitEmailAddrList(header);
  QStrList resultList;
  int i,j;
  for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
  {
    i = (*it).find('<');
    if (i >= 0)
    {
      j = (*it).find('>', i+1);
      if (j > i) (*it) = (*it).mid(i+1, j-i-1);
    }
    else // if it's "radej@kde.org (Sven Radej)"
    {
      i = (*it).find('(');
      if (i > 0)
        (*it).truncate(i);  // "radej@kde.org "
    }
    (*it) = (*it).stripWhiteSpace();
    if (!(*it).isEmpty())
      resultList.append((*it).latin1());
  }
  return resultList;
}


//-----------------------------------------------------------------------------
QString KMMessage::headerField(const QCString& aName) const
{
  DwHeaders& header = mMsg->Headers();
  DwField* field;
  QString result;

  if (aName.isEmpty() || !(field = header.FindField(aName)))
    result = "";
  else
    result = decodeRFC2047String(header.FieldBody(aName.data()).
                    AsString().c_str());
  return result;
}


//-----------------------------------------------------------------------------
void KMMessage::removeHeaderField(const QCString& aName)
{
  DwHeaders& header = mMsg->Headers();
  DwField* field;

  field = header.FindField(aName);
  if (!field) return;

  header.RemoveField(field);
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setHeaderField(const QCString& aName, const QString& bValue)
{
  DwHeaders& header = mMsg->Headers();
  DwString str;
  DwField* field;
  QCString aValue = "";
  if (!bValue.isEmpty())
  {
    QCString encoding = autoDetectCharset(charset(), sPrefCharsets, bValue);
    if (encoding.isEmpty())
       encoding = "utf-8";
    aValue = encodeRFC2047String(bValue, encoding);
  }

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
QCString KMMessage::typeStr(void) const
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
void KMMessage::setTypeStr(const QCString& aStr)
{
  mMsg->Headers().ContentType().SetTypeStr(DwString(aStr));
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
QCString KMMessage::subtypeStr(void) const
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
void KMMessage::setSubtypeStr(const QCString& aStr)
{
  mMsg->Headers().ContentType().SetSubtypeStr(DwString(aStr));
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
QCString KMMessage::contentTransferEncodingStr(void) const
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
void KMMessage::setContentTransferEncodingStr(const QCString& aStr)
{
  mMsg->Headers().ContentTransferEncoding().FromString(aStr);
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
  QPtrList< DwBodyPart > parts;
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
  QPtrList< DwBodyPart > parts;
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
    aPart->setName("");
    headers = &part->Headers();

    // Content-type
    if (headers->HasContentType())
    {
      aPart->setTypeStr(headers->ContentType().TypeStr().c_str());
      aPart->setSubtypeStr(headers->ContentType().SubtypeStr().c_str());
      DwParameter *param=headers->ContentType().FirstParameter();
      while(param)
      {
        if (!qstricmp(param->Attribute().c_str(), "charset"))
          aPart->setCharset(QCString(param->Value().c_str()).lower());
        else if (param->Attribute().c_str()=="name*")
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
        aPart->setName(KMMsgBase::decodeRFC2047String(headers->
          ContentType().Name().c_str()) );
      else if (!headers->Subject().AsString().empty())
        aPart->setName( KMMsgBase::decodeRFC2047String(headers->
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
void KMMessage::deleteBodyParts(void)
{
  mMsg->Body().DeleteBodyParts();
}


//-----------------------------------------------------------------------------
void KMMessage::addBodyPart(const KMMessagePart* aPart)
{
  QCString charset  = aPart->charset();

  DwBodyPart* part = DwBodyPart::NewBodyPart(emptyString, 0);

  QCString type     = aPart->typeStr();
  QCString subtype  = aPart->subtypeStr();
  QCString cte      = aPart->cteStr();
  QCString contDesc = aPart->contentDescriptionEncoded();
  QCString contDisp = aPart->contentDisposition();
  QCString encoding = autoDetectCharset(charset, sPrefCharsets, aPart->name());
  if (encoding.isEmpty()) encoding = "utf-8";
  QCString name     = KMMsgBase::encodeRFC2231String(aPart->name(), encoding);
  bool RFC2231encoded = aPart->name() != QString(name);
  QCString paramAttr  = aPart->parameterAttribute();

  DwHeaders& headers = part->Headers();
  if (type != "" && subtype != "")
  {
    headers.ContentType().SetTypeStr(type.data());
    headers.ContentType().SetSubtypeStr(subtype.data());
    if (!charset.isEmpty()){
         DwParameter *param;
         param=new DwParameter;
         param->SetAttribute("charset");
         param->SetValue(charset.data());
         headers.ContentType().AddParameter(param);
    }
  }

  if (RFC2231encoded)
  {
    DwParameter *nameParam;
    nameParam = new DwParameter;
    nameParam->SetAttribute("name*");
    nameParam->SetValue(name.data());
    headers.ContentType().AddParameter(nameParam);
  } else {
    if(!name.isEmpty())
      headers.ContentType().SetName(name.data());
  }

  if (!paramAttr.isEmpty())
  {
    QCString encoding = autoDetectCharset(charset, sPrefCharsets,
      aPart->parameterValue());
    if (encoding.isEmpty()) encoding = "utf-8";
    QCString paramValue;
    paramValue = KMMsgBase::encodeRFC2231String(aPart->parameterValue(),
                                                encoding);
    DwParameter *param = new DwParameter;
    if (aPart->parameterValue() != QString(paramValue))
    {
      param->SetAttribute((paramAttr + '*').data());
    } else {
      param->SetAttribute(paramAttr.data());
    }
    param->SetValue(paramValue.data());
    headers.ContentType().AddParameter(param);
  }

  if (!cte.isEmpty())
    headers.Cte().FromString(cte);

  if (!contDesc.isEmpty())
    headers.ContentDescription().FromString(contDesc);

  if (!contDisp.isEmpty())
    headers.ContentDisposition().FromString(contDisp);

  if (!aPart->body().isNull())
    part->Body().FromString(aPart->body());
  else
    part->Body().FromString("");

  mMsg->Body().AddBodyPart(part);

  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::viewSource(const QString& aCaption, QTextCodec *codec, bool fixedfont)
{
  QString str = (codec) ? codec->toUnicode(asString()) :
    QString::fromLocal8Bit(asString());

#if ALLOW_GUI
  QMultiLineEdit *edt;

  edt = new QMultiLineEdit;
  KWin::setIcons(edt->winId(), kapp->icon(), kapp->miniIcon());
  if (!aCaption.isEmpty()) edt->setCaption(aCaption);

  edt->setTextFormat(Qt::PlainText);
  edt->setText(str);
  if (fixedfont)
    edt->setFont(KGlobalSettings::fixedFont());
  edt->setReadOnly(TRUE);

  edt->resize(KApplication::desktop()->width()/2,
	      2*KApplication::desktop()->height()/3);
  edt->setCursorPosition(0, 0);  edt->show();

#else //not ALLOW_GUI
  kdDebug(5006) << "Message source: " << (aCaption.isEmpty() ? "" : (const char*)aCaption) << "\n" << str << "\n--- end of message ---" << endl;

#endif
}


//-----------------------------------------------------------------------------
QString KMMessage::stripEmailAddr(const QString& aStr)
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
QCString KMMessage::getEmailAddr(const QString& aStr)
{
  int a, i, j, len, found = 0;
  QChar c;
  // Find the '@' in the email address:
  a = aStr.find('@');
  if (a<0) return aStr.latin1();
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
  return aStr.mid(i+1,len).latin1();
}


//-----------------------------------------------------------------------------
QString KMMessage::emailAddrAsAnchor(const QString& aEmail, bool stripped)
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
      if (stripped) result += KMMessage::stripEmailAddr(tmp2).latin1();
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
QStringList KMMessage::splitEmailAddrList(const QString& aStr)
{
  // Features:
  // - always ignores quoted characters
  // - ignores everything (including parentheses and commas)
  //   inside quoted strings
  // - supports nested comments
  // - ignores everything (including double quotes and commas)
  //   inside comments

  QStringList list;

  if (aStr.isEmpty())
    return list;

  QString addr;
  uint addrstart = 0;
  int commentlevel = 0;
  bool insidequote = false;

  for (uint index=0; index<aStr.length(); index++) {
    // the following conversion to latin1 is o.k. because
    // we can safely ignore all non-latin1 characters
    switch (aStr[index].latin1()) {
    case '"' : // start or end of quoted string
      if (commentlevel == 0)
        insidequote = !insidequote;
      break;
    case '(' : // start of comment
      if (!insidequote)
        commentlevel++;
      break;
    case ')' : // end of comment
      if (!insidequote) {
        if (commentlevel > 0)
          commentlevel--;
        else {
          kdDebug(5006) << "Error in address splitting: Unmatched ')'"
                        << endl;
          return list;
        }
      }
      break;
    case '\\' : // quoted character
      index++; // ignore the quoted character
      break;
    case ',' :
      if (!insidequote && (commentlevel == 0)) {
        addr = aStr.mid(addrstart, index-addrstart);
        kdDebug(5006) << "Found address: " << addr << endl;
        if (!addr.isEmpty())
          list += addr.simplifyWhiteSpace();
        addrstart = index+1;
      }
      break;
    }
  }
  // append the last address to the list
  if (!insidequote && (commentlevel == 0)) {
    addr = aStr.mid(addrstart, aStr.length()-addrstart);
    kdDebug(5006) << "Found address: " << addr << endl;
    if (!addr.isEmpty())
      list += addr.simplifyWhiteSpace();
  }
  else
    kdDebug(5006) << "Error in address splitting: "
                  << "Unexpected end of address list"
                  << endl;

  return list;
}


//-----------------------------------------------------------------------------
void KMMessage::readConfig(void)
{
  KConfig *config=kapp->config();
  KConfigGroupSaver saver(config, "General");

  config->setGroup("General");

  sCreateOwnMessageIdHeaders = config->readBoolEntry( "createOwnMessageIdHeaders",
						      false );
  sMessageIdSuffix = config->readEntry( "myMessageIdSuffix", "" );

  int languageNr = config->readNumEntry("reply-current-language",0);

  { // area for config group "KMMessage #n"
    KConfigGroupSaver saver(config, QString("KMMessage #%1").arg(languageNr));
    sReplyLanguage = config->readEntry("language",KGlobal::locale()->language());
    sReplyStr = config->readEntry("phrase-reply",
      i18n("On %D, you wrote:"));
    sReplyAllStr = config->readEntry("phrase-reply-all",
      i18n("On %D, %F wrote:"));
    sForwardStr = config->readEntry("phrase-forward",
      i18n("Forwarded Message"));
    sIndentPrefixStr = config->readEntry("indent-prefix",">%_");
  }

  { // area for config group "Composer"
    KConfigGroupSaver saver(config, "Composer");
    sReplySubjPrefixes = config->readListEntry("reply-prefixes", ',');
    if (sReplySubjPrefixes.count() == 0)
      sReplySubjPrefixes.append("Re:");
    sReplaceSubjPrefix = config->readBoolEntry("replace-reply-prefix", true);
    sForwardSubjPrefixes = config->readListEntry("forward-prefixes", ',');
    if (sForwardSubjPrefixes.count() == 0)
      sForwardSubjPrefixes.append("Fwd:");
    sReplaceForwSubjPrefix = config->readBoolEntry("replace-forward-prefix", true);

    sSmartQuote = config->readBoolEntry("smart-quote", true);
    sWrapCol = config->readNumEntry("break-at", 78);
    if ((sWrapCol == 0) || (sWrapCol > 78))
      sWrapCol = 78;
    if (sWrapCol < 60)
      sWrapCol = 60;

    sPrefCharsets = config->readListEntry("pref-charsets");
  }

  { // area for config group "Reader"
    KConfigGroupSaver saver(config, "Reader");
    sHdrStyle = config->readNumEntry("hdr-style", KMReaderWin::HdrFancy);
  }
}

QCString KMMessage::defaultCharset()
{
  QCString retval;

  if (!sPrefCharsets.isEmpty())
      retval = sPrefCharsets[0].latin1();

  if (retval.isEmpty()  || (retval == "locale"))
      retval = QCString(KGlobal::locale()->codecForEncoding()->mimeName()).lower();

  if (retval == "jisx0208.1983-0") retval = "iso-2022-jp";
  else if (retval == "ksc5601.1987-0") retval = "euc-kr";
  return retval;
}

const QStringList &KMMessage::preferredCharsets()
{
  return sPrefCharsets;
}

//-----------------------------------------------------------------------------
QCString KMMessage::charset(void) const
{
   DwMediaType &mType=mMsg->Headers().ContentType();
   mType.Parse();
   DwParameter *param=mType.FirstParameter();
   while(param){
      if (!qstricmp(param->Attribute().c_str(), "charset"))
        return param->Value().c_str();
      else param=param->Next();
   }
   return ""; // us-ascii, but we don't have to specify it
}

//-----------------------------------------------------------------------------
void KMMessage::setCharset(const QCString& bStr)
{
   QCString aStr = bStr.lower();
   if (aStr.isNull())
       aStr = "";
   DwMediaType &mType=mMsg->Headers().ContentType();
   mType.Parse();
   DwParameter *param=mType.FirstParameter();
   while(param)
      // FIXME use the mimelib functions here for comparison.
      if (!qstricmp(param->Attribute().c_str(), "charset")) break;
      else param=param->Next();
   if (!param){
      param=new DwParameter;
      param->SetAttribute("charset");
      mType.AddParameter(param);
   }
   param->SetValue(DwString(aStr));
   mType.Assemble();
}

//-----------------------------------------------------------------------------
void KMMessage::setStatus(const KMMsgStatus aStatus, int idx)
{
    if (mStatus == aStatus)
	return;
    KMMsgBase::setStatus(aStatus, idx);
    mStatus = aStatus;
    mDirty = TRUE;
}

//-----------------------------------------------------------------------------
void KMMessage::link(const KMMessage *aMsg, KMMsgStatus aStatus)
{
  Q_ASSERT(aStatus == KMMsgStatusReplied || aStatus == KMMsgStatusForwarded);

  QString message = headerField("X-KMail-Link-Message");
  if (!message.isEmpty())
    message += ",";
  QString type = headerField("X-KMail-Link-Type");
  if (!type.isEmpty())
    type += ",";
  
  message += QString::number(aMsg->getMsgSerNum());
  if (aStatus == KMMsgStatusReplied)
    type += "reply";
  else if (aStatus == KMMsgStatusForwarded)
    type += "forward";
  
  setHeaderField("X-KMail-Link-Message", message);
  setHeaderField("X-KMail-Link-Type", type);
}

//-----------------------------------------------------------------------------
void KMMessage::getLink(int n, ulong *retMsgSerNum, KMMsgStatus *retStatus) const
{
  *retMsgSerNum = 0;
  *retStatus = KMMsgStatusUnknown;

  QString message = headerField("X-KMail-Link-Message");
  QString type = headerField("X-KMail-Link-Type");
  message = message.section(',', n, n);
  type = type.section(',', n, n);

  if (!message.isEmpty() && !type.isEmpty()) {
    *retMsgSerNum = message.toULong();
    if (type == "reply")
      *retStatus = KMMsgStatusReplied;
    else if (type == "forward")
      *retStatus = KMMsgStatusForwarded;
  }
}
