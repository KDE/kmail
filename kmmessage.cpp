// kmmessage.cpp

// if you do not want GUI elements in here then set ALLOW_GUI to 0.
#include <config.h>

#define ALLOW_GUI 1
#include "kmmessage.h"
#include "mailinglist-magic.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;
#include "kmfolderindex.h"
#include "undostack.h"
#include "kmversion.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "kmkernel.h"
#include "headerstrategy.h"
using KMail::HeaderStrategy;

#include <cryptplugwrapperlist.h>
#include <kpgpblock.h>

#include <kapplication.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kconfig.h>
#include <khtml_part.h>

#include <qcursor.h>
#include <qtextcodec.h>
#include <qmessagebox.h>
#include <kmime_util.h>
#include <kmime_charfreq.h>

#include <kmime_header_parsing.h>
using KMime::HeaderParsing::parseAddressList;
using namespace KMime::Types;

#include <mimelib/body.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>
#include <mimelib/string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <klocale.h>
#include <stdlib.h>

#if ALLOW_GUI
#include <kmessagebox.h>
#endif

// needed temporarily until KMime is replacing the partNode helper class:
#include "partNode.h"

using namespace KMime;

static DwString emptyString("");

// Values that are set from the config file with KMMessage::readConfig()
static QString sReplyLanguage, sReplyStr, sReplyAllStr, sIndentPrefixStr;
static bool sSmartQuote, sReplaceSubjPrefix, sReplaceForwSubjPrefix;
static int sWrapCol;
static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
static QStringList sPrefCharsets;

QString KMMessage::sForwardStr = "";
const HeaderStrategy * KMMessage::sHeaderStrategy = HeaderStrategy::rich();

//-----------------------------------------------------------------------------
KMMessage::KMMessage(DwMessage* aMsg)
  : mMsg(aMsg),
    mNeedsAssembly(true),
    mIsComplete(false),
    mDecodeHTML(false),
    mTransferInProgress(0),
    mOverrideCodec(0),
    mUnencryptedMsg(0)
{
  mEncryptionState = KMMsgEncryptionStateUnknown;
  mSignatureState = KMMsgSignatureStateUnknown;
}

//-----------------------------------------------------------------------------
KMMessage::KMMessage(const KMMessage& other) : KMMessageInherited( other ), mMsg(0)
{
  mUnencryptedMsg = 0;
  assign( other );
}

void KMMessage::assign( const KMMessage& other )
{
  delete mMsg;
  delete mUnencryptedMsg;

  mNeedsAssembly = true;//other.mNeedsAssembly;
  if( other.mMsg )
    mMsg = new DwMessage( *(other.mMsg) );
  mOverrideCodec = other.mOverrideCodec;
  mDecodeHTML = other.mDecodeHTML;
  mIsComplete = false;//other.mIsComplete;
  mTransferInProgress = other.mTransferInProgress;
  mMsgSize = other.mMsgSize;
  mMsgLength = other.mMsgLength;
  mFolderOffset = other.mFolderOffset;
  mStatus  = other.mStatus;
  mEncryptionState = other.mEncryptionState;
  mSignatureState = other.mSignatureState;
  mMDNSentState = other.mMDNSentState;
  mDate    = other.mDate;
  if( other.hasUnencryptedMsg() )
    mUnencryptedMsg = new KMMessage( *other.unencryptedMsg() );
  else
    mUnencryptedMsg = 0;
  //mFileName = ""; // we might not want to copy the other messages filename (?)
  //mMsgSerNum = other.mMsgSerNum; // what about serial number ?
  mMsgSerNum = 0;
  //KMMsgBase::assign( &other );
}

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
KMMessage::KMMessage(KMFolderIndex* parent): KMMessageInherited(parent)
{
  mNeedsAssembly = FALSE;
  mMsg = new DwMessage;
  mOverrideCodec = 0;
  mDecodeHTML = FALSE;
  mIsComplete = FALSE;
  mTransferInProgress = 0;
  mMsgSize = 0;
  mMsgLength = 0;
  mFolderOffset = 0;
  mStatus  = KMMsgStatusNew;
  mEncryptionState = KMMsgEncryptionStateUnknown;
  mSignatureState = KMMsgSignatureStateUnknown;
  mMDNSentState = KMMsgMDNStateUnknown;
  mDate    = 0;
  mFileName = "";
  mMsgSerNum = 0;
  mUnencryptedMsg = 0;
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMMsgInfo& msgInfo): KMMessageInherited()
{
  mNeedsAssembly = FALSE;
  mMsg = new DwMessage;
  mOverrideCodec = 0;
  mDecodeHTML = FALSE;
  mIsComplete = FALSE;
  mTransferInProgress = 0;
  mMsgSize = msgInfo.msgSize();
  mMsgLength = 0;
  mFolderOffset = msgInfo.folderOffset();
  mStatus = msgInfo.status();
  mEncryptionState = msgInfo.encryptionState();
  mSignatureState = msgInfo.signatureState();
  mMDNSentState = msgInfo.mdnSentState();
  mDate = msgInfo.date();
  mFileName = msgInfo.fileName();
  mMsgSerNum = msgInfo.getMsgSerNum();
  KMMsgBase::assign(&msgInfo);
  mUnencryptedMsg = 0;
}


//-----------------------------------------------------------------------------
KMMessage::~KMMessage()
{
  Q_ASSERT( !transferInProgress() );
  delete mMsg;
  kernel->undoStack()->msgDestroyed( this );
}


//-----------------------------------------------------------------------------
bool KMMessage::isMessage(void) const
{
  return TRUE;
}

bool KMMessage::isUrgent() const {
  return headerField( "Priority" ).contains( "urgent", false )
    || headerField( "X-Priority" ).startsWith( "2" );
}

//-----------------------------------------------------------------------------
void KMMessage::setUnencryptedMsg( KMMessage* unencrypted )
{
  delete mUnencryptedMsg;
  mUnencryptedMsg = unencrypted;
}

//-----------------------------------------------------------------------------
const DwString& KMMessage::asDwString() const
{
  if (mNeedsAssembly)
  {
    mNeedsAssembly = FALSE;
    mMsg->Assemble();
  }
  return mMsg->AsString();
}

//-----------------------------------------------------------------------------
const DwMessage *KMMessage::asDwMessage(void)
{
  if (mNeedsAssembly)
  {
    mNeedsAssembly = FALSE;
    mMsg->Assemble();
  }
  return mMsg;
}

//-----------------------------------------------------------------------------
QCString KMMessage::asString() const {
  return asDwString().c_str();
}


QCString KMMessage::asSendableString() const
{
  KMMessage msg;
  msg.fromString(asString());
  msg.removePrivateHeaderFields();
  msg.removeHeaderField("Bcc");
  return msg.asString();
}

QCString KMMessage::headerAsSendableString() const
{
  KMMessage msg;
  msg.fromString(asString());
  msg.removePrivateHeaderFields();
  msg.removeHeaderField("Bcc");
  return msg.headerAsString().latin1();
}

void KMMessage::removePrivateHeaderFields() {
  removeHeaderField("Status");
  removeHeaderField("X-Status");
  removeHeaderField("X-KMail-EncryptionState");
  removeHeaderField("X-KMail-SignatureState");
  removeHeaderField("X-KMail-MDN-Sent");
  removeHeaderField("X-KMail-Transport");
  removeHeaderField("X-KMail-Identity");
  removeHeaderField("X-KMail-Fcc");
  removeHeaderField("X-KMail-Redirect-From");
  removeHeaderField("X-KMail-Link-Message");
  removeHeaderField("X-KMail-Link-Type");
}

//-----------------------------------------------------------------------------
void KMMessage::setStatusFields(void)
{
  char str[3];

  setHeaderField("Status", status() & KMMsgStatusNew ? "R \0" : "RO\0");
  setHeaderField("X-Status", statusToStr(status()));

  str[0] = (char)encryptionState();
  str[1] = '\0';
  setHeaderField("X-KMail-EncryptionState", str);

  str[0] = (char)signatureState();
  str[1] = '\0';
  //kdDebug(5006) << "Setting SignatureState header field to " << str[0] << endl;
  setHeaderField("X-KMail-SignatureState", str);

  str[0] = static_cast<char>( mdnSentState() );
  str[1] = '\0';
  setHeaderField("X-KMail-MDN-Sent", str);

  // We better do the assembling ourselves now to prevent the
  // mimelib from changing the message *body*.  (khz, 10.8.2002)
  mNeedsAssembly = false;
  mMsg->Headers().Assemble();
  mMsg->Assemble( mMsg->Headers(),
                  mMsg->Body() );
}


//----------------------------------------------------------------------------
QString KMMessage::headerAsString(void) const
{
  DwHeaders& header = mMsg->Headers();
  header.Assemble();
  if(header.AsString() != "")
    return header.AsString().c_str();
  return "";
}


//-----------------------------------------------------------------------------
DwMediaType& KMMessage::dwContentType(void)
{
  return mMsg->Headers().ContentType();
}

void KMMessage::fromByteArray( const QByteArray & ba, bool setStatus ) {
  return fromDwString( DwString( ba.data(), ba.size() ), setStatus );
}

void KMMessage::fromString( const QCString & str, bool aSetStatus ) {
  return fromDwString( DwString( str.data() ), aSetStatus );
}

