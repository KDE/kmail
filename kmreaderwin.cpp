// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

// #define STRICT_RULES_OF_GERMAN_GOVERNMENT_02

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
// #define KMAIL_READER_HTML_DEBUG

#include <config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <qclipboard.h>
#include <qhbox.h>
#include <qstyle.h>
#include <qtextcodec.h>
#include <qpaintdevicemetrics.h>
#include <qstringlist.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpgp.h>
#include <kpgpblock.h>
#include <krun.h>
#include <ktempfile.h>
#include <kprocess.h>
#include <kstdguiitem.h>

// khtml headers
#include <khtml_part.h>
#include <khtmlview.h> // So that we can get rid of the frames
#include <dom/html_element.h>
#include <dom/html_block.h>


#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

#include <kmime_mdn.h>
using namespace KMime;

#include "kmversion.h"
#include "kmglobal.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include "kmgroupware.h"

#include "kbusyptr.h"
#include "kfileio.h"
#include "kmfolderindex.h"
#include "kmcommands.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsgpartdlg.h"
#include "kmtextbrowser.h"
#include "kmreaderwin.h"
#include "partNode.h"
#include "linklocator.h"
#include "kmmsgdict.h"
#include "kmsender.h"
#include "mailinglist-magic.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;
#include "partmetadata.h"
using KMail::PartMetaData;
#include "attachmentstrategy.h"
using KMail::AttachmentStrategy;
#include "headerstrategy.h"
using KMail::HeaderStrategy;
#include "headerstyle.h"
using KMail::HeaderStyle;
#include "khtmlparthtmlwriter.h"
using KMail::HtmlWriter;
using KMail::KHtmlPartHtmlWriter;

#ifdef KMAIL_READER_HTML_DEBUG
#include "filehtmlwriter.h"
using KMail::FileHtmlWriter;
#include "teehtmlwriter.h"
using KMail::TeeHtmlWriter;
#endif // !NDEBUG

// for the MIME structure viewer (khz):
#include "kmmimeparttree.h"


// X headers...
#undef Never
#undef Always

//--- Sven's save attachments to /tmp start ---
#include <unistd.h>
#include <kstandarddirs.h>  // for access and getpid
#include <kglobalsettings.h>
//--- Sven's save attachments to /tmp end ---

// for the click on attachment stuff (dnaber):
#include <kuserprofile.h>

// Do the tmp stuff correctly - thanks to Harri Porten for
// reminding me (sven)

#include "vcard.h"
#include "kmdisplayvcard.h"
#include <kpopupmenu.h>
#include <qimage.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

class NewByteArray : public QByteArray
{
public:
    NewByteArray &appendNULL();
    NewByteArray &operator+=( const char * );
    NewByteArray &operator+=( const QByteArray & );
    NewByteArray &operator+=( const QCString & );
    QByteArray& qByteArray();
};

NewByteArray& NewByteArray::appendNULL()
{
    QByteArray::detach();
    uint len1 = size();
    if ( !QByteArray::resize( len1 + 1 ) )
        return *this;
    *(data() + len1) = '\0';
    return *this;
}
NewByteArray& NewByteArray::operator+=( const char * newData )
{
    if ( !newData )
        return *this;
    QByteArray::detach();
    uint len1 = size();
    uint len2 = qstrlen( newData );
    if ( !QByteArray::resize( len1 + len2 ) )
        return *this;
    memcpy( data() + len1, newData, len2 );
    return *this;
}
NewByteArray& NewByteArray::operator+=( const QByteArray & newData )
{
    if ( newData.isNull() )
        return *this;
    QByteArray::detach();
    uint len1 = size();
    uint len2 = newData.size();
    if ( !QByteArray::resize( len1 + len2 ) )
        return *this;
    memcpy( data() + len1, newData.data(), len2 );
    return *this;
}
NewByteArray& NewByteArray::operator+=( const QCString & newData )
{
    if ( newData.isEmpty() )
        return *this;
    QByteArray::detach();
    uint len1 = size();
    uint len2 = newData.length(); // forget about the trailing 0x00 !
    if ( !QByteArray::resize( len1 + len2 ) )
        return *this;
    memcpy( data() + len1, newData.data(), len2 );
    return *this;
}
QByteArray& NewByteArray::qByteArray()
{
    return *((QByteArray*)this);
}



// This function returns the complete data that were in this
// message parts - *after* all encryption has been removed that
// could be removed.
// - This is used to store the message in decrypted form.
void KMReaderWin::objectTreeToDecryptedMsg( partNode* node,
                                            NewByteArray& resultingData,
                                            KMMessage& theMessage,
                                            bool weAreReplacingTheRootNode,
                                            int recCount )
{
  kdDebug(5006) << QString("-------------------------------------------------" ) << endl;
  kdDebug(5006) << QString("KMReaderWin::objectTreeToDecryptedMsg( %1 )  START").arg( recCount ) << endl;
  if( node ) {
    partNode* curNode = node;
    partNode* dataNode = curNode;
    bool bIsMultipart = false;

    switch( curNode->type() ){
      case DwMime::kTypeText: {
kdDebug(5006) << "* text *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeHtml:
kdDebug(5006) << "html" << endl;
            break;
          case DwMime::kSubtypeXVCard:
kdDebug(5006) << "v-card" << endl;
            break;
          case DwMime::kSubtypeRichtext:
kdDebug(5006) << "rich text" << endl;
            break;
          case DwMime::kSubtypeEnriched:
kdDebug(5006) << "enriched " << endl;
            break;
          case DwMime::kSubtypePlain:
kdDebug(5006) << "plain " << endl;
            break;
          default:
kdDebug(5006) << "default " << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeMultipart: {
kdDebug(5006) << "* multipart *" << endl;
          bIsMultipart = true;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMixed:
kdDebug(5006) << "mixed" << endl;
            break;
          case DwMime::kSubtypeAlternative:
kdDebug(5006) << "alternative" << endl;
            break;
          case DwMime::kSubtypeDigest:
kdDebug(5006) << "digest" << endl;
            break;
          case DwMime::kSubtypeParallel:
kdDebug(5006) << "parallel" << endl;
            break;
          case DwMime::kSubtypeSigned:
kdDebug(5006) << "signed" << endl;
            break;
          case DwMime::kSubtypeEncrypted: {
kdDebug(5006) << "encrypted" << endl;
              if( curNode->mChild ) {
                /*
                    ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                */
                partNode* data =
                  curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeOctetStream, false, true );
                if( !data ) {
                  data = curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Mime, false, true );
                }
                if( data && data->mChild )
                  dataNode = data;
              }
            }
            break;
          default :
kdDebug(5006) << "(  unknown subtype  )" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeMessage: {
kdDebug(5006) << "* message *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeRfc822: {
kdDebug(5006) << "RfC 822" << endl;
              if( curNode->mChild )
                dataNode = curNode->mChild;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeApplication: {
kdDebug(5006) << "* application *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypePostscript:
kdDebug(5006) << "postscript" << endl;
            break;
          case DwMime::kSubtypeOctetStream: {
kdDebug(5006) << "octet stream" << endl;
              if( curNode->mChild )
                dataNode = curNode->mChild;
            }
            break;
          case DwMime::kSubtypePgpEncrypted:
kdDebug(5006) << "pgp encrypted" << endl;
            break;
          case DwMime::kSubtypePgpSignature:
kdDebug(5006) << "pgp signed" << endl;
            break;
          case DwMime::kSubtypePkcs7Mime: {
kdDebug(5006) << "pkcs7 mime" << endl;
              // note: subtype Pkcs7Mime can also be signed
              //       and we do NOT want to remove the signature!
              if( ( curNode->encryptionState() != KMMsgNotEncrypted )
                  && curNode->mChild )
                dataNode = curNode->mChild;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeImage: {
kdDebug(5006) << "* image *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeJpeg:
kdDebug(5006) << "JPEG" << endl;
            break;
          case DwMime::kSubtypeGif:
kdDebug(5006) << "GIF" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeAudio: {
kdDebug(5006) << "* audio *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeBasic:
kdDebug(5006) << "basic" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeVideo: {
kdDebug(5006) << "* video *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMpeg:
kdDebug(5006) << "mpeg" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeModel:
kdDebug(5006) << "* model *" << endl;
        break;
    }


    DwHeaders& rootHeaders( theMessage.headers() );
    DwBodyPart * part = dataNode->dwPart() ? dataNode->dwPart() : 0;
    DwHeaders * headers(
        (part && part->hasHeaders())
        ? &part->Headers()
        : (  (weAreReplacingTheRootNode || !dataNode->mRoot)
            ? &rootHeaders
            : 0 ) );
    if( dataNode == curNode ) {
kdDebug(5006) << "dataNode == curNode:  Save curNode without replacing it." << endl;

      // A) Store the headers of this part IF curNode is not the root node
      //    AND we are not replacing a node that allready *has* replaced
      //    the root node in previous recursion steps of this function...
      if( headers ) {
        if( dataNode->mRoot && !weAreReplacingTheRootNode ) {
kdDebug(5006) << "dataNode is NOT replacing the root node:  Store the headers." << endl;
          resultingData += headers->AsString().c_str();
        } else if( weAreReplacingTheRootNode && part->hasHeaders() ){
kdDebug(5006) << "dataNode replace the root node:  Do NOT store the headers but change" << endl;
kdDebug(5006) << "                                 the Message's headers accordingly." << endl;
kdDebug(5006) << "              old Content-Type = " << rootHeaders.ContentType().AsString().c_str() << endl;
kdDebug(5006) << "              new Content-Type = " << headers->ContentType(   ).AsString().c_str() << endl;
          rootHeaders.ContentType()             = headers->ContentType();
          theMessage.setContentTransferEncodingStr(
              headers->HasContentTransferEncoding()
            ? headers->ContentTransferEncoding().AsString().c_str()
            : "" );
          rootHeaders.ContentDescription() = headers->ContentDescription();
          rootHeaders.ContentDisposition() = headers->ContentDisposition();
          theMessage.setNeedsAssembly();
        }
      }

      // B) Store the body of this part.
      if( headers && bIsMultipart && dataNode->mChild )  {
kdDebug(5006) << "is valid Multipart, processing children:" << endl;
        QCString boundary = headers->ContentType().Boundary().c_str();
        curNode = dataNode->mChild;
        // store children of multipart
        while( curNode ) {
kdDebug(5006) << "--boundary" << endl;
          if( resultingData.size() &&
              ( '\n' != resultingData.at( resultingData.size()-1 ) ) )
            resultingData += QCString( "\n" );
          resultingData += QCString( "\n" );
          resultingData += "--";
          resultingData += boundary;
          resultingData += "\n";
          // note: We are processing a harmless multipart that is *not*
          //       to be replaced by one of it's children, therefor
          //       we set their doStoreHeaders to true.
          objectTreeToDecryptedMsg( curNode,
                                    resultingData,
                                    theMessage,
                                    false,
                                    recCount + 1 );
          curNode = curNode->mNext;
        }
kdDebug(5006) << "--boundary--" << endl;
        resultingData += "\n--";
        resultingData += boundary;
        resultingData += "--\n\n";
kdDebug(5006) << "Multipart processing children - DONE" << endl;
      } else if( part ){
        // store simple part
kdDebug(5006) << "is Simple part or invalid Multipart, storing body data .. DONE" << endl;
        resultingData += part->Body().AsString().c_str();
      }
    } else {
kdDebug(5006) << "dataNode != curNode:  Replace curNode by dataNode." << endl;
      bool rootNodeReplaceFlag = weAreReplacingTheRootNode || !curNode->mRoot;
      if( rootNodeReplaceFlag ) {
kdDebug(5006) << "                      Root node will be replaced." << endl;
      } else {
kdDebug(5006) << "                      Root node will NOT be replaced." << endl;
      }
      // store special data to replace the current part
      // (e.g. decrypted data or embedded RfC 822 data)
      objectTreeToDecryptedMsg( dataNode,
                                resultingData,
                                theMessage,
                                rootNodeReplaceFlag,
                                recCount + 1 );
    }
  }
  kdDebug(5006) << QString("\nKMReaderWin::objectTreeToDecryptedMsg( %1 )  END").arg( recCount ) << endl;
}


/*
 ===========================================================================


        E N D    O F     T E M P O R A R Y     M I M E     C O D E


 ===========================================================================
*/













const int KMReaderWin::delay = 150;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent,
			 QWidget *mainWindow,
			 KActionCollection* actionCollection,
			 KMMimePartTree* mimePartTree,
                         int* showMIMETreeMode,
                         const char *aName,
                         int aFlags )
  : KMReaderWinInherited(aParent, aName, aFlags | Qt::WDestructiveClose),
    mAttachmentStrategy( 0 ),
    mHeaderStrategy( 0 ),
    mHeaderStyle( 0 ),
    mShowCompleteMessage( false ),
    mMimePartTree( mimePartTree ),
    mShowMIMETreeMode( showMIMETreeMode ),
    mRootNode( 0 ),
    mMainWindow( mainWindow ),
    mActionCollection( actionCollection ),
    mHtmlWriter( 0 )
{
  mUseGroupware = false;
  mAutoDelete = false;
  mLastSerNum = 0;
  mMessage = 0;
  mLastStatus = KMMsgStatusUnknown;
  mMsgDisplay = true;
  mPrinting = false;
  mShowColorbar = false;
  mInlineImage = false;
  mIsFirstTextPart = true;

  initHtmlWidget();
  readConfig();
  mHtmlOverride = false;

  connect( &updateReaderWinTimer, SIGNAL(timeout()),
  	   this, SLOT(updateReaderWin()) );
  connect( &mResizeTimer, SIGNAL(timeout()),
  	   this, SLOT(slotDelayedResize()) );
  connect( &mDelayedMarkTimer, SIGNAL(timeout()),
           this, SLOT(slotTouchMessage()) );

  mCodec = 0;
  mAutoDetectEncoding = true;

  if(getenv("KMAIL_DEBUG_READER_CRYPTO") != 0){
    QCString cE = getenv("KMAIL_DEBUG_READER_CRYPTO");
    mDebugReaderCrypto = cE == "1" || cE.upper() == "ON" || cE.upper() == "TRUE";
    kdDebug(5006) << "KMAIL_DEBUG_READER_CRYPTO = TRUE" << endl;
  }else{
    mDebugReaderCrypto = false;
    kdDebug(5006) << "KMAIL_DEBUG_READER_CRYPTO = FALSE" << endl;
  }

  if (!mActionCollection)
      return;
  KActionCollection *ac = mActionCollection;

  mMailToComposeAction = new KAction( i18n("Send To..."), 0, this,
				    SLOT(slotMailtoCompose()), ac,
				    "mailto_compose" );
  mMailToReplyAction = new KAction( i18n("Send Reply To..."), 0, this,
				    SLOT(slotMailtoReply()), ac,
				    "mailto_reply" );
  mMailToForwardAction = new KAction( i18n("Forward To..."),
				    0, this, SLOT(slotMailtoForward()), ac,
				    "mailto_forward" );
  mAddAddrBookAction = new KAction( i18n("Add to Address Book"),
				    0, this, SLOT(slotMailtoAddAddrBook()),
				    ac, "add_addr_book" );
  mOpenAddrBookAction = new KAction( i18n("Open in Address Book"),
				    0, this, SLOT(slotMailtoOpenAddrBook()),
				    ac, "openin_addr_book" );
  mCopyAction = new KAction( i18n("Copy to Clipboard"), 0, this,
			     SLOT(slotUrlCopy()), ac, "copy_url" );
  mUrlOpenAction = new KAction( i18n("Open URL"), 0, this,
			     SLOT(slotUrlOpen()), ac, "open_url" );
  mUrlSaveAsAction = new KAction( i18n("Save Link As..."), 0, this,
			     SLOT(slotUrlSave()), ac, "saveas_url" );
  mReplyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
			      SLOT(slotReplyToMsg()), ac, "reply" );
  mReplyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
				 Key_A, this, SLOT(slotReplyAllToMsg()),
				 ac, "reply_all" );
  mReplyListAction = new KAction( i18n("Reply to Mailing-&List..."),
				  "mail_replylist", Key_L, this,
				  SLOT(slotReplyListToMsg()), ac,
				  "reply_list" );

  mForwardActionMenu = new KActionMenu( i18n("Message->","&Forward"),
					"mail_forward", ac,
					"message_forward" );
  connect( mForwardActionMenu, SIGNAL(activated()), this,
	   SLOT(slotForwardMsg()) );
  mForwardAction = new KAction( i18n("&Inline..."), "mail_forward",
				SHIFT+Key_F, this, SLOT(slotForwardMsg()),
				ac, "message_forward_inline" );
  mForwardActionMenu->insert( mForwardAction );
  mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", Key_F, this,
					SLOT(slotForwardAttachedMsg()), ac,
					"message_forward_as_attachment" );
  mForwardActionMenu->insert( mForwardAttachedAction );
  mRedirectAction = new KAction( i18n("Message->Forward->","&Redirect..."),
				 Key_E, this, SLOT(slotRedirectMsg()),
				 ac, "message_forward_redirect" );
  mForwardActionMenu->insert( mRedirectAction );
  mNoQuoteReplyAction = new KAction( i18n("Reply Without &Quote..."), SHIFT+Key_R,
    this, SLOT(slotNoQuoteReplyToMsg()), ac, "noquotereply" );

  //---- Bounce action
  mBounceAction = new KAction( i18n("&Bounce..."), 0, this,
			      SLOT(slotBounceMsg()), ac, "bounce" );

  //----- Create filter actions
  mFilterMenu = new KActionMenu( i18n("&Create Filter"), ac, "create_filter" );

  mSubjectFilterAction = new KAction( i18n("Filter on &Subject..."), 0, this,
				      SLOT(slotSubjectFilter()),
				      ac, "subject_filter");
  mFilterMenu->insert( mSubjectFilterAction );

  mFromFilterAction = new KAction( i18n("Filter on &From..."), 0, this,
				   SLOT(slotFromFilter()),
				   ac, "from_filter");
  mFilterMenu->insert( mFromFilterAction );

  mToFilterAction = new KAction( i18n("Filter on &To..."), 0, this,
				 SLOT(slotToFilter()),
				 ac, "to_filter");
  mFilterMenu->insert( mToFilterAction );

  mListFilterAction = new KAction( i18n("Filter on Mailing-&List..."), 0, this,
                                   SLOT(slotMailingListFilter()), ac,
                                   "mlist_filter");
  mFilterMenu->insert( mListFilterAction );

  mToggleFixFontAction = new KToggleAction( i18n("Use Fi&xed Font"),
			Key_X, this, SLOT(slotToggleFixedFont()),
			ac, "toggle_fixedfont" );

  mViewSourceAction = new KAction( i18n("&View Source"), Key_V, this,
		      SLOT(slotShowMsgSrc()), ac, "view_source" );

  mPrintAction = KStdAction::print (this, SLOT(slotPrintMsg()), ac);

}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  delete mHtmlWriter; mHtmlWriter = 0;
  if (mAutoDelete) delete message();
  delete mRootNode;
  removeTempFiles();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMimePartTree( KMMimePartTree* mimePartTree )
{
  mMimePartTree = mimePartTree;
}

//-----------------------------------------------------------------------------
void KMReaderWin::setUseGroupware( bool on )
{
  mUseGroupware = on;
}

//-----------------------------------------------------------------------------
void KMReaderWin::removeTempFiles()
{
  for (QStringList::Iterator it = mTempFiles.begin(); it != mTempFiles.end();
    it++)
  {
    QFile::remove(*it);
  }
  mTempFiles.clear();
  for (QStringList::Iterator it = mTempDirs.begin(); it != mTempDirs.end();
    it++)
  {
    QDir(*it).rmdir(*it);
  }
  mTempDirs.clear();
}


//-----------------------------------------------------------------------------
bool KMReaderWin::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     if (message())
	 message()->readConfig();
     update( true ); // Force update
     return true;
  }
  return KMReaderWinInherited::event(e);
}



//-----------------------------------------------------------------------------
void KMReaderWin::readColorConfig(void)
{
  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Reader");

  c1 = QColor(kapp->palette().active().text());
  c2 = KGlobalSettings::linkColor();
  c3 = KGlobalSettings::visitedLinkColor();
  c4 = QColor(kapp->palette().active().base());
  cHtmlWarning = QColor( 0xFF, 0x40, 0x40 ); // warning frame color: light red

  // The default colors are also defined in configuredialog.cpp
  cPgpEncrH = QColor( 0x00, 0x80, 0xFF ); // light blue
  cPgpOk1H  = QColor( 0x40, 0xFF, 0x40 ); // light green
  cPgpOk0H  = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
  cPgpWarnH = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
  cPgpErrH  = QColor( 0xFF, 0x00, 0x00 ); // red
  cCBnoHtmlB = Qt::lightGray;
  cCBnoHtmlF = Qt::black;
  cCBisHtmlB = Qt::black;
  cCBisHtmlF = Qt::white;

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    c1 = config->readColorEntry("ForegroundColor",&c1);
    c2 = config->readColorEntry("LinkColor",&c2);
    c3 = config->readColorEntry("FollowedColor",&c3);
    c4 = config->readColorEntry("BackgroundColor",&c4);
    cPgpEncrH = config->readColorEntry( "PGPMessageEncr", &cPgpEncrH );
    cPgpOk1H  = config->readColorEntry( "PGPMessageOkKeyOk", &cPgpOk1H );
    cPgpOk0H  = config->readColorEntry( "PGPMessageOkKeyBad", &cPgpOk0H );
    cPgpWarnH = config->readColorEntry( "PGPMessageWarn", &cPgpWarnH );
    cPgpErrH  = config->readColorEntry( "PGPMessageErr", &cPgpErrH );
    cHtmlWarning = config->readColorEntry( "HTMLWarningColor", &cHtmlWarning );
    cCBnoHtmlB = config->readColorEntry( "ColorbarBackgroundPlain", &cCBnoHtmlB );
    cCBnoHtmlF = config->readColorEntry( "ColorbarForegroundPlain", &cCBnoHtmlF );
    cCBisHtmlB = config->readColorEntry( "ColorbarBackgroundHTML",  &cCBisHtmlB );
    cCBisHtmlF = config->readColorEntry( "ColorbarForegroundHTML",  &cCBisHtmlF );
  }

  // determine the frame and body color for PGP messages from the header color
  // if the header color equals the background color then the other colors are
  // also set to the background color (-> old style PGP message viewing)
  // else
  // the brightness of the frame is set to 4/5 of the brightness of the header
  // the saturation of the body is set to 1/8 of the saturation of the header
  int h,s,v;
  if ( cPgpOk1H == c4 )
  { // header color == background color?
    cPgpOk1F = c4;
    cPgpOk1B = c4;
  }
  else
  {
    cPgpOk1H.hsv( &h, &s, &v );
    cPgpOk1F.setHsv( h, s, v*4/5 );
    cPgpOk1B.setHsv( h, s/8, v );
  }
  if ( cPgpOk0H == c4 )
  { // header color == background color?
    cPgpOk0F = c4;
    cPgpOk0B = c4;
  }
  else
  {
    cPgpOk0H.hsv( &h, &s, &v );
    cPgpOk0F.setHsv( h, s, v*4/5 );
    cPgpOk0B.setHsv( h, s/8, v );
  }
  if ( cPgpWarnH == c4 )
  { // header color == background color?
    cPgpWarnF = c4;
    cPgpWarnB = c4;
  }
  else
  {
    cPgpWarnH.hsv( &h, &s, &v );
    cPgpWarnF.setHsv( h, s, v*4/5 );
    cPgpWarnB.setHsv( h, s/8, v );
  }
  if ( cPgpErrH == c4 )
  { // header color == background color?
    cPgpErrF = c4;
    cPgpErrB = c4;
  }
  else
  {
    cPgpErrH.hsv( &h, &s, &v );
    cPgpErrF.setHsv( h, s, v*4/5 );
    cPgpErrB.setHsv( h, s/8, v );
  }

  if ( cPgpEncrH == c4 )
  { // header color == background color?
    cPgpEncrF = c4;
    cPgpEncrB = c4;
  }
  else
  {
    cPgpEncrH.hsv( &h, &s, &v );
    cPgpEncrF.setHsv( h, s, v*4/5 );
    cPgpEncrB.setHsv( h, s/8, v );
  }

  //
  // Prepare the quoted fonts
  //
  mQuoteFontTag[0] = quoteFontTag(0);
  mQuoteFontTag[1] = quoteFontTag(1);
  mQuoteFontTag[2] = quoteFontTag(2);
}

//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = KMKernel::config();
  QString encoding;

  { // block defines the lifetime of KConfigGroupSaver
  KConfigGroupSaver saver(config, "Pixmaps");
  mBackingPixmapOn = FALSE;
  mBackingPixmapStr = config->readEntry("Readerwin","");
  if (!mBackingPixmapStr.isEmpty())
    mBackingPixmapOn = TRUE;
  }

  {
  KConfigGroupSaver saver(config, "Reader");
  // initialize useFixedFont from the saved value; the corresponding toggle
  // action is initialized in the main window
  mUseFixedFont = config->readBoolEntry( "useFixedFont", false );
  mHtmlMail = config->readBoolEntry( "htmlMail", false );
  setHeaderStyleAndStrategy( HeaderStyle::create( config->readEntry( "header-style", "fancy" ) ),
			     HeaderStrategy::create( config->readEntry( "header-set-displayed", "rich" ) ) );

  mAttachmentStrategy =
    AttachmentStrategy::create( config->readEntry( "attachment-strategy" ) );

  mViewer->setOnlyLocalReferences( !config->readBoolEntry( "htmlLoadExternal", false ) );

  // if the user uses OpenPGP then the color bar defaults to enabled
  // else it defaults to disabled
  if( Kpgp::Module::getKpgp()->usePGP() )
    mShowColorbar = true;
  else
    mShowColorbar = false;
  mShowColorbar = config->readBoolEntry( "showColorbar", mShowColorbar );
  // if the value defaults to enabled and KMail (with color bar) is used for
  // the first time the config dialog doesn't know this if we don't save the
  // value now
  config->writeEntry( "showColorbar", mShowColorbar );
  }

  {
  KConfigGroupSaver saver(config, "Fonts");
  mBodyFont = KGlobalSettings::generalFont();
  mFixedFont = KGlobalSettings::fixedFont();
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    mBodyFont = config->readFontEntry((mPrinting) ? "print-font" : "body-font",
      &mBodyFont);
    mFixedFont = config->readFontEntry("fixed-font", &mFixedFont);
  }
  else {
    setFont(KGlobalSettings::generalFont());
  }
  mViewer->setStandardFont( bodyFontFamily() );
  }

  {
    KConfigGroup behaviour( KMKernel::config(), "Behaviour" );
    mDelayedMarkAsRead = behaviour.readBoolEntry( "DelayedMarkAsRead", true );
    mDelayedMarkTimeout = behaviour.readNumEntry( "DelayedMarkTime", 0 );
  }

  readColorConfig();

  if (message()) {
    update();
    message()->readConfig();
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig(bool aWithSync)
{
  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Reader");
  config->writeEntry( "useFixedFont", mUseFixedFont );
  if ( headerStyle() )
    config->writeEntry( "header-style", headerStyle()->name() );
  if ( headerStrategy() )
    config->writeEntry( "header-set-displayed", headerStrategy()->name() );
  if ( attachmentStrategy() )
    config->writeEntry("attachment-strategy",attachmentStrategy()->name());
  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
QString KMReaderWin::quoteFontTag( int quoteLevel )
{
  KConfig *config = KMKernel::config();

  QColor color;

  { // block defines the lifetime of KConfigGroupSaver
    KConfigGroupSaver saver(config, "Reader");
    if( config->readBoolEntry( "defaultColors", true ) == true )
    {
      color = QColor(kapp->palette().active().text());
    }
    else
    {
      if( quoteLevel == 0 ) {
	QColor defaultColor( 0x00, 0x80, 0x00 );
	color = config->readColorEntry( "QuotedText1", &defaultColor );
      } else if( quoteLevel == 1 ) {
	QColor defaultColor( 0x00, 0x70, 0x00 );
	color = config->readColorEntry( "QuotedText2", &defaultColor );
      } else if( quoteLevel == 2 ) {
	QColor defaultColor( 0x00, 0x60, 0x00 );
	color = config->readColorEntry( "QuotedText3", &defaultColor );
      } else
	color = QColor(kapp->palette().active().base());
    }
  }

  QFont font;
  {
    KConfigGroupSaver saver(config, "Fonts");
    if( config->readBoolEntry( "defaultFonts", true ) == true )
    {
      font = KGlobalSettings::generalFont();
      font.setItalic(true);
    }
    else
    {
      const QFont defaultFont = QFont("helvetica");
      if( quoteLevel == 0 )
	font  = config->readFontEntry( "quote1-font", &defaultFont );
      else if( quoteLevel == 1 )
	font  = config->readFontEntry( "quote2-font", &defaultFont );
      else if( quoteLevel == 2 )
	font  = config->readFontEntry( "quote3-font", &defaultFont );
      else
      {
	font = KGlobalSettings::generalFont();
	font.setItalic(true);
      }
    }
  }

  QString style;
  if( mPrinting )
    style = "color:#000000;";
  else
    style = QString( "color:%1;" ).arg( color.name() );
  if( font.italic() )
    style += "font-style:italic;";
  if( font.bold() )
    style += "font-weight:bold;";

  return QString( "<div style=\"%1\">" ).arg( style );
}


//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  mBox = new QHBox(this);

  mColorBar = new QLabel(" ", mBox);
  mColorBar->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  mColorBar->setEraseColor( mPrinting ? QColor( "white" ) : c4 );

  if ( !mShowColorbar )
    mColorBar->hide();


  mViewer = new KHTMLPart(mBox, "khtml");
  mViewer->widget()->setFocusPolicy(WheelFocus);
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->setPluginsEnabled(false);
  mViewer->setJScriptEnabled(false); // just make this explicit
  mViewer->setJavaEnabled(false);    // just make this explicit
  mViewer->setMetaRefreshEnabled(false);
  mViewer->setURLCursor(KCursor::handCursor());
  // Espen 2000-05-14: Getting rid of thick ugly frames
  mViewer->view()->setLineWidth(0);

  if ( !htmlWriter() )
#ifdef KMAIL_READER_HTML_DEBUG
    mHtmlWriter = new TeeHtmlWriter( new FileHtmlWriter( QString::null ),
				     new KHtmlPartHtmlWriter( mViewer, 0 ) );
#else
    mHtmlWriter = new KHtmlPartHtmlWriter( mViewer, 0 );
#endif

  connect(mViewer->browserExtension(),
          SIGNAL(openURLRequest(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotUrlOpen(const KURL &, const KParts::URLArgs &)));
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotUrlOpen(const KURL &, const KParts::URLArgs &)));
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotUrlPopup(const QString &, const QPoint &)));
}