void KMMessage::fromDwString(const DwString& str, bool aSetStatus)
{
  const char* strPos = str.data();
  char ch;
  bool needsJpDecode = false;
  delete mMsg;
  mMsg = new DwMessage;
  mMsgLength = str.length();

  if (strPos) for (; strPos < str.data() + str.length(); ++strPos)
  {
    ch = *strPos;
    if (!((ch>=' ' || ch=='\t' || ch=='\n' || ch<='\0' || ch == 0x1b)
	  && !(ch=='>' && strPos > str.data()
	       && qstrncmp(strPos-1, "\n>From", 6) == 0)))
    {
	needsJpDecode = true;
	break;
    }
  }

  if (needsJpDecode) {
  // copy string and throw out obsolete control characters
      char *resultPos;
      int len = str.length();
      char* rawData = new char[ len + 1 ];
      QCString result;
      result.setRawData( rawData, len + 1 );
      strPos = str.data();
      resultPos = (char*)result.data();

      if (strPos) for (; strPos < str.data() + str.length(); ++strPos)
      {
	  ch = *strPos;
//  Mail header charset(iso-2022-jp) is using all most E-mail system in Japan.
//  ISO-2022-JP code consists of ESC(0x1b) character and 7Bit character which
//  used from '!' character to  '~' character.  toyo
	  if ((ch>=' ' || ch=='\t' || ch=='\n' || ch<='\0' || ch == 0x1b)
	      && !(ch=='>' && strPos > str.data()
	      && qstrncmp(strPos-1, "\n>From", 6) == 0))
	      *resultPos++ = ch;
      }
      *resultPos = '\0'; // terminate zero for casting
      DwString jpStr;
      jpStr.TakeBuffer( result.data(), len + 1, 0, result.length() );
      mMsg->FromString( jpStr );
      result.resetRawData( result, len + 1);
  } else {
      mMsg->FromString( str );
  }
  mMsg->Parse();

  if (aSetStatus) {
    setStatus(headerField("Status").latin1(), headerField("X-Status").latin1());
    setEncryptionStateChar( headerField("X-KMail-EncryptionState").at(0) );
    setSignatureStateChar(  headerField("X-KMail-SignatureState").at(0) );
    setMDNSentState( static_cast<KMMsgMDNSentState>( headerField("X-KMail-MDN-Sent").at(0).latin1() ) );
  }

  mNeedsAssembly = FALSE;
    mDate = date();

  // Convert messages with a binary body into a message with attachment.
#if 0
  QCString ct = dwContentType().TypeStr().c_str();
  QCString st = dwContentType().SubtypeStr().c_str();
  ct = ct.lower();
  st = st.lower();
  if (   ct.isEmpty()
      || ct == "text"
      || ct == "multipart"
      || (    ct == "application"
              && (st == "pkcs7-mime" || st == "x-pkcs7-mime" || st == "pgp") ) )
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
#endif
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
      switch ((char)ch) {
      case 'D':
	/* I'm not too sure about this change. Is it not possible
	   to have a long form of the date used? I don't
	   like this change to a short XX/XX/YY date format.
	   At least not for the default. -sanders */
	result += KMime::DateFormatter::formatDate( KMime::DateFormatter::Localized,
						    date(), sReplyLanguage, false );
        break;
      case 'e':
        result += from();
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
      case 'T':
        result += stripEmailAddr(to());
        break;
      case 't':
        result += to();
        break;
      case 'C':
        result += stripEmailAddr(cc());
        break;
      case 'c':
        result += cc();
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
         msg += indent + '\n';
      }
      else
      {
         if (text.isEmpty())
            text = line;
         else
            text += ' '+line.stripWhiteSpace();

         if (((int) text.length() < maxLength) || ((int) line.length() < (maxLength-10)))
            msg += flowText(text, indent, maxLength);
      }
   }
   if (!text.isEmpty())
      msg += flowText(text, indent, maxLength);

   bool appendEmptyLine = true;
   if (!part.count())
     appendEmptyLine = false;

   part.clear();
   return appendEmptyLine;
}

static void stripSignature(QString& msg, bool clearSigned)
{
  if (clearSigned)
  {
    msg = msg.left(msg.findRev(QRegExp("\\n--\\s?\\n")));
  }
  else
  {
    msg = msg.left(msg.findRev("\n-- \n"));
  }
}

static void smartQuote( QString &msg, int maxLength )
{
  QStringList part;
  QString oldIndent;
  bool firstPart = true;


  QStringList lines = QStringList::split('\n', msg, true);

  msg = QString::null;
  for(QStringList::Iterator it = lines.begin();
      it != lines.end();
      it++)
  {
     QString line = *it;

     QString indent = splitLine( line );

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

           if ((it2 != part.end()) && ((*it2).endsWith(":")))
           {
              fromLine = oldIndent + (*it2) + '\n';
              part.remove(it2);
           }
        }
        if (flushPart( msg, part, oldIndent, maxLength))
        {
           if (oldIndent.length() > indent.length())
              msg += indent + '\n';
           else
              msg += oldIndent + '\n';
        }
        if (!fromLine.isEmpty())
        {
           msg += fromLine;
        }
        oldIndent = indent;
     }
     part.append(line);
  }
  flushPart( msg, part, oldIndent, maxLength);
}


//-----------------------------------------------------------------------------
void KMMessage::parseTextStringFromDwPart( DwBodyPart * mainBody,
					   DwBodyPart * firstBodyPart,
                                           QCString& parsedString,
                                           const QTextCodec*& codec,
                                           bool& isHTML ) const
{
  // get a valid CryptPlugList
  CryptPlugWrapperList cryptPlugList;
  KConfig *config = KMKernel::config();
  cryptPlugList.loadFromConfig( config );

  isHTML = false;
  int mainType    = type();
  int mainSubType = subtype();
  if(    (DwMime::kTypeNull    == mainType)
      || (DwMime::kTypeUnknown == mainType) ){
      mainType    = DwMime::kTypeText;
      mainSubType = DwMime::kSubtypePlain;
  }
  partNode rootNode( mainBody, mainType, mainSubType);
  if ( firstBodyPart ) {
    partNode* curNode = rootNode.setFirstChild( new partNode( firstBodyPart ) );
    curNode->buildObjectTree();
  }
  // initialy parse the complete message to decrypt any encrypted parts
  {
    ObjectTreeParser otp( 0, 0, true, false, true );
    otp.parseObjectTree( &rootNode );
  }
  partNode * curNode = rootNode.findType( DwMime::kTypeText,
                               DwMime::kSubtypeUnknown,
                               true,
                               false );
  kdDebug(5006) << "\n\n======= KMMessage::parseTextStringFromDwPart()   -    "
                << QString( curNode ? "text part found!\n" : "sorry, no text node!\n" ) << endl;
  if( curNode ) {
    isHTML = DwMime::kSubtypeHtml == curNode->subType();
    // now parse the TEXT message part we want to quote
    ObjectTreeParser otp( 0, 0, true, false, true );
    otp.parseObjectTree( curNode );
    parsedString = otp.resultString();
    codec = curNode->msgPart().codec();
  }
  kdDebug(5006) << "\n\n======= KMMessage::parseTextStringFromDwPart()   -    parsed string:\n\""
                << QString( parsedString + "\"\n\n" ) << endl;
}

//-----------------------------------------------------------------------------
QCString KMMessage::asQuotedString( const QString& aHeaderStr,
                                    const QString& aIndentStr,
                                    const QString& selection /* = QString::null */,
                                    bool aStripSignature /* = true */,
                                    bool allowDecryption /* = true */) const
{
  QString result;
  QString headerStr;
  QString indentStr;

  indentStr = formatString(aIndentStr);
  headerStr = formatString(aHeaderStr);

  // Quote message. Do not quote mime message parts that are of other
  // type than "text".
  if( !selection.isEmpty() ) {
    result = selection;
  }
  else {
    QCString parsedString;
    bool isHTML = false;
    bool clearSigned = false;
    const QTextCodec * codec = 0;

    if( numBodyParts() == 0 ) {
      DwBodyPart * mainBody = 0;
      DwBodyPart * firstBodyPart = getFirstDwBodyPart();
      if( !firstBodyPart ) {
        mainBody = new DwBodyPart(((KMMessage*)this)->asDwString(), 0);
	mainBody->Parse();
      }
      parseTextStringFromDwPart( mainBody, firstBodyPart, parsedString, codec,
                                 isHTML );
    }
    else {
      DwBodyPart *dwPart = getFirstDwBodyPart();
      if( dwPart )
        parseTextStringFromDwPart( 0, dwPart, parsedString, codec, isHTML );
    }

    codec = this->codec();

    if( !parsedString.isEmpty() ) {
      Kpgp::Module* pgp = Kpgp::Module::getKpgp();
      Q_ASSERT(pgp != 0);

      QPtrList<Kpgp::Block> pgpBlocks;
      QStrList nonPgpBlocks;
      if( allowDecryption &&
          Kpgp::Module::prepareMessageForDecryption( parsedString,
                                                     pgpBlocks,
                                                     nonPgpBlocks ) )
      {
        // Only decrypt/strip off the signature if there is only one OpenPGP
        // block in the message
        if( pgpBlocks.count() == 1 )
        {
          Kpgp::Block* block = pgpBlocks.first();
          if( ( block->type() == Kpgp::PgpMessageBlock ) ||
              ( block->type() == Kpgp::ClearsignedBlock ) )
          {
            if( block->type() == Kpgp::PgpMessageBlock ) {
              // try to decrypt this OpenPGP block
              block->decrypt();
            }
            else {
              // strip off the signature
              block->verify();
              clearSigned = true;
            }

            result = codec->toUnicode( nonPgpBlocks.first() )
                   + codec->toUnicode( block->text() )
                   + codec->toUnicode( nonPgpBlocks.last() );
          }
        }
      }
      if( result.isEmpty() )
        result = codec->toUnicode( parsedString );
    }

    if( !result.isEmpty() && mDecodeHTML && isHTML ) {
      KHTMLPart htmlPart;
      htmlPart.setOnlyLocalReferences( true );
      htmlPart.setMetaRefreshEnabled( false );
      htmlPart.setPluginsEnabled( false );
      htmlPart.setJScriptEnabled( false );
      htmlPart.setJavaEnabled( false );
      htmlPart.begin();
      htmlPart.write( result );
      htmlPart.end();
      htmlPart.selectAll();
      result = htmlPart.selectedText();
    }

    if( aStripSignature )
      stripSignature( result, clearSigned );
  }

  // Remove blank lines at the beginning:
  // 1. find first non space, non linebreak character
  int i = result.find( QRegExp( "[^\\s]" ) );
  // 2. find the start of the current line
  i = result.findRev( "\n", i );
  if( i >= 0 )
    result.remove( 0, (uint)i );

  result.replace( "\n", '\n' + indentStr );
  result = indentStr + result + '\n';

  if( sSmartQuote )
    smartQuote( result, sWrapCol );

  return QString( headerStr + result ).utf8();
}

//-----------------------------------------------------------------------------
// static
QString KMMessage::stripOffPrefixes( const QString& str )
{
  return replacePrefixes( str, sReplySubjPrefixes + sForwardSubjPrefixes,
                          true, QString::null ).stripWhiteSpace();
}

//-----------------------------------------------------------------------------
// static
QString KMMessage::replacePrefixes( const QString& str,
                                    const QStringList& prefixRegExps,
                                    bool replace,
                                    const QString& newPrefix )
{
  bool recognized = false;
  // construct a big regexp that
  // 1. is anchored to the beginning of str (sans whitespace)
  // 2. matches at least one of the part regexps in prefixRegExps
  QString bigRegExp = QString::fromLatin1("^(?:\\s+|(?:%1))+\\s*")
                      .arg( prefixRegExps.join(")|(?:") );
  QRegExp rx( bigRegExp, false /*case insens.*/ );
  if ( !rx.isValid() ) {
    kdWarning(5006) << "KMMessage::replacePrefixes(): bigRegExp = \""
                    << bigRegExp << "\"\n"
                    << "prefix regexp is invalid!" << endl;
    // try good ole Re/Fwd:
    recognized = str.startsWith( newPrefix );
  } else { // valid rx
    QString tmp = str;
    if ( rx.search( tmp ) == 0 ) {
      recognized = true;
      if ( replace )
	return tmp.replace( 0, rx.matchedLength(), newPrefix + ' ' );
    }
  }
  if ( !recognized )
    return newPrefix + ' ' + str;
  else
    return str;
}

//-----------------------------------------------------------------------------
QString KMMessage::cleanSubject() const
{
  return cleanSubject( sReplySubjPrefixes + sForwardSubjPrefixes,
		       true, QString::null ).stripWhiteSpace();
}

//-----------------------------------------------------------------------------
QString KMMessage::cleanSubject( const QStringList & prefixRegExps,
                                 bool replace,
                                 const QString & newPrefix ) const
{
  return KMMessage::replacePrefixes( subject(), prefixRegExps, replace,
                                     newPrefix );
}

//-----------------------------------------------------------------------------
KMMessage* KMMessage::createReply( bool replyToAll /* = false */,
                                   bool replyToList /* = false */,
                                   QString selection /* = QString::null */,
                                   bool noQuote /* = false */,
                                   bool allowDecryption /* = true */,
                                   bool selectionIsBody /* = false */)
{
  KMMessage* msg = new KMMessage;
  QString str, replyStr, mailingListStr, replyToStr, toStr;
  QCString refStr, headerName;

  msg->initFromMessage(this);

  KMMLInfo::name(this, headerName, mailingListStr);
  replyToStr = replyTo();

  msg->setCharset("utf-8");

  if( replyToList && !headerField( "Mail-Followup-To" ).isEmpty() ) {
    // strip my own address from the list of recipients
    QStringList recipients =
      splitEmailAddrList( headerField( "Mail-Followup-To" ) );
    toStr = stripAddressFromAddressList( msg->from(), recipients ).join(", ");
  }
  else if (replyToList && parent() && parent()->isMailingList())
  {
    // Reply to mailing-list posting address
    toStr = parent()->mailingListPostAddress();
  }
  else if (replyToList
	   && headerField("List-Post").find("mailto:", 0, false) != -1 )
  {
    QString listPost = headerField("List-Post");
    QRegExp rx( "<mailto:([^@>]+)@([^>]+)>", false );
    if ( rx.search( listPost, 0 ) != -1 ) // matched
      toStr = rx.cap(1) + '@' + rx.cap(2);
  }
  else if (replyToAll)
  {
    QStringList recipients;

    // add addresses from the Reply-To header to the list of recipients
    if( !replyToStr.isEmpty() )
      recipients += splitEmailAddrList( replyToStr );

    // add From address to the list of recipients if it's not already there
    if( !from().isEmpty() )
      if( !addressIsInAddressList( from(), recipients ) ) {
        recipients += from();
        kdDebug(5006) << "Added " << from() << " to the list of recipients"
                      << endl;
      }

    // add only new addresses from the To header to the list of recipients
    if( !to().isEmpty() ) {
      QStringList list = splitEmailAddrList( to() );
      for( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if( !addressIsInAddressList( *it, recipients ) ) {
          recipients += *it;
          kdDebug(5006) << "Added " << *it << " to the list of recipients"
                        << endl;
        }
      }
    }

    // strip my own address from the list of recipients
    toStr = stripAddressFromAddressList( msg->from(), recipients ).join(", ");

    // the same for the cc field
    if( !cc().isEmpty() ) {
      QStringList ccRecipients;
      // add only new addresses from the CC header to the list of CC recipients
      QStringList list = splitEmailAddrList( cc() );
      for( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if(    !addressIsInAddressList( *it, recipients )
            && !addressIsInAddressList( *it, ccRecipients ) ) {
          ccRecipients += *it;
          kdDebug(5006) << "Added " << *it << " to the list of CC recipients"
                        << endl;
        }
      }

      // strip my own address from the list of CC recipients
      ccRecipients = stripAddressFromAddressList( msg->from(), ccRecipients );
      msg->setCc( ccRecipients.join(", ") );
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

  if (replyToAll || replyToList || !mailingListStr.isEmpty()
      || (parent() && parent()->isMailingList()))
    replyStr = sReplyAllStr;
  else replyStr = sReplyStr;
  replyStr += "\n";

  if (!noQuote) {
    if( selectionIsBody ){
      QCString cStr = selection.latin1();
      msg->setBody( cStr );
    }else{
      msg->setBody(asQuotedString(replyStr, sIndentPrefixStr, selection,
				  sSmartQuote, allowDecryption));
    }
  }

  msg->setSubject(cleanSubject(sReplySubjPrefixes, sReplaceSubjPrefix, "Re:"));

  // setStatus(KMMsgStatusReplied);
  msg->link(this, KMMsgStatusReplied);

  return msg;
}


//-----------------------------------------------------------------------------
QCString KMMessage::getRefStr() const
{
  QCString firstRef, lastRef, refStr, retRefStr;
  int i, j;

  refStr = headerField("References").stripWhiteSpace().latin1();

  if (refStr.isEmpty())
    return headerField("Message-Id").latin1();

  i = refStr.find('<');
  j = refStr.find('>');
  firstRef = refStr.mid(i, j-i+1);
  if (!firstRef.isEmpty())
    retRefStr = firstRef + ' ';

  i = refStr.findRev('<');
  j = refStr.findRev('>');

  lastRef = refStr.mid(i, j-i+1);
  if (!lastRef.isEmpty() && lastRef != firstRef)
    retRefStr += lastRef + ' ';

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
	     "This will only work if the email address of the sender, "
	     "%1, is valid.\n"
             "The failing address will be reported to be %2.")
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

  if (sHeaderStrategy == HeaderStrategy::all()) {
    s = "\n\n----------  " + sForwardStr + "  ----------\n\n";
    s += headerAsString();
    str = asQuotedString(s, "", QString::null, false, false);
    str += "\n-------------------------------------------------------\n";
  } else {
    s = "\n\n----------  " + sForwardStr + "  ----------\n\n";
    s += "Subject: " + subject() + "\n";
    s += "Date: "
         + KMime::DateFormatter::formatDate( KMime::DateFormatter::Localized,
                                             date(), sReplyLanguage, false )
         + "\n";
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
      QCString mimeType = msgPart.typeStr().lower() + '/'
                        + msgPart.subtypeStr().lower();
      // don't add the detached signature as attachment when forwarding a
      // PGP/MIME signed message inline
      if( mimeType != "application/pgp-signature" ) {
        if (i > 0 || qstricmp(msgPart.typeStr(),"text") != 0)
          msg->addBodyPart(&msgPart);
      }
    }
  }

  msg->setSubject(cleanSubject(sForwardSubjPrefixes, sReplaceForwSubjPrefix, "Fwd:"));

  msg->cleanupHeader();

  // setStatus(KMMsgStatusForwarded);
  msg->link(this, KMMsgStatusForwarded);

  return msg;
}