void KMReaderWin::setAttachmentStrategy( const AttachmentStrategy * strategy ) {
  mAttachmentStrategy = strategy ? strategy : AttachmentStrategy::smart() ;
  update( true );
}

void KMReaderWin::setHeaderStyleAndStrategy( const HeaderStyle * style,
					     const HeaderStrategy * strategy ) {
  mHeaderStyle = style ? style : HeaderStyle::fancy() ;
  mHeaderStrategy = strategy ? strategy : HeaderStrategy::rich() ;
  update( true );
}

//-----------------------------------------------------------------------------
void KMReaderWin::setCodec(const QTextCodec *codec)
{
  mCodec = codec;
  if(!codec) {
    mAutoDetectEncoding = true;
    update(true);
    return;
  }
  mAutoDetectEncoding = false;
  update(true);
}

//-----------------------------------------------------------------------------
void KMReaderWin::setMsg(KMMessage* aMsg, bool force)
{
  if (aMsg)
      kdDebug(5006) << "(" << aMsg->getMsgSerNum() << ", last " << mLastSerNum << ") " << aMsg->subject() << " "
        << aMsg->fromStrip() << endl;

  // If not forced and there is aMsg and aMsg is same as mMsg then return
  if (!force && aMsg && mLastSerNum != 0 && aMsg->getMsgSerNum() == mLastSerNum)
    return;

  kdDebug(5006) << "set Msg, force = " << force << endl;

  // connect to the updates if we have hancy headers

  mDelayedMarkTimer.stop();

  mLastSerNum = (aMsg) ? aMsg->getMsgSerNum() : 0;

  // assume if a serial number exists it can be used to find the assoc KMMessage
  if (mLastSerNum <= 0)
    mMessage = aMsg;
  else
    mMessage = 0;
  if (message() != aMsg) {
    mMessage = aMsg;
    mLastSerNum = 0; // serial number was invalid
    Q_ASSERT(0);
  }

  mLastStatus = (aMsg) ? aMsg->status() : KMMsgStatusUnknown;
  if (aMsg)
  {
    aMsg->setCodec(mCodec);
    aMsg->setDecodeHTML(htmlMail());
  }

  // Avoid flicker, somewhat of a cludge
  if (force) {
    // stop the timer to avoid calling updateReaderWin twice
    updateReaderWinTimer.stop();
    updateReaderWin();
  }
  else if (updateReaderWinTimer.isActive())
    updateReaderWinTimer.changeInterval( delay );
  else
    updateReaderWinTimer.start( 0, TRUE );

  if (mDelayedMarkAsRead && (mDelayedMarkTimeout != 0))
    mDelayedMarkTimer.start( mDelayedMarkTimeout * 1000, TRUE );
  else
    slotTouchMessage();
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  if (mLastSerNum > 0) // no risk for a dangling pointer
    return;
  updateReaderWinTimer.stop();
  mDelayedMarkTimer.stop();
  mLastSerNum = 0;
  mMessage = 0;
}

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  I18N_NOOP("KMail is now a KPart and can be embedded in the Kontact container application along with other KDE PIM applications."),
  I18N_NOOP("Remove duplicates function for removing duplicate messages in a folder."),
  I18N_NOOP("Messages can be dragged and dropped on a composer window to add those messages as attachments."),
  I18N_NOOP("Deletion in threaded mode is improved, child messages will no longer be scattered when a parent is deleted."),
  I18N_NOOP("Multiple messages can now be selected in the search dialog."),
  I18N_NOOP("New context menu in the search dialog with Move, Copy, Reply etc. actions for operating on selected messages."),
  I18N_NOOP("Search criteria in the search dialog now supports more types of rules and a variable number of rules."),
  I18N_NOOP("Faster searching of large messages."),
  I18N_NOOP("'Search Folders' which are a KMail folder that stores a search expression and is dynamically updated (also known as virtual folders)."),
  I18N_NOOP("The separate window for reading mail has a context menu with Reply, Copy etc. actions for operating on the message displayed."),
  I18N_NOOP("The separate window for reading mail has a tool bar."),
  I18N_NOOP("Startup of KMail is faster."),
  I18N_NOOP("Switching between folders is faster."),
  I18N_NOOP("The contents of all composer windows are saved to disk on composer window creation and then periodically saved to prevent mail loss in the result of a system crash."),
  I18N_NOOP("The state of KMail folders is saved to disk periodically to prevent status information loss in the result of a system crash."),
  I18N_NOOP("You can select your startup folder")
};
static const int numKMailNewFeatures =
  sizeof kmailNewFeatures / sizeof *kmailNewFeatures;


//-----------------------------------------------------------------------------
void KMReaderWin::displayAboutPage()
{
  mColorBar->hide();
  mMsgDisplay = FALSE;
  QString location = locate("data", "kmail/about/main.html");
  QString content = kFileToString(location);
  mViewer->begin(location);
  QString info =
    i18n("%1: KMail version; %2: help:// URL; %3: homepage URL; "
	 "%4: prior KMail version; %5: prior KDE version; "
	 "%6: generated list of new features; "
	 "%7: First-time user text (only shown on first start)"
	 "--- end of comment ---",
	 "<h2>Welcome to KMail %1</h2><p>KMail is the email client for the K "
	 "Desktop Environment. It is designed to be fully compatible with "
	 "Internet mailing standards including MIME, SMTP, POP3 and IMAP."
	 "</p>\n"
	 "<ul><li>KMail has many powerful features which are described in the "
	 "<a href=\"%2\">documentation</a></li>\n"
	 "<li>The <a href=\"%3\">KMail homepage</A> offers information about "
	 "new versions of KMail</li></ul>\n"
	 "<p>Some of the new features in this release of KMail include "
	 "(compared to KMail %4, which is part of KDE %5):</p>\n"
	 "<ul>\n%6</ul>\n"
	 "%7\n"
	 "<p>We hope that you will enjoy KMail.</p>\n"
	 "<p>Thank you,</p>\n"
	 "<p>&nbsp; &nbsp; The KMail Team</p>")
    .arg(KMAIL_VERSION) // KMail version
    .arg("help:/kmail/index.html") // KMail help:// URL
    .arg("http://kmail.kde.org/") // KMail homepage URL
    .arg("1.5").arg("3.1"); // prior KMail and KDE version

  QString featureItems;
  for ( int i = 0 ; i < numKMailNewFeatures ; i++ )
    featureItems += i18n("<li>%1</li>\n").arg( i18n( kmailNewFeatures[i] ) );

  info = info.arg( featureItems );

  if( kernel->firstStart() ) {
    info = info.arg( i18n("<p>Please take a moment to fill in the KMail "
			  "configuration panel at Settings-&gt;Configure "
			  "KMail.\n"
			  "You need to create at least a default identity and "
			  "an incoming as well as outgoing mail account."
			  "</p>\n") );
  } else {
    info = info.arg( QString::null );
  }
  mViewer->write(content.arg(pointsToPixel( fontSize() )).arg(info));
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
  if (!mMsgDisplay) return;

  mViewer->view()->setUpdatesEnabled( false );
  mViewer->view()->viewport()->setUpdatesEnabled( false );
  static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);

  htmlWriter()->reset();

  KMFolder* folder;
  if (message(&folder))
  {
    if( !kernel->groupware().isGroupwareFolder( folder ) ){
      if ( mShowColorbar )
	mColorBar->show();
      else
	mColorBar->hide();
      parseMsg();
    }
  }
  else
  {
    mColorBar->hide();
    htmlWriter()->begin();
    htmlWriter()->write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
			"Transitional//EN\">\n<html><body" +
			QString(" bgcolor=\"%1\"").arg(c4.name()));

    if (mBackingPixmapOn)
      htmlWriter()->write(" background=\"file://" + mBackingPixmapStr + "\"");
    htmlWriter()->write("></body></html>");
    htmlWriter()->end();
    mViewer->view()->viewport()->setUpdatesEnabled( true );
    mViewer->view()->setUpdatesEnabled( true );
    mViewer->view()->viewport()->repaint( false );
    if( mMimePartTree )
      mMimePartTree->clear();
  }
}

//-----------------------------------------------------------------------------
int KMReaderWin::pointsToPixel(int pointSize) const
{
  QPaintDeviceMetrics const pdm(mViewer->view());

  return (pointSize * pdm.logicalDpiY() + 36) / 72;
}