static const struct {
  const char * dontAskAgainID;
  bool         canDeny;
  const char * text;
} mdnMessageBoxes[] = {
  { "mdnNormalAsk", true,
    I18N_NOOP("This message contains a request to send a disposition "
	      "notification.\n"
	      "You can either ignore the request or let KMail send a "
	      "\"denied\" or normal response.") },
  { "mdnUnknownOption", false,
    I18N_NOOP("This message contains a request to send a disposition "
	      "notification.\n"
	      "It contains a processing instruction that is marked as "
	      "\"required\", but which is unknown to KMail.\n"
	      "You can either ignore the request or let KMail send a "
	      "\"failed\" response.") },
  { "mdnMultipleAddressesInReceiptTo", true,
    I18N_NOOP("This message contains a request to send a disposition "
	      "notification,\n"
	      "but it is requested to send the notification to more "
	      "than one address.\n"
	      "You can either ignore the request or let KMail send a "
	      "\"denied\" or normal response.") },
  { "mdnReturnPathEmpty", true,
    I18N_NOOP("This message contains a request to send a disposition "
	      "notification,\n"
	      "but there is no return-path set.\n"
	      "You can either ignore the request or let KMail send a "
	      "\"denied\" or normal response.") },
  { "mdnReturnPathNotInReceiptTo", true,
    I18N_NOOP("This message contains a request to send a disposition "
	      "notification,\n"
	      "but the return-path address differs from the address "
	      "the notification was requested to be sent to.\n"
	      "You can either ignore the request or let KMail send a "
	      "\"denied\" or normal response.") },
};

static const int numMdnMessageBoxes
      = sizeof mdnMessageBoxes / sizeof *mdnMessageBoxes;


static int requestAdviceOnMDN( const char * what ) {
  for ( int i = 0 ; i < numMdnMessageBoxes ; ++i )
    if ( !qstrcmp( what, mdnMessageBoxes[i].dontAskAgainID ) )
      if ( mdnMessageBoxes[i].canDeny ) {
	int answer = QMessageBox::information( 0,
			 i18n("Message Disposition Notification Request"),
			 i18n( mdnMessageBoxes[i].text ),
			 i18n("&Ignore"), i18n("Send \"&denied\""), i18n("&Send") );
	return answer ? answer + 1 : 0 ; // map to "mode" in createMDN
      } else {
	int answer = QMessageBox::information( 0,
			 i18n("Message Disposition Notification Request"),
			 i18n( mdnMessageBoxes[i].text ),
			 i18n("&Ignore"), i18n("&Send") );
	return answer ? answer + 2 : 0 ; // map to "mode" in createMDN
      }
  kdWarning(5006) << "didn't find data for message box \""
		  << what << "\"" << endl;
  return 0;
}

KMMessage* KMMessage::createMDN( MDN::ActionMode a,
				 MDN::DispositionType d,
				 bool allowGUI,
				 QValueList<MDN::DispositionModifier> m )
{
  // RFC 2298: At most one MDN may be issued on behalf of each
  // particular recipient by their user agent.  That is, once an MDN
  // has been issued on behalf of a recipient, no further MDNs may be
  // issued on behalf of that recipient, even if another disposition
  // is performed on the message.
//#define MDN_DEBUG 1
#ifndef MDN_DEBUG
  if ( mdnSentState() != KMMsgMDNStateUnknown &&
       mdnSentState() != KMMsgMDNNone )
    return 0;
#else
  char st[2]; st[0] = (char)mdnSentState(); st[1] = 0;
  kdDebug(5006) << "mdnSentState() == '" << st << "'" << endl;
#endif

  // RFC 2298: An MDN MUST NOT be generated in response to an MDN.
  if ( findDwBodyPart( DwMime::kTypeMessage,
		       DwMime::kSubtypeDispositionNotification ) ) {
    setMDNSentState( KMMsgMDNIgnore );
    return 0;
  }

  // extract where to send to:
  QString receiptTo = headerField("Disposition-Notification-To");
  if ( receiptTo.stripWhiteSpace().isEmpty() ) return 0;
  receiptTo.remove( '\n' );


  MDN::SendingMode s = MDN::SentAutomatically; // set to manual if asked user
  QString special; // fill in case of error, warning or failure
  KConfigGroup mdnConfig( KGlobal::config(), "MDN" );

  // default:
  int mode = mdnConfig.readNumEntry( "default-policy", 0 );
  if ( !mode || mode < 0 || mode > 3 ) {
    // early out for ignore:
    setMDNSentState( KMMsgMDNIgnore );
    return 0;
  }

  // RFC 2298: An importance of "required" indicates that
  // interpretation of the parameter is necessary for proper
  // generation of an MDN in response to this request.  If a UA does
  // not understand the meaning of the parameter, it MUST NOT generate
  // an MDN with any disposition type other than "failed" in response
  // to the request.
  QString notificationOptions = headerField("Disposition-Notification-Options");
  if ( notificationOptions.contains( "required", false ) ) {
    // ### hacky; should parse...
    // There is a required option that we don't understand. We need to
    // ask the user what we should do:
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnUnknownOption" );
    s = MDN::SentManually;

    special = i18n("Header \"Disposition-Notification-Options\" contained "
		   "required, but unknown parameter");
    d = MDN::Failed;
    m.clear(); // clear modifiers
  }

  // RFC 2298: [ Confirmation from the user SHOULD be obtained (or no
  // MDN sent) ] if there is more than one distinct address in the
  // Disposition-Notification-To header.
  kdDebug(5006) << "splitEmailAddrList(receiptTo): "
	    << splitEmailAddrList(receiptTo).join("\n") << endl;
  if ( splitEmailAddrList(receiptTo).count() > 1 ) {
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnMultipleAddressesInReceiptTo" );
    s = MDN::SentManually;
  }

  // RFC 2298: MDNs SHOULD NOT be sent automatically if the address in
  // the Disposition-Notification-To header differs from the address
  // in the Return-Path header. [...] Confirmation from the user
  // SHOULD be obtained (or no MDN sent) if there is no Return-Path
  // header in the message [...]
  AddrSpecList returnPathList = extractAddrSpecs("Return-Path");
  QString returnPath = returnPathList.isEmpty() ? QString::null
    : returnPathList.front().localPart + '@' + returnPathList.front().domain ;
  kdDebug(5006) << "clean return path: " << returnPath << endl;
  if ( returnPath.isEmpty() || !receiptTo.contains( returnPath, false ) ) {
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( returnPath.isEmpty() ?
			       "mdnReturnPathEmpty" :
			       "mdnReturnPathNotInReceiptTo" );
    s = MDN::SentManually;
  }

  if ( mode == 1 ) { // ask
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnNormalAsk" );
    s = MDN::SentManually; // asked user
  }

  switch ( mode ) {
  case 0: // ignore:
    setMDNSentState( KMMsgMDNIgnore );
    return 0;
  default:
  case 1:
    kdFatal(5006) << "KMMessage::createMDN(): The \"ask\" mode should "
		  << "never appear here!" << endl;
    break;
  case 2: // deny
    d = MDN::Denied;
    m.clear();
    break;
  case 3:
    break;
  }


  // extract where to send from:
  QString finalRecipient = kernel->identityManager()
    ->identityForUoidOrDefault( identityUoid() ).fullEmailAddr();

  //
  // Generate message:
  //

  KMMessage * receipt = new KMMessage();
  receipt->initFromMessage( this );
  receipt->removeHeaderField("Content-Type");
  receipt->removeHeaderField("Content-Transfer-Encoding");
  // Modify the ContentType directly (replaces setAutomaticFields(true))
  DwHeaders & header = receipt->mMsg->Headers();
  header.MimeVersion().FromString("1.0");
  DwMediaType & contentType = receipt->dwContentType();
  contentType.SetType( DwMime::kTypeMultipart );
  contentType.SetSubtype( DwMime::kSubtypeReport );
  contentType.CreateBoundary(0);
  receipt->mNeedsAssembly = true;
  receipt->setContentTypeParam( "report-type", "disposition-notification" );

  QString description = replaceHeadersInString( MDN::descriptionFor( d, m ) );

  // text/plain part:
  KMMessagePart firstMsgPart;
  firstMsgPart.setTypeStr( "text" );
  firstMsgPart.setSubtypeStr( "plain" );
  firstMsgPart.setBodyFromUnicode( description );
  receipt->addBodyPart( &firstMsgPart );

  // message/disposition-notification part:
  KMMessagePart secondMsgPart;
  secondMsgPart.setType( DwMime::kTypeMessage );
  secondMsgPart.setSubtype( DwMime::kSubtypeDispositionNotification );
  //secondMsgPart.setCharset( "us-ascii" );
  //secondMsgPart.setCteStr( "7bit" );
  secondMsgPart.setBodyEncoded( MDN::dispositionNotificationBodyContent(
	                    finalRecipient,
			    rawHeaderField("Original-Recipient"),
			    id(), /* Message-ID */
			    d, a, s, m, special ) );
  receipt->addBodyPart( &secondMsgPart );

  // message/rfc822 or text/rfc822-headers body part:
  int num = mdnConfig.readNumEntry( "quote-message", 0 );
  if ( num < 0 || num > 2 ) num = 0;
  MDN::ReturnContent returnContent = static_cast<MDN::ReturnContent>( num );

  KMMessagePart thirdMsgPart;
  switch ( returnContent ) {
  case MDN::All:
    thirdMsgPart.setTypeStr( "message" );
    thirdMsgPart.setSubtypeStr( "rfc822" );
    thirdMsgPart.setBody( asSendableString() );
    receipt->addBodyPart( &thirdMsgPart );
    break;
  case MDN::HeadersOnly:
    thirdMsgPart.setTypeStr( "text" );
    thirdMsgPart.setSubtypeStr( "rfc822-headers" );
    thirdMsgPart.setBody( headerAsSendableString() );
    receipt->addBodyPart( &thirdMsgPart );
    break;
  case MDN::Nothing:
  default:
    break;
  };

  receipt->setTo( receiptTo );
  receipt->setSubject( "Message Disposition Notification" );
  receipt->setReplyToId( msgId() );
  receipt->setReferences( getRefStr() );

  receipt->cleanupHeader();

  kdDebug(5006) << "final message:\n" + receipt->asString() << endl;

  //
  // Set "MDN sent" status:
  //
  KMMsgMDNSentState state = KMMsgMDNStateUnknown;
  switch ( d ) {
  case MDN::Displayed:   state = KMMsgMDNDisplayed;  break;
  case MDN::Deleted:     state = KMMsgMDNDeleted;    break;
  case MDN::Dispatched:  state = KMMsgMDNDispatched; break;
  case MDN::Processed:   state = KMMsgMDNProcessed;  break;
  case MDN::Denied:      state = KMMsgMDNDenied;     break;
  case MDN::Failed:      state = KMMsgMDNFailed;     break;
  };
  setMDNSentState( state );

  return receipt;
}