//-----------------------------------------------------------------------------
void KMReaderWin::showHideMimeTree( bool showIt )
{
  if( mMimePartTree && ( !mShowMIMETreeMode || (0 != *mShowMIMETreeMode) ) ){
    if( showIt || (mShowMIMETreeMode && (*mShowMIMETreeMode == 2)) )
      mMimePartTree->show();
    else
      mMimePartTree->hide();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(void)
{
  KMMessage *msg = message();
  if ( msg == 0 )
    return;

  if( mMimePartTree )
    mMimePartTree->clear();

  int mainType = msg->type();
  bool isMultipart = ( DwMime::kTypeMultipart == mainType );

  showHideMimeTree( isMultipart );

  QString bkgrdStr = "";
  if (mBackingPixmapOn)
    bkgrdStr = " background=\"file://" + mBackingPixmapStr + "\"";

  htmlWriter()->begin();

  if (mAutoDetectEncoding) {
    mCodec = 0;
    QCString encoding;
    if( DwMime::kTypeText == mainType )
      encoding = msg->charset();
    else if ( isMultipart ) {
      if (msg->numBodyParts() > 0) {
        KMMessagePart msgPart;
        msg->bodyPart(0, &msgPart);
        encoding = msgPart.charset();
      }
    }
    if (encoding.isEmpty())
      encoding = kernel->networkCodec()->name();
    mCodec = KMMsgBase::codecForName(encoding);
  }

  if (!mCodec)
    mCodec = QTextCodec::codecForName("iso8859-1");
  msg->setCodec(mCodec);


//      QString( "table.rfc822 { width: 100%; "
//                 "border-style:solid; border-width: 8px; }\n" )

/*

#links {
  border-left-width:1cm;
  border-left-style:solid;
  border-color:red;
  padding-left:1cm;
  text-align:justify; }
#linksrechts {
  border-left-width:1cm;
  border-left-style:solid;
  border-left-color:red;
  padding-left:1cm;
  border-right-width:1cm;
  border-right-style:solid;
  border-right-color:green;
  padding-right:1cm;
  text-align:justify; }
#rundrum {
  border-width:1px;
  border-style:solid;
  border-color:blue;
  padding:1cm;
  text-align:justify; }

*/


  QColorGroup cg = kapp->palette().active();
  htmlWriter()->queue("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
		      "Transitional//EN\">\n<html><head><title></title>"
		      "<style type=\"text/css\">" +
    ((mPrinting) ? QString("body { font-family: \"%1\"; font-size: %2pt; "
                           "color: #000000; background-color: #FFFFFF; }\n")
        .arg( bodyFontFamily() ).arg( fontSize() )
      : QString("body { font-family: \"%1\"; font-size: %2px; "
        "color: %3; background-color: %4; }\n")
        .arg( bodyFontFamily() ).arg( pointsToPixel( fontSize() ) )
        .arg( mPrinting ? QString("#000000") : c1.name() )
        .arg( mPrinting ? QString("#FFFFFF") : c4.name() ) ) +
    ((mPrinting) ? QString("a { color: #000000; text-decoration: none; }")
      : QString("a { color: %1; ").arg(c2.name()) +
        "text-decoration: none; }" + // just playing

        QString( "table.encr { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpEncrF.name() ) +
        QString( "tr.encrH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpEncrH.name() ) +
        QString( "tr.encrB { background-color: %1; }\n" )
        .arg( cPgpEncrB.name() ) +

        QString( "table.rfc822 { width: 100%; "
                 "border-top-style: solid; "
                 "border-top-width: 1px; "
                 "border-top-color: black; "
                 "border-left-style: solid; "
                 "border-left-width: 1px; "
                 "border-left-color: black; "
                 "border-bottom-style: solid; "
                 "border-bottom-width: 1px; "
                 "border-bottom-color: black; "
                 "border-right-style: hidden; "
                 "border-right-width: 0px; "
                 "padding: 0px; } \n" ) +
        QString( "tr.rfc822H { font-weight: bold; }\n" ) +
        QString( "tr.rfc822B { font-weight: normal; }\n" ) +


        QString( "table.signOkKeyOk { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpOk1F.name() ) +
        QString( "tr.signOkKeyOkH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpOk1H.name() ) +
        QString( "tr.signOkKeyOkB { background-color: %1; }\n" )
        .arg( cPgpOk1B.name() ) +

        QString( "table.signOkKeyBad { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpOk0F.name() ) +
        QString( "tr.signOkKeyBadH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpOk0H.name() ) +
        QString( "tr.signOkKeyBadB { background-color: %1; }\n" )
        .arg( cPgpOk0B.name() ) +

        QString( "table.signWarn { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpWarnF.name() ) +
        QString( "tr.signWarnH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpWarnH.name() ) +
        QString( "tr.signWarnB { background-color: %1; }\n" )
        .arg( cPgpWarnB.name() ) +

        QString( "table.signErr { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpErrF.name() ) +
        QString( "tr.signErrH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpErrH.name() ) +
        QString( "tr.signErrB { background-color: %1; }\n" )
        .arg( cPgpErrB.name() )) +

        QString( "div.fancyHeaderSubj { background-color: %1; "
                                       "color: %2; padding: 4px; "
                                       "border: solid %3 1px; }\n"
		 "div.fancyHeaderSubj a[href] { color: %2; }"
		 "div.fancyHeaderSubj a[href]:hover { text-decoration: underline; }\n")
        .arg((mPrinting) ? cg.background().name() : cg.highlight().name())
        .arg((mPrinting) ? cg.foreground().name() : cg.highlightedText().name())
        .arg((mPrinting) ? cg.foreground().name() : cg.highlightedText().name())
        .arg(cg.foreground().name()) +
        QString( "div.fancyHeaderDtls { background-color: %1; color: %2; "
                                       "border-bottom: solid %3 1px; "
                                       "border-left: solid %4 1px; "
                                       "border-right: solid %5 1px; "
                                       "margin-bottom: 1em; "
                                       "padding: 2px; }\n" )
         .arg(cg.background().name())
         .arg(cg.foreground().name())
         .arg(cg.foreground().name())
         .arg(cg.foreground().name())
         .arg(cg.foreground().name()) +
         QString( "table.fancyHeaderDtls { width: 100%; "
                                          "border-width: 0px; "
                                          "align: left }\n"
                  "th.fancyHeaderDtls { padding: 0px; "
				       "white-space: nowrap; "
                                       "border-spacing: 0px; "
                                       "text-align: left; "
                                       "vertical-align: top; }\n"
                  "td.fancyHeaderDtls { padding: 0px; "
                                       "border-spacing: 0px; "
                                       "text-align: left; "
                                       "text-valign: top; "
                                       "width: 100%; }\n" ) +
         "</style></head>" +
		 // TODO: move these to stylesheet, too:
    ((mPrinting) ? QString("<body>") : QString("<body ") + bkgrdStr + ">" ));

  if (!parent())
    setCaption(msg->subject());

  parseMsg(msg);

  htmlWriter()->queue("</body></html>");
  htmlWriter()->flush();
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg, bool onlyProcessHeaders)
{
#ifndef NDEBUG
  QString s("\n#######\n#######\n#######  parseMsg(KMMessage* aMsg ");
  if( aMsg == message() )
    s += "==";
  else
    s += "!=";
  s += " aMsg, bool onlyProcessHeaders == ";
  if( onlyProcessHeaders )
    s += "true";
  else
    s += "false";
  s += "\n#######\n#######";
  kdDebug(5006) << s << endl;
#endif

  mColorBar->setEraseColor( QColor( "white" ) );
  mColorBar->setText("");

  if( !onlyProcessHeaders )
    removeTempFiles();
  KMMessagePart msgPart;
  QCString subtype, contDisp;
  QByteArray str;
  partNode* savedRootNode = 0;

  assert(aMsg!=0);

  QCString type = aMsg->typeStr();

  int mainType    = aMsg->type();
  int mainSubType = aMsg->subtype();
  QString mainCntTypeStr;
  if(    (DwMime::kTypeNull    == mainType)
      || (DwMime::kTypeUnknown == mainType) ){
    mainType    = DwMime::kTypeText;
    mainSubType = DwMime::kSubtypePlain;
    mainCntTypeStr = "text/plain";
  } else {
    mainCntTypeStr = aMsg->typeStr();
    int scpos = mainCntTypeStr.find(';');
    if( -1 < scpos)
      mainCntTypeStr.truncate( scpos );
  }

  VCard *vc = 0;
  bool hasVCard = false;

  // store message body in mRootNode if *no* body parts found
  // (please read the comment below before crying about me)  :-)
  DwBodyPart* mainBody = 0;
  DwBodyPart* firstBodyPart = aMsg->getFirstDwBodyPart();
  if( !firstBodyPart ) {
    // ATTENTION: This definitely /should/ be optimized.
    //            Copying the message text into a new body part
    //            surely is not the most efficient way to go.
    //            I decided to do so for being able to get a
    //            solution working for old style (== non MIME)
    //            mails without spending much time on implementing.
    //            During code revisal when switching to KMime
    //            all this will probably disappear anyway (or it
    //            will be optimized, resp.).       (khz, 6.12.2001)
    kdDebug(5006) << "*no* first body part found, creating one from Message" << endl;
    mainBody = new DwBodyPart(aMsg->asDwString(), 0);
    mainBody->Parse();
  }

  if( onlyProcessHeaders )
    savedRootNode = mRootNode;
  else
    delete mRootNode;

  mRootNode = new partNode( mainBody, mainType, mainSubType, true );
  mRootNode->setFromAddress( aMsg->from() );

  QString cntDesc = aMsg->subject();
  if( cntDesc.isEmpty() )
    cntDesc = i18n("( body part )");
  KIO::filesize_t cntSize = aMsg->msgSize();
  QString cntEnc;
  if( aMsg->contentTransferEncodingStr().isEmpty() )
    cntEnc = "7bit";
  else
    cntEnc = aMsg->contentTransferEncodingStr();

  if( firstBodyPart ) {
kdDebug(5006) << "\n     ----->  First body part *was* found, filling the Mime Part Tree" << endl;
    // store pointers to the MIME objects in our fast access tree
    partNode* curNode = mRootNode->setFirstChild( new partNode(firstBodyPart) );
    curNode->buildObjectTree();
    // fill the MIME part tree viewer
    if( mMimePartTree && !onlyProcessHeaders )
      mRootNode->fillMimePartTree( 0,
                                   mMimePartTree,
                                   cntDesc,
                                   mainCntTypeStr,
                                   cntEnc,
                                   cntSize );
  } else if( mMimePartTree && !onlyProcessHeaders ) {
kdDebug(5006) << "\n     ----->  Inserting Root Node into the Mime Part Tree" << endl;
    mRootNode->fillMimePartTree( 0,
                                 mMimePartTree,
                                 cntDesc,
                                 mainCntTypeStr,
                                 cntEnc,
                                 cntSize );
kdDebug(5006) << "\n     <-----  Finished inserting Root Node into Mime Part Tree" << endl;
  } else if(  !onlyProcessHeaders ){
kdDebug(5006) << "\n     ------  Sorry, no Mime Part Tree - can NOT insert Root Node!" << endl;
  }

  partNode* vCardNode = mRootNode->findType( DwMime::kTypeText, DwMime::kSubtypeXVCard );
  if( vCardNode ) {
    int vcerr;
    const QTextCodec *atmCodec = (mAutoDetectEncoding) ?
      KMMsgBase::codecForName(vCardNode->msgPart().charset()) : mCodec;
    if (!atmCodec) atmCodec = mCodec;
    vc = VCard::parseVCard(atmCodec->toUnicode(
            vCardNode->msgPart().bodyDecoded() ),
            &vcerr);
    if( vc ) {
      delete vc;
      kdDebug(5006) << "FOUND A VALID VCARD" << endl;
      hasVCard = true;
      writePartIcon(&vCardNode->msgPart(), aMsg->partNumber(vCardNode->dwPart()), TRUE );
    }
  }
  htmlWriter()->queue("<div id=\"header\">"
          + (writeMsgHeader(aMsg, hasVCard))
          + "</div><div><br></div>");


  mIsFirstTextPart = true;
  // show message content
  if( !onlyProcessHeaders ) {
    ObjectTreeParser otp( this );
    otp.parseObjectTree( mRootNode );
  }

  // store encrypted/signed status information in the KMMessage
  //  - this can only be done *after* calling parseObjectTree()
  KMMsgEncryptionState encryptionState = mRootNode->overallEncryptionState();
  KMMsgSignatureState  signatureState  = mRootNode->overallSignatureState();
  aMsg->setEncryptionState( encryptionState );
  aMsg->setSignatureState(  signatureState  );

  bool emitReplaceMsgByUnencryptedVersion = false;
// note: The following define is specified on top of this file. To compile
//       a less strict version of KMail just comment it out there above.
#ifdef STRICT_RULES_OF_GERMAN_GOVERNMENT_02

  // Hack to make sure the S/MIME CryptPlugs follows the strict requirement
  // of german government:
  // --> All received encrypted messages *must* be stored in unencrypted form
  //     after they have been decrypted once the user has read them.
  //     ( "Aufhebung der Verschluesselung nach dem Lesen" )
  //
  // note: Since there is no configuration option for this, we do that for
  //       all kinds of encryption now - *not* just for S/MIME.
  //       This could be changed in the objectTreeToDecryptedMsg() function
  //       by deciding when (or when not, resp.) to set the 'dataNode' to
  //       something different than 'curNode'.


kdDebug(5006) << "\n\n\nKMReaderWin::parseMsg()  -  special post-encryption handling:\n1." << endl;
kdDebug(5006) << "(!onlyProcessHeaders) = "                        << (!onlyProcessHeaders) << endl;
kdDebug(5006) << "(aMsg == msg) = "                               << (aMsg == message()) << endl;
kdDebug(5006) << "   (KMMsgStatusUnknown == mLastStatus) = "           << (KMMsgStatusUnknown == mLastStatus) << endl;
kdDebug(5006) << "|| (KMMsgStatusNew     == mLastStatus) = "           << (KMMsgStatusNew     == mLastStatus) << endl;
kdDebug(5006) << "|| (KMMsgStatusUnread  == mLastStatus) = "           << (KMMsgStatusUnread  == mLastStatus) << endl;
kdDebug(5006) << "(mIdOfLastViewedMessage != aMsg->msgId()) = "    << (mIdOfLastViewedMessage != aMsg->msgId()) << endl;
kdDebug(5006) << "   (KMMsgFullyEncrypted == encryptionState) = "     << (KMMsgFullyEncrypted == encryptionState) << endl;
kdDebug(5006) << "|| (KMMsgPartiallyEncrypted == encryptionState) = " << (KMMsgPartiallyEncrypted == encryptionState) << endl;
         // only proceed if we were called the normal way - not by
         // click in the MIME tree viewer
  if(    !onlyProcessHeaders
         // only proceed if we were called the normal way - not by
         // double click on the message (==not running in a separate window)

      && (aMsg == message())
         // only proceed if this message was not saved encryptedly before
         // to make sure only *new* messages are saved in decrypted form
      && (    (KMMsgStatusUnknown == mLastStatus)
           || (KMMsgStatusNew     == mLastStatus)
           || (KMMsgStatusUnread  == mLastStatus) )
         // avoid endless recursions
      && (mIdOfLastViewedMessage != aMsg->msgId())
         // only proceed if this message is (at least partially) encrypted
      && (    (KMMsgFullyEncrypted == encryptionState)
           || (KMMsgPartiallyEncrypted == encryptionState) ) ) {

kdDebug(5006) << "KMReaderWin  -  calling objectTreeToDecryptedMsg()" << endl;

    NewByteArray decryptedData;
    // note: The following call may change the message's headers.
    objectTreeToDecryptedMsg( mRootNode, decryptedData, *aMsg );
    // add a \0 to the data
    decryptedData.appendNULL();
    QCString resultString( decryptedData.data() );
kdDebug(5006) << "KMReaderWin  -  resulting data:" << resultString << endl;

    if( !resultString.isEmpty() ) {
kdDebug(5006) << "KMReaderWin  -  composing unencrypted message" << endl;
      // try this:
      aMsg->setBody( resultString );
      KMMessage* unencryptedMessage = new KMMessage( *aMsg );
      // because this did not work:
      /*
      DwMessage dwMsg( DwString( aMsg->asString() ) );
      dwMsg.Body() = DwBody( DwString( resultString.data() ) );
      dwMsg.Body().Parse();
      KMMessage* unencryptedMessage = new KMMessage( &dwMsg );
      */
kdDebug(5006) << "KMReaderWin  -  resulting message:" << unencryptedMessage->asString() << endl;
kdDebug(5006) << "KMReaderWin  -  attach unencrypted message to aMsg" << endl;
      aMsg->setUnencryptedMsg( unencryptedMessage );
      emitReplaceMsgByUnencryptedVersion = true;
    }
  }
#endif

  // save current main Content-Type before deleting mRootNode
  int rootNodeCntType = mRootNode ? mRootNode->type() : DwMime::kTypeUnknown;
  // if necessary restore original mRootNode
  if(onlyProcessHeaders) {
    delete mRootNode;
    mRootNode = savedRootNode;
  }
  // store message id to avoid endless recursions
  setIdOfLastViewedMessage( aMsg->msgId() );

  if( emitReplaceMsgByUnencryptedVersion ) {
kdDebug(5006) << "KMReaderWin  -  invoce saving in decrypted form:" << endl;
    emit replaceMsgByUnencryptedVersion();
  } else {
kdDebug(5006) << "KMReaderWin  -  finished parsing and displaying of message." << endl;
    if (!onlyProcessHeaders)
      showHideMimeTree( (DwMime::kTypeMultipart   == rootNodeCntType) ||
                        (DwMime::kTypeApplication == rootNodeCntType) ||
                        (DwMime::kTypeMessage     == rootNodeCntType) ||
                        (DwMime::kTypeModel       == rootNodeCntType) );
  }
  if( mColorBar->text().isEmpty() ) {
    mColorBar->setEraseColor( cCBnoHtmlB );
    mColorBar->setPaletteForegroundColor( cCBnoHtmlF );
    mColorBar->setTextFormat( Qt::PlainText );
    mColorBar->setText(i18n("\nN\no\n \nH\nT\nM\nL\n \nM\ne\ns\ns\na\ng\ne"));
  }
}

//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader(KMMessage* aMsg, bool hasVCard)
{
  kdFatal( !headerStyle(), 5006 )
    << "trying to writeMsgHeader() without a header style set!" << endl;
  kdFatal( !headerStrategy(), 5006 )
    << "trying to writeMsgHeader() without a header strategy set!" << endl;
  return headerStyle()->format( aMsg, headerStrategy(),
				hasVCard ? mTempFiles.last() : QString::null,
				mPrinting );
}



#define SIG_FRAME_COL_UNDEF  99
#define SIG_FRAME_COL_RED    -1
#define SIG_FRAME_COL_YELLOW  0
#define SIG_FRAME_COL_GREEN   1
QString KMReaderWin::sigStatusToString( CryptPlugWrapper* cryptPlug,
                                        int status_code,
                                        CryptPlugWrapper::SigStatusFlags statusFlags,
                                        int& frameColor,
                                        bool& showKeyInfos )
{
    // note: At the moment frameColor and showKeyInfos are
    //       used for CMS only but not for PGP signatures
    // pending(khz): Implement usage of these for PGP sigs as well.
    showKeyInfos = true;
    QString result;
    if( cryptPlug ) {
        if( 0 <= cryptPlug->libName().find( "gpgme-openpgp", 0, false ) ) {
            // process enum according to it's definition to be read in
            // GNU Privacy Guard CVS repository /gpgme/gpgme/gpgme.h
            switch( status_code ) {
            case 0: // GPGME_SIG_STAT_NONE
                result = i18n("Error: Signature not verified");
                break;
            case 1: // GPGME_SIG_STAT_GOOD
                result = i18n("Good signature");
                break;
            case 2: // GPGME_SIG_STAT_BAD
                result = i18n("BAD signature");
                break;
            case 3: // GPGME_SIG_STAT_NOKEY
                result = i18n("No public key to verify the signature");
                break;
            case 4: // GPGME_SIG_STAT_NOSIG
                result = i18n("No signature found");
                break;
            case 5: // GPGME_SIG_STAT_ERROR
                result = i18n("Error verifying the signature");
                break;
            case 6: // GPGME_SIG_STAT_DIFF
                result = i18n("Different results for signatures");
                break;
            /* PENDING(khz) Verify exact meaning of the following values:
            case 7: // GPGME_SIG_STAT_GOOD_EXP
                return i18n("Signature certificate is expired");
            break;
            case 8: // GPGME_SIG_STAT_GOOD_EXPKEY
                return i18n("One of the certificate's keys is expired");
            break;
            */
            default:
                result = "";   // do *not* return a default text here !
                break;
            }
        } else
        if( 0 <= cryptPlug->libName().find( "gpgme-smime",   0, false ) ) {
            // process status bits according to SigStatus_...
            // definitions in kdenetwork/libkdenetwork/cryptplug.h

            if( CryptPlugWrapper::SigStatus_UNKNOWN == statusFlags ) {
                result = i18n("No status information available.");
                frameColor = SIG_FRAME_COL_YELLOW;
                showKeyInfos = false;
                return result;
            }

            if( CryptPlugWrapper::SigStatus_VALID & statusFlags ) {
                result = i18n("GOOD signature!");
                // Note:
                // Here we are work differently than KMail did before!
                //
                // The GOOD case ( == sig matching and the complete
                // certificate chain was verified and is valid today )
                // by definition does *not* show any key
                // information but just states that things are OK.
                //           (khz, according to LinuxTag 2002 meeting)
                frameColor = SIG_FRAME_COL_GREEN;
                showKeyInfos = false;
                return result;
            }

            // we are still there?  OK, let's test the different cases:

            // we assume green, test for yellow or red (in this order!)
            frameColor = SIG_FRAME_COL_GREEN;
            QString result2;
            if( CryptPlugWrapper::SigStatus_KEY_EXPIRED & statusFlags ){
                // still is green!
                result2 += i18n("One key has expired.");
            }
            if( CryptPlugWrapper::SigStatus_SIG_EXPIRED & statusFlags ){
                // and still is green!
                result2 += i18n("The signature has expired.");
            }

            // test for yellow:
            if( CryptPlugWrapper::SigStatus_KEY_MISSING & statusFlags ) {
                result2 += i18n("Unable to verify: key missing.");
                // if the signature certificate is missing
                // we cannot show infos on it
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_CRL_MISSING & statusFlags ){
                result2 += i18n("CRL not available.");
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_CRL_TOO_OLD & statusFlags ){
                result2 += i18n("Available CRL is too old.");
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_BAD_POLICY & statusFlags ){
                result2 += i18n("A policy was not met.");
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_SYS_ERROR & statusFlags ){
                result2 += i18n("A system error occurred.");
                // if a system error occured
                // we cannot trust any information
                // that was given back by the plug-in
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_NUMERICAL_CODE & statusFlags ) {
                result2 += i18n("Internal system error #%1 occurred.")
                        .arg( statusFlags - CryptPlugWrapper::SigStatus_NUMERICAL_CODE );
                // if an unsupported internal error occured
                // we cannot trust any information
                // that was given back by the plug-in
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }

            // test for red:
            if( CryptPlugWrapper::SigStatus_KEY_REVOKED & statusFlags ){
                // this is red!
                result2 += i18n("One key has been revoked.");
                frameColor = SIG_FRAME_COL_RED;
            }
            if( CryptPlugWrapper::SigStatus_RED & statusFlags ) {
                if( result2.isEmpty() )
                    // Note:
                    // Here we are work differently than KMail did before!
                    //
                    // The BAD case ( == sig *not* matching )
                    // by definition does *not* show any key
                    // information but just states that things are BAD.
                    //
                    // The reason for this: In this case ALL information
                    // might be falsificated, we can NOT trust the data
                    // in the body NOT the signature - so we don't show
                    // any key/signature information at all!
                    //         (khz, according to LinuxTag 2002 meeting)
                    showKeyInfos = false;
                frameColor = SIG_FRAME_COL_RED;
            }
            else
                result = "";

            if( SIG_FRAME_COL_GREEN == frameColor ) {
                if( result2.isEmpty() )
                    result = i18n("GOOD signature!");
                else
                    result = i18n("Good signature.");
            } else if( SIG_FRAME_COL_RED == frameColor ) {
                if( result2.isEmpty() )
                    result = i18n("BAD signature!");
                else
                    result = i18n("Bad signature.");
            } else
                result = "";

            if( !result2.isEmpty() ) {
                if( !result.isEmpty() )
                    result.append("<br />");
                result.append( result2 );
            }
        }
        /*
        // add i18n support for 3rd party plug-ins here:
        else if (0 <= cryptPlug->libName().find( "yetanotherpluginname", 0, false )) {

        }
        */
    }
    return result;
}

//---------------------------------------------------

QString KMReaderWin::writeSigstatHeader( PartMetaData& block,
                                         CryptPlugWrapper* cryptPlug,
                                         const QString& fromAddress )
{
    bool isSMIME = cryptPlug && (0 <= cryptPlug->libName().find( "smime",   0, false ));
    QString signer = block.signer;

    QString htmlStr;
    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
    QString cellPadding("cellpadding=\"1\"");

    if( block.isEncapsulatedRfc822Message )
    {
        htmlStr += "<table cellspacing=\"1\" "+cellPadding+" class=\"rfc822\">"
            "<tr class=\"rfc822H\"><td dir=\"" + dir + "\">";
        htmlStr += i18n("Encapsulated message");
        htmlStr += "</td></tr><tr class=\"rfc822B\"><td>";
    }

    if( block.isEncrypted )
    {
        htmlStr += "<table cellspacing=\"1\" "+cellPadding+" class=\"encr\">"
            "<tr class=\"encrH\"><td dir=\"" + dir + "\">";
        if( block.isDecryptable )
            htmlStr += i18n("Encrypted message");
        else {
            htmlStr +=
                QString("%1<br />%2 <i>%3</i>")
                .arg( i18n("Cannot decrypt message.") )
                .arg( block.errorText.isEmpty() ? QString("") : i18n("Error: ") )
                .arg( block.errorText );
        }
        htmlStr += "</td></tr><tr class=\"encrB\"><td>";
    }

    if (block.isSigned) {
        QStringList& blockAddrs( block.signerMailAddresses );
        // note: At the moment frameColor and showKeyInfos are
        //       used for CMS only but not for PGP signatures
        // pending(khz): Implement usage of these for PGP sigs as well.
        int frameColor = SIG_FRAME_COL_UNDEF;
        bool showKeyInfos;
        bool onlyShowKeyURL = false;
        bool cannotCheckSignature = true;
        QString statusStr = sigStatusToString( cryptPlug,
                                               block.status_code,
                                               block.sigStatusFlags,
                                               frameColor,
                                               showKeyInfos );
        // if needed fallback to english status text
        // that was reported by the plugin
        if( statusStr.isEmpty() )
            statusStr = block.status;

        switch( frameColor ){
            case SIG_FRAME_COL_RED:
                cannotCheckSignature = false;
                break;
            case SIG_FRAME_COL_YELLOW:
                cannotCheckSignature = true;
                break;
            case SIG_FRAME_COL_GREEN:
                cannotCheckSignature = false;
                break;
        }

        // compose the string for displaying the key ID
        // either as URL or not linked (for PGP)
        // note: Once we can start PGP key manager programs
        //       from within KMail we could change this and
        //       allways show the URL.    (khz, 2002/06/27)
        QString startKeyHREF;
        if( isSMIME )
            startKeyHREF =
                QString("<a href=\"kmail:showCertificate#%1 ### %2 ### %3\">")
                .arg( cryptPlug->displayName() )
                .arg( cryptPlug->libName() )
                .arg( block.keyId );
        QString keyWithWithoutURL
            = isSMIME
            ? QString("%1%2</a>")
                .arg( startKeyHREF )
                .arg( cannotCheckSignature ? i18n("[Details]") : ("0x" + block.keyId) )
            : "0x" + QString::fromUtf8( block.keyId );


        // temporary hack: allways show key infos!
        showKeyInfos = true;

        // Sorry for using 'black' as null color but .isValid()
        // checking with QColor default c'tor did not work for
        // some reason.
        if( isSMIME && (SIG_FRAME_COL_UNDEF != frameColor) ) {

            // new frame settings for CMS:
            // beautify the status string
            if( !statusStr.isEmpty() ) {
                statusStr.prepend("<i>");
                statusStr.append( "</i>");
            }

            // special color handling: S/MIME uses only green/yellow/red.
            switch( frameColor ){
                case SIG_FRAME_COL_RED:
                    block.signClass = "signErr";//"signCMSRed";
                    onlyShowKeyURL = true;
                    break;
                case SIG_FRAME_COL_YELLOW:
                    block.signClass = "signOkKeyBad";//"signCMSYellow";
                    break;
                case SIG_FRAME_COL_GREEN: {
                        block.signClass = "signOkKeyOk";//"signCMSGreen";
                        // extra hint for green case
                        // that email addresses in DN do not match fromAddress
                        QString greenCaseWarning;
                        QString msgFrom( KMMessage::getEmailAddr(fromAddress) );
                        QString certificate;
                        if( block.keyId.isEmpty() )
                            certificate = "certificate";
                        else
                            certificate = QString("%1%2</a>")
                                          .arg( startKeyHREF )
                                          .arg( "certificate" );
                        if( blockAddrs.count() ){
                            if( blockAddrs.grep(
                                    msgFrom,
                                    false ).isEmpty() ) {
                                greenCaseWarning =
                                    "<u>" +
                                    i18n("Warning:") +
                                    "</u> " +
                                    i18n("Sender's mail address is not stored "
                                         "in the %1 used for signing.").arg(certificate) +
                                    "<br />" +
                                    i18n("sender: ") +
                                    "&lt;" +
                                    msgFrom +
                                    "&gt;<br />" +
                                    i18n("stored: ") +
                                    "&lt;";
                                // We cannot use Qt's join() function here but
                                // have to join the addresses manually to
                                // extract the mail addresses (without '<''>')
                                // before including it into our string:
                                bool bStart = true;
                                for(QStringList::ConstIterator it = blockAddrs.begin();
                                    it != blockAddrs.end(); ++it ){
                                    if( !bStart )
                                        greenCaseWarning.append("&gt;, <br />&nbsp; &nbsp;&lt;");
                                    bStart = false;
                                    greenCaseWarning.append( KMMessage::getEmailAddr(*it) );
                                }
                                greenCaseWarning.append( "&gt;" );
                            }
                        } else {
                            greenCaseWarning =
                                "<u>" +
                                i18n("Warning:") +
                                "</u> " +
                                i18n("No mail address is stored in the %1 used for signing, "
                                     "so we cannot compare it to the sender's address &lt;%2&gt;.")
                                .arg(certificate)
                                .arg(msgFrom);
                        }
                        if( !greenCaseWarning.isEmpty() ) {
                            if( !statusStr.isEmpty() )
                                statusStr.append("<br />&nbsp;<br />");
                            statusStr.append( greenCaseWarning );
                        }
                    }
                    break;
            }

            htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                "class=\"" + block.signClass + "\">"
                "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
            if( showKeyInfos ) {

                if( cannotCheckSignature ) {
                    htmlStr += i18n( "Not enough information to check "
                                     "signature. %1" )
                                .arg( keyWithWithoutURL );
                }
                else {

                    if (block.signer.isEmpty())
                        signer = "";
                    else {
                        // HTMLize the signer's user id and try to create mailto: link
                        signer = KMMessage::emailAddrAsAnchor( signer );
                        if( blockAddrs.count() ){
                            QString address = KMMessage::encodeMailtoUrl( blockAddrs.first() );
                            signer = "<a href=\"mailto:" + address + "\">" + signer + "</a>";
                        }
                    }

                    if( block.keyId.isEmpty() ) {
                        if( signer.isEmpty() || onlyShowKeyURL )
                            htmlStr += i18n( "Message was signed with unknown key." );
                        else
                            htmlStr += i18n( "Message was signed by %1." )
                                    .arg( signer );
                    } else {
                        bool dateOK = (0 < block.creationTime.tm_year);
                        QDate created( 1900 + block.creationTime.tm_year,
                                    block.creationTime.tm_mon,
                                    block.creationTime.tm_mday );
                        if( dateOK && created.isValid() ) {
                            if( signer.isEmpty() ) {
                                if( onlyShowKeyURL )
                                    htmlStr += i18n( "Message was signed with key %1." )
                                                .arg( keyWithWithoutURL );
                                else
                                    htmlStr += i18n( "Message was signed with key %1, created %2." )
                                                .arg( keyWithWithoutURL ).arg( created.toString( Qt::LocalDate ) );
                            }
                            else {
                                if( onlyShowKeyURL )
                                    htmlStr += i18n( "Message was signed with key %1." )
                                            .arg( keyWithWithoutURL );
                                else
                                    htmlStr += i18n( "Message was signed by %3 with key %1, created %2." )
                                            .arg( keyWithWithoutURL )
                                            .arg( created.toString( Qt::LocalDate ) )
                                            .arg( signer );
                            }
                        }
                        else {
                            if( signer.isEmpty() || onlyShowKeyURL )
                                htmlStr += i18n( "Message was signed with key %1." )
                                        .arg( keyWithWithoutURL );
                            else
                                htmlStr += i18n( "Message was signed by %2 with key %1." )
                                        .arg( keyWithWithoutURL )
                                        .arg( signer );
                        }
                    }
                }
                htmlStr += "<br />";
                if( !statusStr.isEmpty() ) {
                    htmlStr += "&nbsp;<br />";
                    htmlStr += i18n( "Status: " );
                    htmlStr += statusStr;
                }
            } else {
                htmlStr += statusStr;
            }
            htmlStr += "</td></tr><tr class=\"" + block.signClass + "B\"><td>";

        } else {

            // old frame settings for PGP:

            if (block.signer.isEmpty()) {
                block.signClass = "signWarn";
                htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                    "class=\"" + block.signClass + "\">"
                    "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                if( !block.keyId.isEmpty() ) {
                    bool dateOK = (0 < block.creationTime.tm_year);
                    QDate created( 1900 + block.creationTime.tm_year,
                                  block.creationTime.tm_mon,
                                  block.creationTime.tm_mday );
                    if( dateOK && created.isValid() )
                        htmlStr += i18n( "Message was signed with unknown key %1, created %2." )
                                .arg( keyWithWithoutURL ).arg( created.toString( Qt::LocalDate ) );
                    else
                        htmlStr += i18n( "Message was signed with unknown key %1." )
                                .arg( keyWithWithoutURL );
                }
                else
                    htmlStr += i18n( "Message was signed with unknown key." );
                htmlStr += "<br />";
                htmlStr += i18n( "The validity of the signature cannot be "
                        "verified." );
                if( !statusStr.isEmpty() ) {
                    htmlStr += "<br />";
                    htmlStr += i18n( "Status: " );
                    htmlStr += "<i>";
                    htmlStr += statusStr;
                    htmlStr += "</i>";
                }
                htmlStr += "</td></tr><tr class=\"" + block.signClass + "B\"><td>";
            }
            else
            {
                // HTMLize the signer's user id and create mailto: link
                signer = KMMessage::quoteHtmlChars( signer, true );
                signer = "<a href=\"mailto:" + signer + "\">" + signer + "</a>";

                if (block.isGoodSignature) {
                    if( block.keyTrust < Kpgp::KPGP_VALIDITY_MARGINAL )
                        block.signClass = "signOkKeyBad";
                    else
                        block.signClass = "signOkKeyOk";
                    htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                        "class=\"" + block.signClass + "\">"
                        "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                    if( !block.keyId.isEmpty() )
                        htmlStr += i18n( "Message was signed by %2 (Key ID: %1)." )
                                   .arg( keyWithWithoutURL )
                                   .arg( signer );
                    else
                        htmlStr += i18n( "Message was signed by %1." ).arg( signer );
                    htmlStr += "<br />";

                    switch( block.keyTrust )
                    {
                        case Kpgp::KPGP_VALIDITY_UNKNOWN:
                        htmlStr += i18n( "The signature is valid, but the key's "
                                "validity is unknown." );
                        break;
                        case Kpgp::KPGP_VALIDITY_MARGINAL:
                        htmlStr += i18n( "The signature is valid and the key is "
                                "marginally trusted." );
                        break;
                        case Kpgp::KPGP_VALIDITY_FULL:
                        htmlStr += i18n( "The signature is valid and the key is "
                                "fully trusted." );
                        break;
                        case Kpgp::KPGP_VALIDITY_ULTIMATE:
                        htmlStr += i18n( "The signature is valid and the key is "
                                "ultimately trusted." );
                        break;
                        default:
                        htmlStr += i18n( "The signature is valid, but the key is "
                                "untrusted." );
                    }
                    htmlStr += "</td></tr>"
                        "<tr class=\"" + block.signClass + "B\"><td>";
                }
                else
                {
                    block.signClass = "signErr";
                    htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                        "class=\"" + block.signClass + "\">"
                        "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                    if( !block.keyId.isEmpty() )
                        htmlStr += i18n( "Message was signed by %2 (Key ID: %1)." )
                        .arg( keyWithWithoutURL )
                        .arg( signer );
                    else
                        htmlStr += i18n( "Message was signed by %1." ).arg( signer );
                    htmlStr += "<br />";
                    htmlStr += i18n("Warning: The signature is bad.");
                    htmlStr += "</td></tr>"
                        "<tr class=\"" + block.signClass + "B\"><td>";
                }
            }
        }
    }

    return htmlStr;
}

QString KMReaderWin::writeSigstatFooter( PartMetaData& block )
{
    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );

    QString htmlStr;

    if (block.isSigned) {
	htmlStr += "</td></tr><tr class=\"" + block.signClass + "H\">";
	htmlStr += "<td dir=\"" + dir + "\">" +
	    i18n( "End of signed message" ) +
	    "</td></tr></table>";
    }

    if (block.isEncrypted) {
	htmlStr += "</td></tr><tr class=\"encrH\"><td dir=\"" + dir + "\">" +
		i18n( "End of encrypted message" ) +
	    "</td></tr></table>";
    }

    if( block.isEncapsulatedRfc822Message )
    {
        htmlStr += "</td></tr><tr class=\"rfc822H\"><td dir=\"" + dir + "\">" +
            i18n( "End of encapsulated message" ) +
            "</td></tr></table>";
    }

    return htmlStr;
}

//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr( const QCString& aStr, const QTextCodec *aCodec,
                                const QString& fromAddress )
{
  KMMsgSignatureState dummy1;
  KMMsgEncryptionState dummy2;
  writeBodyStr( aStr, aCodec, fromAddress, dummy1, dummy2 );
}

//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr( const QCString& aStr, const QTextCodec *aCodec,
                                const QString& fromAddress,
                                KMMsgSignatureState&  inlineSignatureState,
                                KMMsgEncryptionState& inlineEncryptionState )
{
  bool goodSignature = false;
  Kpgp::Module* pgp = Kpgp::Module::getKpgp();
  assert(pgp != 0);
  bool isPgpMessage = false; // true if the message contains at least one
                             // PGP MESSAGE or one PGP SIGNED MESSAGE block
  QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
  QString headerStr = QString("<div dir=\"%1\">").arg(dir);

  inlineSignatureState  = KMMsgNotSigned;
  inlineEncryptionState = KMMsgNotEncrypted;
  QPtrList<Kpgp::Block> pgpBlocks;
  QStrList nonPgpBlocks;
  if( Kpgp::Module::prepareMessageForDecryption( aStr, pgpBlocks, nonPgpBlocks ) )
  {
      bool isEncrypted = false, isSigned = false;
      bool fullySignedOrEncrypted = true;
      bool couldDecrypt = false;
      QString signer;
      QCString keyId;
      QString decryptionError;
      Kpgp::Validity keyTrust = Kpgp::KPGP_VALIDITY_FULL;

      QPtrListIterator<Kpgp::Block> pbit( pgpBlocks );

      QStrListIterator npbit( nonPgpBlocks );

      QString htmlStr;
      for( ; *pbit != 0; ++pbit, ++npbit )
      {
	  // insert the next Non-OpenPGP block
	  QCString str( *npbit );
	  if( !str.isEmpty() ) {
	    htmlStr += quotedHTML( aCodec->toUnicode( str ) );
            kdDebug( 5006 ) << "Non-empty Non-OpenPGP block found: '" << str
                            << "'" << endl;
            fullySignedOrEncrypted = false;
          }

	  //htmlStr += "<br>";

	  Kpgp::Block* block = *pbit;
	  if( ( block->type() == Kpgp::PgpMessageBlock ) ||
	      ( block->type() == Kpgp::ClearsignedBlock ) )
	  {
	      isPgpMessage = true;
	      if( block->type() == Kpgp::PgpMessageBlock )
	      {
		  emit noDrag();
		  // try to decrypt this OpenPGP block
		  couldDecrypt = block->decrypt();
		  isEncrypted = block->isEncrypted();
		  if (!couldDecrypt) {
		    decryptionError = pgp->lastErrorMsg();
		  }
	      }
	      else
	      {
		  // try to verify this OpenPGP block
		  block->verify();
	      }

	      isSigned = block->isSigned();
	      if( isSigned )
	      {
                  keyId = block->signatureKeyId();
		  signer = block->signatureUserId();
		  if( !signer.isEmpty() )
		  {
		      goodSignature = block->goodSignature();

		      if( !keyId.isEmpty() )
			keyTrust = pgp->keyTrust( keyId );
		      else
			// This is needed for the PGP 6 support because PGP 6 doesn't
			// print the key id of the signing key if the key is known.
			keyTrust = pgp->keyTrust( signer );
		  }
	      }

              if( isSigned )
                inlineSignatureState = KMMsgPartiallySigned;
	      if( isEncrypted )
                inlineEncryptionState = KMMsgPartiallyEncrypted;

	      PartMetaData messagePart;

	      messagePart.isSigned = isSigned;
	      messagePart.isGoodSignature = goodSignature;
	      messagePart.isEncrypted = isEncrypted;
	      messagePart.isDecryptable = couldDecrypt;
	      messagePart.decryptionError = decryptionError;
	      messagePart.signer = signer;
	      messagePart.keyId = keyId;
	      messagePart.keyTrust = keyTrust;

	      htmlStr += writeSigstatHeader( messagePart, 0, fromAddress );

	      htmlStr += quotedHTML( aCodec->toUnicode( block->text() ) );
	      htmlStr += writeSigstatFooter( messagePart );
	  }
	  else // block is neither message block nor clearsigned block
	    htmlStr += quotedHTML( aCodec->toUnicode( block->text() ) );
      }

      // add the last Non-OpenPGP block
      QCString str( nonPgpBlocks.last() );
      if( !str.isEmpty() ) {
        htmlStr += quotedHTML( aCodec->toUnicode( str ) );
        // Even if the trailing Non-OpenPGP block isn't empty we still
        // consider the message part fully signed/encrypted because else
        // all inline signed mailing list messages would only be partially
        // signed because of the footer which is often added by the mailing
        // list software. IK, 2003-02-15
      }
      if( fullySignedOrEncrypted ) {
        if( inlineSignatureState == KMMsgPartiallySigned )
          inlineSignatureState = KMMsgFullySigned;
        if( inlineEncryptionState == KMMsgPartiallyEncrypted )
          inlineEncryptionState = KMMsgFullyEncrypted;
      }
      htmlWriter()->queue( htmlStr );
  }
  else
    htmlWriter()->queue( quotedHTML( aCodec->toUnicode( aStr ) ) );
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeHTMLStr(const QString& aStr)
{
  mColorBar->setEraseColor( cCBisHtmlB );
  mColorBar->setPaletteForegroundColor( cCBisHtmlF );
  mColorBar->setTextFormat( Qt::RichText );
  mColorBar->setText(i18n("<b><br>H<br>T<br>M<br>L<br> <br>M<br>e<br>s<br>s<br>a<br>g<br>e</b>"));
  htmlWriter()->queue(aStr);
}

//-----------------------------------------------------------------------------

QString KMReaderWin::quotedHTML(const QString& s)
{
  QString htmlStr;
  QString normalStartTag;
  const QString normalEndTag = "</div>";
  const QString quoteEnd = "</div>";

  unsigned int pos, beg;
  unsigned int length = s.length();

  QString style;
  if( mBodyFont.bold() )
    style += "font-weight:bold;";
  if( mBodyFont.italic() )
    style += "font-style:italic;";
  if( style.isEmpty() )
    normalStartTag = "<div>";
  else
    normalStartTag = QString("<div style=\"%1\">").arg( style );

  // skip leading empty lines
  for( pos = 0; pos < length && s[pos] <= ' '; pos++ );
  while (pos > 0 && (s[pos-1] == ' ' || s[pos-1] == '\t')) pos--;
  beg = pos;

  int currQuoteLevel = -2; // -2 == no previous lines

  while (beg<length)
  {
    QString line;

    /* search next occurance of '\n' */
    pos = s.find('\n', beg, FALSE);
    if (pos == (unsigned int)(-1))
	pos = length;

    line = s.mid(beg,pos-beg);
    beg = pos+1;

    /* calculate line's current quoting depth */
    int actQuoteLevel = -1;
    for (unsigned int p=0; p<line.length(); p++) {
      switch (line[p].latin1()) {
        case '>':
        case '|':
          actQuoteLevel++;
          break;
        case ' ':  // spaces and tabs are allowed between the quote markers
        case '\t':
        case '\r':
          break;
        default:  // stop quoting depth calculation
          p = line.length();
          break;
      }
    } /* for() */

    if ( actQuoteLevel != currQuoteLevel ) {
      /* finish last quotelevel */
      if (currQuoteLevel == -1)
        htmlStr.append( normalEndTag );
      else if (currQuoteLevel >= 0)
        htmlStr.append( quoteEnd );

      /* start new quotelevel */
      currQuoteLevel = actQuoteLevel;
      if (actQuoteLevel == -1)
        htmlStr += normalStartTag;
      else
        htmlStr += mQuoteFontTag[currQuoteLevel%3];
    }

    // don't write empty <div ...></div> blocks (they have zero height)
    if( !line.isEmpty() )
    {
      if( line.isRightToLeft() )
        htmlStr += QString( "<div dir=\"rtl\">" );
      else
        htmlStr += QString( "<div dir=\"ltr\">" );
      htmlStr += strToHtml( line, true );
      htmlStr += QString( "</div>" );
    }
    else
      htmlStr += "<br>";
  } /* while() */

  /* really finish the last quotelevel */
  if (currQuoteLevel == -1)
     htmlStr.append( normalEndTag );
  else
     htmlStr.append( quoteEnd );

  //kdDebug(5006) << "KMReaderWin::quotedHTML:\n"
  //              << "========================================\n"
  //              << htmlStr
  //              << "\n======================================\n";
  return htmlStr;
}


//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum,
  bool quiet)
{
  QString iconName, href, label, comment, contDisp;
  QString fileName;

  if(aMsgPart == 0) {
    kdDebug(5006) << "writePartIcon: aMsgPart == 0\n" << endl;
    return;
  }

  kdDebug(5006) << "writePartIcon: PartNum: " << aPartNum << endl;

  comment = aMsgPart->contentDescription();
  comment = KMMessage::quoteHtmlChars( comment, true );

  fileName = aMsgPart->fileName();
  if (fileName.isEmpty()) fileName = aMsgPart->name();
  label = KMMessage::quoteHtmlChars( fileName, true );

//--- Sven's save attachments to /tmp start ---
  KTempFile *tempFile = new KTempFile(QString::null,
    "." + QString::number(aPartNum));
  tempFile->setAutoDelete(true);
  QString fname = tempFile->name();
  delete tempFile;

  bool ok = true;

  if (access(QFile::encodeName(fname), W_OK) != 0) // Not there or not writable
    if (mkdir(QFile::encodeName(fname), 0) != 0
      || chmod (QFile::encodeName(fname), S_IRWXU) != 0)
        ok = false; //failed create

  if (ok)
  {
    mTempDirs.append(fname);
    //fileName.replace(QRegExp("[/\"\']"),"");
    // strip off a leading path
    int slashPos = fileName.findRev( '/' );
    if( -1 != slashPos )
      fileName = fileName.mid( slashPos + 1 );
    if (fileName.isEmpty()) fileName = "unnamed";
    fname += "/" + fileName;

    if (!kByteArrayToFile(aMsgPart->bodyDecodedBinary(), fname, false, false, false))
      ok = false;
    mTempFiles.append(fname);
  }
  if (ok)
  {
    href = QString("file:")+KURL::encode_string(fname);
    //debug ("Wrote attachment to %s", href.data());
  }
  else {
    //--- Sven's save attachments to /tmp end ---
    href = QString("part://%1").arg(aPartNum+1);
  }

  // sven: for viewing images inline
  if (mInlineImage)
    iconName = href;
  else
    iconName = aMsgPart->iconName();
  if (iconName.right(14)=="mime_empty.png")
  {
    aMsgPart->magicSetType();
    iconName = aMsgPart->iconName();
  }
  if (!quiet)
    htmlWriter()->queue("<table><tr><td><a href=\"" + href + "\"><img src=\"" +
                   iconName + "\" border=\"0\">" + label +
                   "</a></td></tr></table>" + comment + "<br>");
}


//-----------------------------------------------------------------------------
QString KMReaderWin::strToHtml(const QString &aStr, bool aPreserveBlanks) const
{
  return LinkLocator::convertToHtml(aStr, aPreserveBlanks);
}


//-----------------------------------------------------------------------------
void KMReaderWin::printMsg()
{
  if (!message()) return;
    mViewer->view()->print();
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const KURL &aUrl)
{
  if (aUrl.isEmpty() || !message()) return -1;

  if (!aUrl.isLocalFile()) return -1;

  QString path = aUrl.path();
  uint right = path.findRev('/');
  uint left = path.findRev('.', right);

  bool ok;
  int res = path.mid(left + 1, right - left - 1).toInt(&ok);
  return (ok) ? res : -1;
}


//-----------------------------------------------------------------------------
void KMReaderWin::resizeEvent(QResizeEvent *)
{
  if( !mResizeTimer.isActive() )
  {
    //
    // Combine all resize operations that are requested as long a
    // the timer runs.
    //
    mResizeTimer.start( 100, true );
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDelayedResize()
{
//  mViewer->widget()->setGeometry(0, 0, width(), height());
  mBox->setGeometry(0, 0, width(), height());
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotTouchMessage()
{
  if (message())
  {
    KMMsgStatus st = message()->status();
    if (st == KMMsgStatusNew || st == KMMsgStatusUnread
        || st == KMMsgStatusRead)
      message()->setStatus(KMMsgStatusOld);
    if ( st == KMMsgStatusNew || st == KMMsgStatusUnread ) {
      KMMessage * receipt = message()->createMDN( MDN::ManualAction,
						  MDN::Displayed,
						  true /* allow GUI */ );
      if ( receipt )
	if ( !kernel->msgSender()->send( receipt ) ) // send or queue
	  KMessageBox::error( this, i18n("Couldn't send MDN!") );
    }
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::closeEvent(QCloseEvent *e)
{
  KMReaderWinInherited::closeEvent(e);
  writeConfig();
}


bool foundSMIMEData( const QString aUrl,
                     QString& displayName,
                     QString& libName,
                     QString& keyId )
{
  static QString showCertMan("showCertificate#");
  displayName = "";
  libName = "";
  keyId = "";
  int i1 = aUrl.find( showCertMan );
  if( -1 < i1 ) {
    i1 += showCertMan.length();
    int i2 = aUrl.find(" ### ", i1);
    if( i1 < i2 )
    {
      displayName = aUrl.mid( i1, i2-i1 );
      i1 = i2+5;
      i2 = aUrl.find(" ### ", i1);
      if( i1 < i2 )
      {
        libName = aUrl.mid( i1, i2-i1 );
        i2 += 5;

        keyId = aUrl.mid( i2 );
        /*
        int len = aUrl.length();
        if( len > i2+1 ) {
          keyId = aUrl.mid( i2, 2 );
          i2 += 2;
          while( len > i2+1 ) {
            keyId += ':';
            keyId += aUrl.mid( i2, 2 );
            i2 += 2;
          }
        }
        */
      }
    }
  }
  return !keyId.isEmpty();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOn(const QString &aUrl)
{
  bool bOk = false;

  QString dummyStr;
  QString keyId;
  QString gwType;
  QString gwAction;
  QString gwAction2;

  KURL url(aUrl);
  mUrlClicked = url;
  int id = msgPartFromUrl(url);

  if (id > 0)
  {
    partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
    if( node ) {
      KMMessagePart& msgPart = node->msgPart();
      QString str = msgPart.fileName();
      if (str.isEmpty()) str = msgPart.name();
      emit statusMsg(i18n("Attachment: ") + str);
      bOk = true;
    }
  }
  else if( aUrl == "kmail:showHTML" )
  {
    emit statusMsg( i18n("Turn on HTML rendering for this message.") );
    bOk = true;
  }
  else if( foundSMIMEData( aUrl, dummyStr, dummyStr, keyId ) )
  {
    emit statusMsg(i18n("Show certificate 0x%1").arg(keyId));
    bOk = true;
  }
  else if( KMGroupware::foundGroupwareLink( aUrl,
                                            gwType,
                                            gwAction,
                                            gwAction2,
                                            dummyStr ) )
  {
    dummyStr = gwType+" "+gwAction;
    if( !gwAction2.isEmpty() )
      dummyStr.append(" "+gwAction2);
    emit statusMsg(i18n("Groupware:\"%1\"").arg(dummyStr));
    bOk = true;
  }
  else if( aUrl.startsWith( "mailto:" ) )
  {
    emit statusMsg( KMMessage::decodeMailtoUrl( aUrl ) );
    bOk = true;
  }

  if( !bOk )
    emit statusMsg(aUrl);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KURL &aUrl, const KParts::URLArgs &)
{
  QString displayName;
  QString libName;
  QString keyId;
  mUrlClicked = aUrl;
  if( aUrl.hasRef() && foundSMIMEData( aUrl.path()+"#"+aUrl.ref(), displayName, libName, keyId ) )
  {
    QString query( "-query " );
    query += keyId;
    KProcess certManagerProc; // save to create on the heap, since
                            // there is no parent
    certManagerProc << "kgpgcertmanager";
    certManagerProc << displayName;
    certManagerProc << libName;
    certManagerProc << "-query";
    certManagerProc << keyId;
    if( !certManagerProc.start( KProcess::DontCare ) )
      KMessageBox::error( this, i18n( "Could not start certificate manager. Please check your installation!" ),
                          i18n( "KMail Error" ) );
    else
      kdDebug(5006) << "\nKMReaderWin::slotUrlOn(): certificate manager started.\n" << endl;
    // process continues to run even after the KProcess object goes
    // out of scope here, since it is started in DontCare run mode.
    return;
  }

  if( aUrl.hasRef() )
    kdDebug(5006) << QString(aUrl.path()+"#"+aUrl.ref()) << endl;

  if( mUseGroupware && kernel->groupware().handleLink( aUrl, message() ) )
    return;

  // handle own links
  if( aUrl.protocol() == "kmail" )
  {
    if( aUrl.path() == "showHTML" )
    {
      setHtmlOverride(!mHtmlOverride);
      update( true );
      return;
    }
  }

  if (!aUrl.hasHost() && aUrl.path() == "/" && aUrl.hasRef())
  {
    if (!mViewer->gotoAnchor(aUrl.ref()))
      static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);
    return;
  }
  int id = msgPartFromUrl(aUrl);
  if (id > 0)
  {
    // clicked onto an attachment
    mAtmCurrent = id;
    mAtmCurrentName = aUrl.path();
    slotAtmOpen();
  }
  else {
//      if (aUrl.protocol().isEmpty() || (aUrl.protocol() == "file"))
//	  return;
      mUrlClicked = aUrl;
      emit urlClicked(aUrl,/* aButton*/LeftButton); //### FIXME: add button to URLArgs!
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  if (!message()) return;
  KURL url( aUrl );
  mUrlClicked = url;

  int id = msgPartFromUrl(url);
  if (id <= 0)
  {
    emit popupMenu(*message(), url, aPos);
  }
  else
  {
    // Attachment popup
    mAtmCurrent = id;
    mAtmCurrentName = url.path();
    KPopupMenu *menu = new KPopupMenu();
    menu->insertItem(i18n("Open..."), this, SLOT(slotAtmOpen()));
    menu->insertItem(i18n("Open With..."), this, SLOT(slotAtmOpenWith()));
    menu->insertItem(i18n("View..."), this, SLOT(slotAtmView()));
    menu->insertItem(i18n("Save As..."), this, SLOT(slotAtmSave()));
    menu->insertItem(i18n("Properties..."), this,
		     SLOT(slotAtmProperties()));
    menu->exec(aPos,0);
    delete menu;
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotFind()
{
  //dnaber:
  KAction *act = mViewer->actionCollection()->action("find");
  if( act )
    act->activate();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotToggleFixedFont()
{
  mUseFixedFont = !mUseFixedFont;
  mViewer->setStandardFont( bodyFontFamily() );
  update(true);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotCopySelectedText()
{
  kapp->clipboard()->setText( mViewer->selectedText() );
}


//-----------------------------------------------------------------------------
void KMReaderWin::atmViewMsg(KMMessagePart* aMsgPart)
{
  KMMessage* msg = new KMMessage;
  assert(aMsgPart!=0);
  msg->fromString(aMsgPart->bodyDecoded());
  assert(msg != 0);
  KMReaderMainWin *win = new KMReaderMainWin();
  win->showMsg( mCodec, msg );
  win->resize(550,600);
  win->show();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMsgPart( KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QTextCodec *aCodec )
{
  QString str;
  kernel->kbp()->busy();
  if (qstricmp(aMsgPart->typeStr(), "message")==0) {
      // if called from compose win
      KMMessage* msg = new KMMessage;
      assert(aMsgPart!=0);
      msg->fromString(aMsgPart->bodyDecoded());
      mMainWindow->setCaption(msg->subject());
      setMsg(msg, true);
      setAutoDelete(true);
  } else if (qstricmp(aMsgPart->typeStr(), "text")==0) {
      if (qstricmp(aMsgPart->subtypeStr(), "x-vcard") == 0) {
	KMDisplayVCard *vcdlg;
	int vcerr;
	VCard *vc = VCard::parseVCard(aCodec->toUnicode(
 	aMsgPart->bodyDecoded()), &vcerr);
	if (!vc) {
          QString errstring = i18n("Error reading in vCard:\n");
	  errstring += VCard::getError(vcerr);
          kernel->kbp()->idle();
	  KMessageBox::error(0, errstring, i18n("vCard error"));
	  kernel->kbp()->idle();
	  return;
	}

	vcdlg = new KMDisplayVCard(vc);
        kernel->kbp()->idle();
	vcdlg->show();
	return;
      }
      if ( aCodec )
	setCodec( aCodec );
      else
	setCodec( KGlobal::charsets()->codecForName( "iso8859-1" ) );
      htmlWriter()->begin();
      htmlWriter()->queue("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
		"Transitional//EN\">\n<html><head><title></title>"
		"<style type=\"text/css\">" +
		QString("a { color: %1;").arg(c2.name()) +
		"text-decoration: none; }" + // just playing
		"</style></head><body " +
		QString(" text=\"%1\"").arg(c1.name()) +
		QString(" bgcolor=\"%1\"").arg(c4.name()) +
		">" );
      QCString str = aMsgPart->bodyDecoded();
      if (aHTML && (qstricmp(aMsgPart->subtypeStr(), "html")==0))  // HTML
	  writeHTMLStr(codec()->toUnicode(str));
      else // plain text
	  writeBodyStr( str,
		    codec(),
		    message() ? message()->from() : QString("") );
      htmlWriter()->queue("</body></html>");
      htmlWriter()->flush();
      mMainWindow->setCaption(i18n("View Attachment: %1").arg(pname));
  } else if (qstricmp(aMsgPart->typeStr(), "image")==0 ||
             (qstricmp(aMsgPart->typeStr(), "application")==0 &&
              qstricmp(aMsgPart->subtypeStr(), "postscript")))
  {
      if (aFileName.isEmpty()) {
	  kernel->kbp()->idle();
	  return;  // prevent crash
      }
      if (aFileName.isEmpty()) return;  // prevent crash
      // Open the window with a size so the image fits in (if possible):
      QImageIO *iio = new QImageIO();
      iio->setFileName(aFileName);
      if( iio->read() ) {
          QImage img = iio->image();
          int scnum = QApplication::desktop()->screenNumber(mMainWindow);
          // determine a reasonable window size
          int width, height;
          if( img.width() < 50 )
              width = 70;
          else if( img.width()+20 < QApplication::desktop()->screen(scnum)->width() )
              width = img.width()+20;
          else
              width = QApplication::desktop()->screen(scnum)->width();
          if( img.height() < 50 )
              height = 70;
          else if( img.height()+20 < QApplication::desktop()->screen(scnum)->height() )
              height = img.height()+20;
          else
              height = QApplication::desktop()->screen(scnum)->height();
          mMainWindow->resize( width, height );
      }
      // Just write the img tag to HTML:
      htmlWriter()->begin();
      htmlWriter()->write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
		     "Transitional//EN\">\n<html><title></title><body>");
      QString linkName = QString("<img src=\"file:%1\" border=0>")
                         .arg(KURL::encode_string(aFileName));
      htmlWriter()->write(linkName);
      htmlWriter()->write("</body></html>");
      htmlWriter()->end();
      setCaption(i18n("View Attachment: ") + pname);
      show();
  } else {
      KMTextBrowser *browser = new KMTextBrowser(); // deletes itself
      QString str = aMsgPart->bodyDecoded();
      // A QString cannot handle binary data. So if it's shorter than the
      // attachment, we assume the attachment is binary:
      if( str.length() < (unsigned) aMsgPart->decodedSize() ) {
        str += i18n("\n[KMail: Attachment contains binary data. Trying to show first %1 characters.]").arg(str.length());
      }
      browser->setText(str);
      browser->resize(500, 550);
      browser->show();
  }
  // ---Sven's view text, html and image attachments in html widget end ---
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmView()
{
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();
    QString pname = msgPart.fileName();
    if (pname.isEmpty()) pname=msgPart.name();
    if (pname.isEmpty()) pname=msgPart.contentDescription();
    if (pname.isEmpty()) pname="unnamed";
    // image Attachment is saved already
    const QTextCodec *atmCodec = (mAutoDetectEncoding) ?
      KMMsgBase::codecForName(msgPart.charset()) : mCodec;
    if (!atmCodec) atmCodec = mCodec;
    if (qstricmp(msgPart.typeStr(), "message")==0) {
      atmViewMsg(&msgPart);
    } else if ((qstricmp(msgPart.typeStr(), "text")==0) && 
	       (qstricmp(msgPart.subtypeStr(), "x-vcard")==0)) {
      setMsgPart( &msgPart, htmlMail(), mAtmCurrentName, pname, atmCodec );
    } else {
      KMReaderMainWin *win = new KMReaderMainWin(&msgPart, htmlMail(),
	mAtmCurrentName, pname, atmCodec );
      win->show();
    }
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  QString str, pname, cmd, fileName;

  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( !node )
    return;

  KMMessagePart& msgPart = node->msgPart();

  if (qstricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  if (qstricmp(msgPart.typeStr(), "text") == 0)
  {
    if (qstricmp(msgPart.subtypeStr(), "x-vcard") == 0) {
      KMDisplayVCard *vcdlg;
      int vcerr;
      const QTextCodec *atmCodec = (mAutoDetectEncoding) ?
        KMMsgBase::codecForName(msgPart.charset()) : mCodec;
      if (!atmCodec) atmCodec = mCodec;
      VCard *vc = VCard::parseVCard(atmCodec->toUnicode(msgPart
        .bodyDecoded()), &vcerr);

      if (!vc) {
        QString errstring = i18n("Error reading in vCard:\n");
        errstring += VCard::getError(vcerr);
        KMessageBox::error(this, errstring, i18n("vCard error"));
        return;
      }

      vcdlg = new KMDisplayVCard(vc);
      vcdlg->show();
      return;
    }
  }

  // What to do when user clicks on an attachment --dnaber, 2000-06-01
  // TODO: show full path for Service, not only name
  QString mimetype = KMimeType::findByURL(KURL(KURL::encode_string(mAtmCurrentName)))->name();
  KService::Ptr offer = KServiceTypeProfile::preferredService(mimetype, "Application");
  QString question;
  QString open_text = i18n("&Open");
  QString filenameText = msgPart.fileName();
  if (filenameText.isEmpty()) filenameText = msgPart.name();
  if ( offer ) {
    question = i18n("Open attachment '%2' using '%1'?")
      .arg(offer->name()).arg(filenameText);
  } else {
    question = i18n("Open attachment '%1'?").arg(filenameText);
    open_text = i18n("&Open With...");
  }
  question += i18n("\n\nNote that opening an attachment may compromise your system's security!");
  // TODO: buttons don't have the correct order, but "Save" should be default
  int choice = KMessageBox::warningYesNoCancel(this, question,
      i18n("Open Attachment?"), KStdGuiItem::saveAs(), open_text);
  if( choice == KMessageBox::Yes ) {		// Save
    slotAtmSave();
  } else if( choice == KMessageBox::No ) {	// Open
    if ( offer ) {
      // There's a default service for this kind of file - use it
      KURL::List lst;
      KURL url;
      url.setPath(mAtmCurrentName);
      lst.append(url);
      KRun::run(*offer, lst);
    } else {
      // There's no know service that handles this type of file, so open
      // the "Open with..." dialog.
      KURL::List lst;
      KURL url;
      url.setPath(mAtmCurrentName);
      lst.append(url);
      KRun::displayOpenWithDialog(lst);
    }
  } else {					// Cancel
    kdDebug(5006) << "Canceled opening attachment" << endl;
  }

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpenWith()
{
  // It makes sense to have an extra "Open with..." entry in the menu
  // so the user can change filetype associations.

    KURL::List lst;
    KURL url;
    url.setPath(mAtmCurrentName);
    lst.append(url);
    KRun::displayOpenWithDialog(lst);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();

    QString fileName = mAtmCurrentName;

    // strip off the leading path
    int slashPos = fileName.findRev( '/' );
    if( -1 != slashPos )
      fileName = fileName.mid( slashPos + 1 );

    // replace all ':' with '_' because ':' isn't allowed on FAT volumes
    int colonPos = -1;
    while( -1 != ( colonPos = fileName.find(':', colonPos + 1) ) )
      fileName[colonPos] = '_';

    // prepend the previously used save dir
    fileName.prepend(mSaveAttachDir);
    KURL url = KFileDialog::getSaveURL( fileName, QString::null, this );

    if( url.isEmpty() )
      return;

    mSaveAttachDir = url.directory() + "/";

    kernel->byteArrayToRemoteFile(msgPart.bodyDecodedBinary(), url);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmProperties()
{
  KMMsgPartDialogCompat dlg(0,TRUE);

  kernel->kbp()->busy();
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();

    dlg.setMsgPart(&msgPart);
    kernel->kbp()->idle();

    dlg.exec();
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollUp()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, -10);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, 10);
}

bool KMReaderWin::atBottom() const
{
    const QScrollView *view = static_cast<const QScrollView *>(mViewer->widget());
    return view->contentsY() + view->visibleHeight() >= view->contentsHeight();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotJumpDown()
{
    QScrollView *view = static_cast<QScrollView *>(mViewer->widget());
    int offs = (view->clipper()->height() < 30) ? view->clipper()->height() : 30;
    view->scrollBy( 0, view->clipper()->height() - offs );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollPrior()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, -(int)(height()*0.8));
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollNext()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, (int)(height()*0.8));
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentChanged()
{

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotTextSelected(bool)
{
  QString temp = mViewer->selectedText();
  kapp->clipboard()->setText(temp);
}

//-----------------------------------------------------------------------------
void KMReaderWin::selectAll()
{
  mViewer->selectAll();
}

//-----------------------------------------------------------------------------
QString KMReaderWin::copyText()
{
  QString temp = mViewer->selectedText();
  return temp;
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentDone()
{
  // mSbVert->setValue(0);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlOverride(bool override)
{
  mHtmlOverride = override;
  if (message())
      message()->setDecodeHTML(htmlMail());
}


//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}


//-----------------------------------------------------------------------------
void KMReaderWin::update( bool force )
{
    setMsg( message(), force );
}


//-----------------------------------------------------------------------------
KMMessage* KMReaderWin::message( KMFolder** aFolder ) const
{
  KMFolder*  tmpFolder;
  KMFolder*& folder = aFolder ? *aFolder : tmpFolder;
  folder = 0;
  if (mMessage)
      return mMessage;
  if (mLastSerNum) {
    KMMessage *message = 0;
    int index;
    kernel->msgDict()->getLocation( mLastSerNum, &folder, &index );
    if (folder )
      message = folder->getMsg( index );
    if (!message)
      kdDebug(5006) << "Attempt to reference invalid serial number " << mLastSerNum << "\n" << endl;
    return message;
  }
  return 0;
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotReplyToMsg()
{
  KMMessage *msg = message();
  KMCommand *command = new KMReplyToCommand( mMainWindow, msg, copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotReplyAllToMsg()
{
  KMMessage *msg = message();
  KMCommand *command = new KMReplyToAllCommand( mMainWindow, msg, copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotForward() {
  // ### FIXME: remember the last used subaction and use that
  // currently, we hard-code "as attachment".
  slotForwardAttachedMsg();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotForwardMsg()
{
  QPtrList<KMMsgBase> msgList;
  msgList.append( message() );
  KMCommand *command = new KMForwardCommand( mMainWindow, msgList );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotForwardAttachedMsg()
{
  QPtrList<KMMsgBase> msgList;
  msgList.append( message() );
  KMCommand *command = new KMForwardAttachedCommand( mMainWindow, msgList, 0 );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotRedirectMsg()
{
  KMCommand *command = new KMRedirectCommand( mMainWindow, message() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotBounceMsg()
{
  KMCommand *command = new KMBounceCommand( mMainWindow, message() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotReplyListToMsg()
{
  KMCommand *command = new KMReplyListCommand( mMainWindow, message(),
					       copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotNoQuoteReplyToMsg()
{
  KMCommand *command = new KMNoQuoteReplyToCommand( mMainWindow, message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotSubjectFilter()
{
  KMMessage *msg = message();
  if (msg) {
    KMCommand *command = new KMFilterCommand( "Subject", msg->subject() );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailingListFilter()
{
  KMMessage *msg = message();
  if (msg) {
    KMCommand *command = new KMMailingListFilterCommand( mMainWindow, msg );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotFromFilter()
{
  KMMessage *msg = message();
  if (msg) {
    KMCommand *command = new KMFilterCommand( "From",  msg->from() );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotToFilter()
{
  KMMessage *msg = message();
  if (msg) {
    KMCommand *command = new KMFilterCommand( "To",  msg->to() );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::updateListFilterAction()
{
  //Proxy the mListFilterAction to update the action text
  QCString name;
  QString value;
  QString lname = KMMLInfo::name( message(), name, value );
  mListFilterAction->setText( i18n("Filter on Mailing-List...") );
  if ( lname.isNull() )
    mListFilterAction->setEnabled( false );
  else {
    mListFilterAction->setEnabled( true );
    mListFilterAction->setText( i18n( "Filter on Mailing-List %1..." ).arg( lname ) );
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlClicked()
{
  KMMainWidget *mainWidget = dynamic_cast<KMMainWidget*>(mMainWindow);
  uint identity = 0;
  if (message() && message()->parent())
    identity = message()->parent()->identity();
  KMCommand *command = new KMUrlClickedCommand( mUrlClicked, identity, this,
						false, mainWidget );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoCompose()
{
  KMCommand *command = new KMMailtoComposeCommand( mUrlClicked, message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoForward()
{
  KMCommand *command = new KMMailtoForwardCommand( mMainWindow, mUrlClicked,
						   message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoAddAddrBook()
{
  KMCommand *command = new KMMailtoAddAddrBookCommand( mUrlClicked,
						       mMainWindow);
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoOpenAddrBook()
{
  KMCommand *command = new KMMailtoOpenAddrBookCommand( mUrlClicked,
							mMainWindow );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlCopy()
{
  KMMainWidget *mainWidget = dynamic_cast<KMMainWidget*>(mMainWindow);
  KMCommand *command = new KMUrlCopyCommand( mUrlClicked, mainWidget );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen()
{
  KMCommand *command = new KMUrlOpenCommand( mUrlClicked, this );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlSave()
{
  KMCommand *command = new KMUrlSaveCommand( mUrlClicked, mMainWindow );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoReply()
{
  KMCommand *command = new KMMailtoReplyCommand( mMainWindow, mUrlClicked,
    message(), copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotShowMsgSrc()
{
  KMCommand *command = new KMShowMsgSrcCommand( mMainWindow, message(),
    mCodec, isfixedFont() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotPrintMsg()
{
  KMCommand *command = new KMPrintCommand( mMainWindow, message(), htmlOverride() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotSaveMsg()
{
  QPtrList<KMMsgBase> msgList;
  msgList.append( message() );
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( mMainWindow,
							msgList );
  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
}

int KMReaderWin::fontSize() const {
  return mUseFixedFont ? mFixedFont.pointSize() : mBodyFont.pointSize() ;
}

QString KMReaderWin::bodyFontFamily() const {
  return mUseFixedFont ? mFixedFont.family() : mBodyFont.family();
}


//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"