QString KMMessage::replaceHeadersInString( const QString & s ) const {
  QString result = s;
  QRegExp rx( "\\$\\{([a-z0-9-]+)\\}", false );
  Q_ASSERT( rx.isValid() );
  int idx = 0;
  while ( ( idx = rx.search( result, idx ) ) != -1 ) {
    QString replacement = headerField( rx.cap(1).latin1() );
    result.replace( idx, rx.matchedLength(), replacement );
    idx += replacement.length();
  }
  return result;
}

QString KMMessage::forwardSubject() const {
  return cleanSubject( sForwardSubjPrefixes, sReplaceForwSubjPrefix, "Fwd:" );
}

QString KMMessage::replySubject() const {
  return cleanSubject( sReplySubjPrefixes, sReplaceSubjPrefix, "Re:" );
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
void KMMessage::initHeader( uint id )
{
  const KMIdentity & ident =
    kernel->identityManager()->identityForUoidOrDefault( id );

  if(ident.fullEmailAddr().isEmpty())
    setFrom("");
  else
    setFrom(ident.fullEmailAddr());

  if(ident.replyToAddr().isEmpty())
    setReplyTo("");
  else
    setReplyTo(ident.replyToAddr());

  if(ident.bcc().isEmpty())
    setBcc("");
  else
    setBcc(ident.bcc());

  if (ident.organization().isEmpty())
    removeHeaderField("Organization");
  else
    setHeaderField("Organization", ident.organization());

  if (ident.isDefault())
    removeHeaderField("X-KMail-Identity");
  else
    setHeaderField("X-KMail-Identity", QString().setNum( ident.uoid() ));

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

  setTo("");
  setSubject("");
  setDateToday();

  setHeaderField("User-Agent", "KMail/" KMAIL_VERSION );
  // This will allow to change Content-Type:
  setHeaderField("Content-Type","text/plain");
}

uint KMMessage::identityUoid() const {
  QString idString = headerField("X-KMail-Identity").stripWhiteSpace();
  bool ok = false;
  int id = idString.toUInt( &ok );

  if ( !ok || id == 0 )
    id = kernel->identityManager()->identityForAddress( to() + cc() ).uoid();
  if ( id == 0 && parent() )
    id = parent()->identity();

  return id;
}


//-----------------------------------------------------------------------------
void KMMessage::initFromMessage(const KMMessage *msg, bool idHeaders)
{
  uint id = msg->identityUoid();

  if ( idHeaders ) initHeader(id);
  else setHeaderField("X-KMail-Identity", QString().setNum(id));
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
    DwMediaType& contentType = dwContentType();
    contentType.SetType(   DwMime::kTypeMultipart);
    contentType.SetSubtype(DwMime::kSubtypeMixed );

    // Create a random printable string and set it as the boundary parameter
    contentType.CreateBoundary(0);
  }
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
QString KMMessage::dateStr(void) const
{
  KConfigGroup general( KMKernel::config(), "General" );
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();

  //kdDebug(5006)<<"####  Date = "<<header.Date().AsString().c_str()<<endl;

  return KMime::DateFormatter::formatDate(
      static_cast<KMime::DateFormatter::FormatType>(general.readNumEntry( "dateFormat", KMime::DateFormatter::Fancy )),
      unixTime, general.readEntry( "customDateFormat" ));
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
  gettimeofday(&tval, 0);
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
QString KMMessage::ccStrip(void) const
{
  return stripEmailAddr(headerField("Cc"));
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
  return headerField( "X-KMail-Fcc" );
}


//-----------------------------------------------------------------------------
void KMMessage::setFcc(const QString& aStr)
{
  setHeaderField( "X-KMail-Fcc", aStr );
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
  if (mParent)
    return headerField(mParent->whoField().utf8());
  return headerField("From");
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
  // We ignore mangled In-Reply-To headers which are created by a
  // misconfigured Mutt. They look like this <"from foo"@bar.baz>, i.e.
  // they contain double quotes and spaces. We only check for '"'.
  if (!replyTo.isEmpty() && (replyTo[0] == '<') &&
      ( -1 == replyTo.find( '"' ) ) )
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
QString KMMessage::references() const
{
  int leftAngle, rightAngle;
  QString references = headerField( "References" );

  // keep the last two entries for threading
  leftAngle = references.findRev( '<' );
  leftAngle = references.findRev( '<', leftAngle - 1 );
  if( leftAngle != -1 )
    references = references.mid( leftAngle );
  rightAngle = references.findRev( '>' );
  if( rightAngle != -1 )
    references.truncate( rightAngle + 1 );

  if( !references.isEmpty() && references[0] == '<' )
    return references;
  else
    return QString::null;
}

//-----------------------------------------------------------------------------
QString KMMessage::replyToAuxIdMD5() const
{
  int rightAngle;
  QString result = references();
  // references contains two items, use the first one
  // (the second to last reference)
  rightAngle = result.find( '>' );
  if( rightAngle != -1 )
    result.truncate( rightAngle + 1 );

  return KMMessagePart::encodeBase64( result );
}

//-----------------------------------------------------------------------------
QString KMMessage::strippedSubjectMD5() const
{
  QString result = stripOffPrefixes( subject() );
  return KMMessagePart::encodeBase64( result );
}

//-----------------------------------------------------------------------------
bool KMMessage::subjectIsPrefixed() const
{
  return strippedSubjectMD5() != KMMessagePart::encodeBase64( subject() );
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
AddressList KMMessage::headerAddrField( const QCString & aName ) const {
  const QCString header = rawHeaderField( aName );
  AddressList result;
  const char * scursor = header.begin();
  if ( !scursor )
    return AddressList();
  const char * const send = header.end();
  if ( !parseAddressList( scursor, send, result ) )
    kdDebug(5006) << "Error in address splitting: parseAddressList returned false!"
                  << endl;
  return result;
}

AddrSpecList KMMessage::extractAddrSpecs( const QCString & header ) const {
  AddressList al = headerAddrField( header );
  AddrSpecList result;
  for ( AddressList::const_iterator ait = al.begin() ; ait != al.end() ; ++ait )
    for ( MailboxList::const_iterator mit = (*ait).mailboxList.begin() ; mit != (*ait).mailboxList.end() ; ++mit )
      result.push_back( (*mit).addrSpec );
  return result;
}

QCString KMMessage::rawHeaderField( const QCString & name ) const {
  if ( name.isEmpty() ) return QCString();

  DwHeaders & header = mMsg->Headers();
  DwField * field = header.FindField( name );

  if ( !field ) return QCString();

  return header.FieldBody( name.data() ).AsString().c_str();
}

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
  if (aName.isEmpty()) return;

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
  str = aName;
  if (str[str.length()-1] != ':') str += ": ";
  else str += ' ';
  str += aValue;
  if (str[str.length()-1] != '\n') str += '\n';

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
  dwContentType().SetTypeStr(DwString(aStr));
  dwContentType().Parse();
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setType(int aType)
{
  dwContentType().SetType(aType);
  dwContentType().Assemble();
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
  dwContentType().SetSubtypeStr(DwString(aStr));
  dwContentType().Parse();
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtype(int aSubtype)
{
  dwContentType().SetSubtype(aSubtype);
  dwContentType().Assemble();
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::setDwMediaTypeParam( DwMediaType &mType,
                                     const QCString& attr,
                                     const QCString& val )
{
  mType.Parse();
  DwParameter *param = mType.FirstParameter();
  while(param) {
    if (!qstricmp(param->Attribute().c_str(), attr))
      break;
    else
      param = param->Next();
  }
  if (!param){
    param = new DwParameter;
    param->SetAttribute(DwString( attr ));
    mType.AddParameter( param );
  }
  else
    mType.SetModified();
  param->SetValue(DwString( val ));
  mType.Assemble();
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTypeParam(const QCString& attr, const QCString& val)
{
  if (mNeedsAssembly) mMsg->Assemble();
  mNeedsAssembly = FALSE;
  setDwMediaTypeParam( dwContentType(), attr, val );
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
DwHeaders& KMMessage::headers() const
{
  return mMsg->Headers();
}


//-----------------------------------------------------------------------------
void KMMessage::setNeedsAssembly(void)
{
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
QCString KMMessage::body(void) const
{
  DwString body = mMsg->Body().AsString();
  QCString str = body.c_str();
  kdWarning( str.length() != body.length(), 5006 )
    << "KMMessage::body(): body is binary but used as text!" << endl;
  return str;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::bodyDecodedBinary(void) const
{
  DwString dwstr;
  DwString dwsrc = mMsg->Body().AsString();

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

  int len = dwstr.size();
  QByteArray ba(len);
  memcpy(ba.data(),dwstr.data(),len);
  return ba;
}


//-----------------------------------------------------------------------------
QCString KMMessage::bodyDecoded(void) const
{
  DwString dwstr;
  DwString dwsrc = mMsg->Body().AsString();

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

  unsigned int len = dwstr.size();
  QCString result(len+1);
  memcpy(result.data(),dwstr.data(),len);
  result[len] = 0;
  kdWarning(result.length() != len, 5006)
    << "KMMessage::bodyDecoded(): body is binary but used as text!" << endl;
  return result;
}


//-----------------------------------------------------------------------------
QValueList<int> KMMessage::determineAllowedCtes( const CharFreq& cf,
                                                 bool allow8Bit,
                                                 bool willBeSigned )
{
  QValueList<int> allowedCtes;

  switch ( cf.type() ) {
  case CharFreq::SevenBitText:
    allowedCtes << DwMime::kCte7bit;
  case CharFreq::EightBitText:
    if ( allow8Bit )
      allowedCtes << DwMime::kCte8bit;
  case CharFreq::SevenBitData:
    if ( cf.printableRatio() > 5.0/6.0 ) {
      // let n the length of data and p the number of printable chars.
      // Then base64 \approx 4n/3; qp \approx p + 3(n-p)
      // => qp < base64 iff p > 5n/6.
      allowedCtes << DwMime::kCteQp;
      allowedCtes << DwMime::kCteBase64;
    } else {
      allowedCtes << DwMime::kCteBase64;
      allowedCtes << DwMime::kCteQp;
    }
    break;
  case CharFreq::EightBitData:
    allowedCtes << DwMime::kCteBase64;
    break;
  case CharFreq::None:
  default:
    // just nothing (avoid compiler warning)
    ;
  }

  // In the following cases only QP and Base64 are allowed:
  // - the buffer will be OpenPGP/MIME signed and it contains trailing
  //   whitespace (cf. RFC 3156)
  // - a line starts with "From "
  if ( ( willBeSigned && cf.hasTrailingWhitespace() ) ||
       cf.hasLeadingFrom() ) {
    allowedCtes.remove( DwMime::kCte8bit );
    allowedCtes.remove( DwMime::kCte7bit );
  }

  return allowedCtes;
}


//-----------------------------------------------------------------------------
void KMMessage::setBodyAndGuessCte( const QByteArray& aBuf,
                                    QValueList<int> & allowedCte,
                                    bool allow8Bit,
                                    bool willBeSigned )
{
  CharFreq cf( aBuf ); // it's safe to pass null arrays

  allowedCte = determineAllowedCtes( cf, allow8Bit, willBeSigned );

#ifndef NDEBUG
  DwString dwCte;
  DwCteEnumToStr(allowedCte[0], dwCte);
  kdDebug(5006) << "CharFreq returned " << cf.type() << "/"
                << cf.printableRatio() << " and I chose "
                << dwCte.c_str() << endl;
#endif

  setCte( allowedCte[0] ); // choose best fitting
  setBodyEncodedBinary( aBuf );
}


//-----------------------------------------------------------------------------
void KMMessage::setBodyAndGuessCte( const QCString& aBuf,
                                    QValueList<int> & allowedCte,
                                    bool allow8Bit,
                                    bool willBeSigned )
{
  CharFreq cf( aBuf.data(), aBuf.length() ); // it's safe to pass null strings

  allowedCte = determineAllowedCtes( cf, allow8Bit, willBeSigned );

#ifndef NDEBUG
  DwString dwCte;
  DwCteEnumToStr(allowedCte[0], dwCte);
  kdDebug(5006) << "CharFreq returned " << cf.type() << "/"
                << cf.printableRatio() << " and I chose "
                << dwCte.c_str() << endl;
#endif

  setCte( allowedCte[0] ); // choose best fitting
  setBodyEncoded( aBuf );
}


//-----------------------------------------------------------------------------
void KMMessage::setBodyEncoded(const QCString& aStr)
{
  DwString dwSrc(aStr.data(), aStr.size()-1 /* not the trailing NUL */);
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

void KMMessage::setMultiPartBody( const QCString & aStr ) {
  setBody( aStr );
  mMsg->Body().Parse();
  mNeedsAssembly = true;
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
  DwBodyPart* part = getFirstDwBodyPart();
  QPtrList< DwBodyPart > parts;
  QString mp = "multipart";

  while (part)
  {
    //dive into multipart messages
    while (    part
            && part->hasHeaders()
            && part->Headers().HasContentType()
            && (mp == part->Headers().ContentType().TypeStr().c_str()) )
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
    };

    if (part)
      part = part->Next();
  }

  return count;
}


//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::getFirstDwBodyPart() const
{
  return mMsg->Body().FirstBodyPart();
}


//-----------------------------------------------------------------------------
int KMMessage::partNumber( DwBodyPart * aDwBodyPart ) const
{
  DwBodyPart *curpart;
  QPtrList< DwBodyPart > parts;
  int curIdx = 0;
  int idx = 0;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();

  while (curpart && !idx) {
    //dive into multipart messages
    while(    curpart
           && curpart->hasHeaders()
           && curpart->Headers().HasContentType()
           && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) )
    {
      parts.append( curpart );
      curpart = curpart->Body().FirstBodyPart();
    }
    // this is where currPart->msgPart contains a leaf message part
    if (curpart == aDwBodyPart)
      idx = curIdx;
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
  return idx;
}


//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::dwBodyPart( int aIdx ) const
{
  DwBodyPart *part, *curpart;
  QPtrList< DwBodyPart > parts;
  int curIdx = 0;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();
  part = 0;

  while (curpart && !part) {
    //dive into multipart messages
    while(    curpart
           && curpart->hasHeaders()
           && curpart->Headers().HasContentType()
           && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) )
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
  return part;
}


//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::findDwBodyPart( int type, int subtype ) const
{
  DwBodyPart *part, *curpart;
  QPtrList< DwBodyPart > parts;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();
  part = 0;

  while (curpart && !part) {
    //dive into multipart messages
    while(curpart
	  && curpart->hasHeaders()
	  && curpart->Headers().HasContentType()
	  && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) ) {
	parts.append( curpart );
	curpart = curpart->Body().FirstBodyPart();
    }
    // this is where curPart->msgPart contains a leaf message part

    // pending(khz): Find out WHY this look does not travel down *into* an
    //               embedded "Message/RfF822" message containing a "Multipart/Mixed"
    if (curpart && curpart->hasHeaders() ) {
      kdDebug(5006) << curpart->Headers().ContentType().TypeStr().c_str()
		<< "  " << curpart->Headers().ContentType().SubtypeStr().c_str() << endl;
    }

    if (curpart &&
	curpart->hasHeaders() &&
	curpart->Headers().ContentType().Type() == type &&
	curpart->Headers().ContentType().Subtype() == subtype) {
	part = curpart;
    } else {
      // go up in the tree until reaching a node with next
      // (or the last top-level node)
      while (curpart && !(curpart->Next()) && !(parts.isEmpty())) {
	curpart = parts.getLast();
	parts.removeLast();
      } ;
      if (curpart)
	curpart = curpart->Next();
    }
  }
  return part;
}


//-----------------------------------------------------------------------------
void KMMessage::bodyPart(DwBodyPart* aDwBodyPart, KMMessagePart* aPart,
			 bool withBody)
{
  if( aPart ) {
    if( aDwBodyPart && aDwBodyPart->hasHeaders()  ) {
      // This must not be an empty string, because we'll get a
      // spurious empty Subject: line in some of the parts.
      aPart->setName(" ");
      DwHeaders& headers = aDwBodyPart->Headers();
      // Content-type
      QCString additionalCTypeParams;
      if (headers.HasContentType())
      {
        DwMediaType& ct = headers.ContentType();
        aPart->setOriginalContentTypeStr( ct.AsString().c_str() );
        aPart->setTypeStr(ct.TypeStr().c_str());
        aPart->setSubtypeStr(ct.SubtypeStr().c_str());
        DwParameter *param = ct.FirstParameter();
        while(param)
        {
          if (!qstricmp(param->Attribute().c_str(), "charset"))
            aPart->setCharset(QCString(param->Value().c_str()).lower());
          else if (param->Attribute().c_str()=="name*")
            aPart->setName(KMMsgBase::decodeRFC2231String(
              param->Value().c_str()));
          else {
            additionalCTypeParams += ';';
            additionalCTypeParams += param->AsString().c_str();
          }
          param=param->Next();
        }
      }
      else
      {
        aPart->setTypeStr("text");      // Set to defaults
        aPart->setSubtypeStr("plain");
      }
      aPart->setAdditionalCTypeParamStr( additionalCTypeParams );
      // Modification by Markus
      if (aPart->name().isEmpty() || aPart->name() == " ")
      {
	if (!headers.ContentType().Name().empty()) {
	  aPart->setName(KMMsgBase::decodeRFC2047String(headers.
							ContentType().Name().c_str()) );
	} else if (!headers.Subject().AsString().empty()) {
	  aPart->setName( KMMsgBase::decodeRFC2047String(headers.
							 Subject().AsString().c_str()) );
	}
      }

      // Content-transfer-encoding
      if (headers.HasContentTransferEncoding())
        aPart->setCteStr(headers.ContentTransferEncoding().AsString().c_str());
      else
        aPart->setCteStr("7bit");

      // Content-description
      if (headers.HasContentDescription())
        aPart->setContentDescription(headers.ContentDescription().AsString().c_str());
      else
        aPart->setContentDescription("");

      // Content-disposition
      if (headers.HasContentDisposition())
        aPart->setContentDisposition(headers.ContentDisposition().AsString().c_str());
      else
        aPart->setContentDisposition("");

      // Body
      if (withBody)
	  aPart->setBody( aDwBodyPart->Body().AsString().c_str() );
      else
	  aPart->setBody( "" );
    }
    // If no valid body part was not given,
    // set all MultipartBodyPart attributes to empty values.
    else
    {
      aPart->setTypeStr("");
      aPart->setSubtypeStr("");
      aPart->setCteStr("");
      // This must not be an empty string, because we'll get a
      // spurious empty Subject: line in some of the parts.
      aPart->setName(" ");
      aPart->setContentDescription("");
      aPart->setContentDisposition("");
      aPart->setBody("");
    }
  }
}


//-----------------------------------------------------------------------------
void KMMessage::bodyPart(int aIdx, KMMessagePart* aPart) const
{
  if( aPart ) {
    // If the DwBodyPart was found get the header fields and body
    DwBodyPart *part = dwBodyPart( aIdx );
    if( part )

    {
      KMMessage::bodyPart(part, aPart);
      if( aPart->name().isEmpty() )
        aPart->setName( i18n("Attachment: ") + QString( "%1" ).arg( aIdx ) );
    }
  }
}


//-----------------------------------------------------------------------------
void KMMessage::deleteBodyParts(void)
{
  mMsg->Body().DeleteBodyParts();
}


//-----------------------------------------------------------------------------
DwBodyPart* KMMessage::createDWBodyPart(const KMMessagePart* aPart)
{
  DwBodyPart* part = DwBodyPart::NewBodyPart(emptyString, 0);

  if( aPart ) {
    QCString charset  = aPart->charset();
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

    DwMediaType& ct = headers.ContentType();
    if (!type.isEmpty() && !subtype.isEmpty())
    {
      ct.SetTypeStr(type.data());
      ct.SetSubtypeStr(subtype.data());
      if (!charset.isEmpty()){
	DwParameter *param;
	param=new DwParameter;
	param->SetAttribute("charset");
	param->SetValue(charset.data());
	ct.AddParameter(param);
      }
    }

    QCString additionalParam = aPart->additionalCTypeParamStr();
    if( !additionalParam.isEmpty() )
    {
      QCString parAV;
      DwString parA, parV;
      int iL, i1, i2, iM;
      iL = additionalParam.length();
      i1 = 0;
      i2 = additionalParam.find(';', i1, false);
      while ( i1 < iL )
      {
        if( -1 == i2 )
          i2 = iL;
        if( i1+1 < i2 ) {
          parAV = additionalParam.mid( i1, (i2-i1) );
          iM = parAV.find('=');
          if( -1 < iM )
          {
            parA = parAV.left( iM );
            parV = parAV.right( parAV.length() - iM - 1 );
            if( ('"' == parV.at(0)) && ('"' == parV.at(parV.length()-1)) )
            {
              parV.erase( 0,  1);
              parV.erase( parV.length()-1 );
            }
          }
          else
          {
            parA = parAV;
            parV = "";
          }
          DwParameter *param;
          param = new DwParameter;
          param->SetAttribute( parA );
          param->SetValue(     parV );
          ct.AddParameter( param );
        }
        i1 = i2+1;
        i2 = additionalParam.find(';', i1, false);
      }
    }

    if (RFC2231encoded)
    {
      DwParameter *nameParam;
      nameParam = new DwParameter;
      nameParam->SetAttribute("name*");
      nameParam->SetValue(name.data(),true);
      ct.AddParameter(nameParam);
    } else {
      if(!name.isEmpty())
        ct.SetName(name.data());
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
	param->SetValue(paramValue.data(),true);
      } else {
        param->SetAttribute(paramAttr.data());
	param->SetValue(paramValue.data());
      }
      ct.AddParameter(param);
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
  }
  return part;
}


//-----------------------------------------------------------------------------
void KMMessage::addDwBodyPart(DwBodyPart * aDwPart)
{
  mMsg->Body().AddBodyPart( aDwPart );
  mNeedsAssembly = TRUE;
}


//-----------------------------------------------------------------------------
void KMMessage::addBodyPart(const KMMessagePart* aPart)
{
  DwBodyPart* part = createDWBodyPart( aPart );
  addDwBodyPart( part );
}


//-----------------------------------------------------------------------------
QString KMMessage::generateMessageId( const QString& addr )
{
  QDateTime datetime = QDateTime::currentDateTime();
  QString msgIdStr;

  msgIdStr = '<' + datetime.toString( "yyyyMMddhhmm.sszzz" );

  QString msgIdSuffix;
  KConfigGroup general( KMKernel::config(), "General" );

  if( general.readBoolEntry( "useCustomMessageIdSuffix", false ) )
    msgIdSuffix = general.readEntry( "myMessageIdSuffix" );

  if( !msgIdSuffix.isEmpty() )
    msgIdStr += '@' + msgIdSuffix;
  else
    msgIdStr += '.' + addr;

  msgIdStr += '>';

  return msgIdStr;
}


//-----------------------------------------------------------------------------
QCString KMMessage::html2source( const QCString & src )
{
  QCString result( 1 + 6*src.length() );  // maximal possible length

  QCString::ConstIterator s = src.begin();
  QCString::Iterator d = result.begin();
  while ( *s ) {
    switch ( *s ) {
    case '<': {
        *d++ = '&';
        *d++ = 'l';
        *d++ = 't';
        *d++ = ';';
        ++s;
      }
      break;
    case '\r': {
        ++s;
      }
      break;
    case '\n': {
        *d++ = '<';
        *d++ = 'b';
        *d++ = 'r';
        *d++ = '>';
        ++s;
      }
      break;
    case '>': {
        *d++ = '&';
        *d++ = 'g';
        *d++ = 't';
        *d++ = ';';
        ++s;
      }
      break;
    case '&': {
        *d++ = '&';
        *d++ = 'a';
        *d++ = 'm';
        *d++ = 'p';
        *d++ = ';';
        ++s;
      }
      break;
    case '"': {
        *d++ = '&';
        *d++ = 'q';
        *d++ = 'u';
        *d++ = 'o';
        *d++ = 't';
        *d++ = ';';
        ++s;
      }
      break;
    case '\'': {
        *d++ = '&';
	*d++ = 'a';
	*d++ = 'p';
	*d++ = 's';
	*d++ = ';';
	++s;
      }
      break;
    default:
        *d++ = *s++;
    }
  }
  result.truncate( d - result.begin() ); // adds trailing NUL
  return result;
}


//-----------------------------------------------------------------------------
QCString KMMessage::lf2crlf( const QCString & src )
{
  QCString result( 1 + 2*src.length() );  // maximal possible length

  QCString::ConstIterator s = src.begin();
  QCString::Iterator d = result.begin();
  // we use cPrev to make sure we insert '\r' only there where it is missing
  char cPrev = '?';
  while ( *s ) {
    if ( ('\n' == *s) && ('\r' != cPrev) )
      *d++ = '\r';
    cPrev = *s;
    *d++ = *s++;
  }
  result.truncate( d - result.begin() ); // adds trailing NUL
  return result;
}


//-----------------------------------------------------------------------------
QString KMMessage::encodeMailtoUrl( const QString& str )
{
  QString result;
  result = QString::fromLatin1( KMMsgBase::encodeRFC2047String( str,
                                                                "utf-8" ) );
  result = KURL::encode_string( result );
  return result;
}


//-----------------------------------------------------------------------------
QString KMMessage::decodeMailtoUrl( const QString& url )
{
  QString result;
  result = KURL::decode_string( url );
  result = KMMsgBase::decodeRFC2047String( result.latin1() );
  return result;
}


//-----------------------------------------------------------------------------
QString KMMessage::stripEmailAddr(const QString& aStr)
{
  QStringList list = splitEmailAddrList(aStr);
  QString result, totalResult, partA, partB;
  int i, j, len;
  for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
  {
    char endCh = '>';
    i = -1;

    //if format is something like "--<King>-- John King" <john@someemail.com>
    if ( (*it)[0] == '"' )
    {
      i = 0;
      endCh = '"';
    }

    if (i<0)
    {
      i = (*it).find('<');
      endCh = '>';
    }

    if (i<0)
    {
      i = (*it).find('(');
      endCh = ')';
    }
    if (i<0) result = *it;
    else {
      partA = (*it).left(i).stripWhiteSpace();
      j = (*it).find(endCh,i+1);
      if (j<0) result = *it;
      else {
        partB = (*it).mid(i+1, j-i-1).stripWhiteSpace();

        if (partA.find('@') >= 0 && !partB.isEmpty()) result = partB;
        else if (!partA.isEmpty()) result = partA;
        else if (endCh == '"') result = partB;
        else result = (*it);

        len = result.length();
        if (result[0]=='"' && result[len-1]=='"')
          result = result.mid(1, result.length()-2);
        else if (result[0]=='<' && result[len-1]=='>')
          result = result.mid(1, result.length()-2);
        else if (result[0]=='(' && result[len-1]==')')
          result = result.mid(1, result.length()-2);
      }
    }
    if (!totalResult.isEmpty()) totalResult += ", ";
    totalResult += result;
  }
  return totalResult;
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
QString KMMessage::quoteHtmlChars( const QString& str, bool removeLineBreaks )
{
  QString result;
  int resultLength = 0;
  result.setLength( 6*str.length() ); // maximal possible length

  QChar ch;

  for( unsigned int i = 0; i < str.length(); ++i ) {
    ch = str[i];
    if( '<' == ch ) {
      result[resultLength++] = '&';
      result[resultLength++] = 'l';
      result[resultLength++] = 't';
      result[resultLength++] = ';';
    }
    else if ( '>' == ch ) {
      result[resultLength++] = '&';
      result[resultLength++] = 'g';
      result[resultLength++] = 't';
      result[resultLength++] = ';';
    }
    else if( '&' == ch ) {
      result[resultLength++] = '&';
      result[resultLength++] = 'a';
      result[resultLength++] = 'm';
      result[resultLength++] = 'p';
      result[resultLength++] = ';';
    }
    else if( '"' == ch ) {
      result[resultLength++] = '&';
      result[resultLength++] = 'q';
      result[resultLength++] = 'u';
      result[resultLength++] = 'o';
      result[resultLength++] = 't';
      result[resultLength++] = ';';
    }
    else if( '\n' == ch ) {
      if( !removeLineBreaks ) {
        result[resultLength++] = '<';
        result[resultLength++] = 'b';
        result[resultLength++] = 'r';
        result[resultLength++] = ' ';
        result[resultLength++] = '/';
        result[resultLength++] = '>';
      }
    }
    else if( '\r' == ch ) {
      // ignore CR
    }
    else {
      result[resultLength++] = ch;
    }
  }
  result.truncate( resultLength ); // get rid of the undefined junk
  return result;
}

//-----------------------------------------------------------------------------
QString KMMessage::emailAddrAsAnchor(const QString& aEmail, bool stripped)
{
  if( aEmail.isEmpty() )
    return aEmail;

  QStringList addressList = KMMessage::splitEmailAddrList( aEmail );

  QString result;

  for( QStringList::ConstIterator it = addressList.begin();
       ( it != addressList.end() );
       ++it ) {
    if( !(*it).isEmpty() ) {
      QString address = *it;
      result += "<a href=\"mailto:"
              + KMMessage::encodeMailtoUrl( address )
              + "\">";
      if( stripped )
        address = KMMessage::stripEmailAddr( address );
      result += KMMessage::quoteHtmlChars( address, true );
      result += "</a>, ";
    }
  }
  // cut of the trailing ", "
  result.truncate( result.length() - 2 );

  kdDebug(5006) << "KMMessage::emailAddrAsAnchor('" << aEmail
                << "') returns:\n-->" << result << "<--" << endl;
  return result;
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
//static
QStringList KMMessage::stripAddressFromAddressList( const QString& address,
                                                    const QStringList& list )
{
  QStringList addresses = list;
  QCString addrSpec = getEmailAddr( address ).lower();
  for( QStringList::Iterator it = addresses.begin();
       it != addresses.end(); ) {
    if( addrSpec == getEmailAddr( *it ).lower() ) {
      kdDebug(5006) << "Removing " << *it << " from the address list"
                    << endl;
      it = addresses.remove( it );
    }
    else
      ++it;
  }
  return addresses;
}


//-----------------------------------------------------------------------------
//static
bool KMMessage::addressIsInAddressList( const QString& address,
                                        const QStringList& addresses )
{
  QCString addrSpec = getEmailAddr( address ).lower();
  for( QStringList::ConstIterator it = addresses.begin();
       it != addresses.end(); ++it ) {
    if( addrSpec == getEmailAddr( *it ).lower() )
      return true;
  }
  return false;
}


//-----------------------------------------------------------------------------
void KMMessage::setTransferInProgress(bool value)
{
  value ? ++mTransferInProgress : --mTransferInProgress;
  Q_ASSERT(mTransferInProgress >= 0 && mTransferInProgress <= 1);
}


//-----------------------------------------------------------------------------
void KMMessage::readConfig(void)
{
  KConfig *config=KMKernel::config();
  KConfigGroupSaver saver(config, "General");

  config->setGroup("General");

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
      sReplySubjPrefixes << "Re\\s*:" << "Re\\[\\d+\\]:" << "Re\\d+:";
    sReplaceSubjPrefix = config->readBoolEntry("replace-reply-prefix", true);
    sForwardSubjPrefixes = config->readListEntry("forward-prefixes", ',');
    if (sForwardSubjPrefixes.count() == 0)
      sForwardSubjPrefixes << "Fwd:" << "FW:";
    sReplaceForwSubjPrefix = config->readBoolEntry("replace-forward-prefix", true);

    sSmartQuote = config->readBoolEntry("smart-quote", true);
    sWrapCol = config->readNumEntry("break-at", 78);
    if ((sWrapCol == 0) || (sWrapCol > 78))
      sWrapCol = 78;
    if (sWrapCol < 30)
      sWrapCol = 30;

    sPrefCharsets = config->readListEntry("pref-charsets");
  }

  { // area for config group "Reader"
    KConfigGroupSaver saver(config, "Reader");
    sHeaderStrategy = HeaderStrategy::create( config->readEntry( "header-set-displayed", "rich" ) );
  }
}

QCString KMMessage::defaultCharset()
{
  QCString retval;

  if (!sPrefCharsets.isEmpty())
    retval = sPrefCharsets[0].latin1();

  if (retval.isEmpty()  || (retval == "locale"))
    retval = QCString(kernel->networkCodec()->mimeName()).lower();

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
  DwMediaType &mType = dwContentType();
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
  else
    mType.SetModified();
  param->SetValue(DwString(aStr));
  mType.Assemble();
}


//-----------------------------------------------------------------------------
void KMMessage::setStatus(const KMMsgStatus aStatus, int idx)
{
  if (mStatus == aStatus)
    return;
  KMMsgBase::setStatus(aStatus, idx);
}

void KMMessage::setEncryptionState(const KMMsgEncryptionState s, int idx)
{
    if( mEncryptionState == s )
        return;
    mEncryptionState = s;
    mDirty = true;
    KMMsgBase::setEncryptionState(s, idx);
}

void KMMessage::setSignatureState(KMMsgSignatureState s, int idx)
{
    if( mSignatureState == s )
        return;
    mSignatureState = s;
    mDirty = true;
    KMMsgBase::setSignatureState(s, idx);
}

void KMMessage::setMDNSentState( KMMsgMDNSentState status, int idx ) {
  if ( mMDNSentState == status )
    return;
  mMDNSentState = status;
  mDirty = true;
  KMMsgBase::setMDNSentState( status, idx );
}

//-----------------------------------------------------------------------------
void KMMessage::link(const KMMessage *aMsg, KMMsgStatus aStatus)
{
  Q_ASSERT(aStatus == KMMsgStatusReplied || aStatus == KMMsgStatusForwarded);

  QString message = headerField("X-KMail-Link-Message");
  if (!message.isEmpty())
    message += ',';
  QString type = headerField("X-KMail-Link-Type");
  if (!type.isEmpty())
    type += ',';

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

void KMMessage::setBodyFromUnicode( const QString & str ) {
  QCString encoding = KMMsgBase::autoDetectCharset( charset(), KMMessage::preferredCharsets(), str );
  if ( encoding.isEmpty() )
    encoding = "utf-8";
  const QTextCodec * codec = KMMsgBase::codecForName( encoding );
  assert( codec );
  QValueList<int> dummy;
  setCharset( encoding );
  setBodyAndGuessCte( codec->fromUnicode( str ), dummy, false /* no 8bit */ );
}

const QTextCodec * KMMessage::codec() const {
  const QTextCodec * c = mOverrideCodec;
  if ( !c )
    // no override-codec set for this message, try the CT charset parameter:
    c = KMMsgBase::codecForName( charset() );
  if ( !c )
    // no charset means us-ascii (RFC 2045), so using local encoding should
    // be okay
    c = kernel->networkCodec();
  assert( c );
  return c;
}

QString KMMessage::bodyToUnicode(const QTextCodec* codec) const {
  if ( !codec )
    // No codec was given, so try the charset in the mail
    codec = this->codec();
  assert( codec );

  return codec->toUnicode( bodyDecoded() );
}
