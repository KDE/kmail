// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

// #define STRICT_RULES_OF_GERMAN_GOVERNMENT_02

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
//#define KMAIL_READER_HTML_DEBUG 1

#include <config.h>

#include "kmreaderwin.h"

#include "kmversion.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include "kmgroupware.h"
#include "kmailicalifaceimpl.h"
#include "kfileio.h"
#include "kmfolderindex.h"
#include "kmcommands.h"
#include "kmmsgpartdlg.h"
#include "mailsourceviewer.h"
using KMail::MailSourceViewer;
#include "partNode.h"
#include "kmmsgdict.h"
#include "kmsender.h"
#include "kcursorsaver.h"
#include "mailinglist-magic.h"
#include "kmkernel.h"
#include "vcardviewer.h"
using KMail::VCardViewer;
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
#include "htmlstatusbar.h"
using KMail::HtmlStatusBar;
#include "folderjob.h"
using KMail::FolderJob;
#include "csshelper.h"
using KMail::CSSHelper;
#include "isubject.h"
using KMail::ISubject;

#ifdef KMAIL_READER_HTML_DEBUG
#include "filehtmlwriter.h"
using KMail::FileHtmlWriter;
#include "teehtmlwriter.h"
using KMail::TeeHtmlWriter;
#endif

#include <kmime_mdn.h>
#include <kmime_header_parsing.h>
using KMime::Types::AddrSpecList;
using namespace KMime;
#include <kapplication.h>
// for the click on attachment stuff (dnaber):
#include <kuserprofile.h>
#include <kcharsets.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>  // Sven's : for access and getpid
#include <kcursor.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kglobalsettings.h>
#include <krun.h>
#include <ktempfile.h>
#include <kprocess.h>
#include <kdialog.h>

// KABC includes
#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>

// khtml headers
#include <khtml_part.h>
#include <khtmlview.h> // So that we can get rid of the frames
#include <dom/html_element.h>
#include <dom/html_block.h>

#include <qclipboard.h>
#include <qhbox.h>
#include <qtextcodec.h>
#include <qpaintdevicemetrics.h>
#include <qlayout.h>
#include <qlabel.h>

#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

// X headers...
#undef Never
#undef Always

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

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
      //    AND we are not replacing a node that already *has* replaced
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
    mOverrideCodec( 0 ),
    mCSSHelper( 0 ),
    //mShowCompleteMessage( false ),
    mMimePartTree( mimePartTree ),
    mShowMIMETreeMode( showMIMETreeMode ),
    mRootNode( 0 ),
    mMainWindow( mainWindow ),
    mActionCollection( actionCollection ),
    mHtmlWriter( 0 )
{
  mAutoDelete = false;
  mLastSerNum = 0;
  mMessage = 0;
  mLastStatus = KMMsgStatusUnknown;
  mMsgDisplay = true;
  mPrinting = false;
  mShowColorbar = false;
  mAtmUpdate = false;

  initHtmlWidget();
  readConfig();
  mHtmlOverride = false;

  connect( &updateReaderWinTimer, SIGNAL(timeout()),
  	   this, SLOT(updateReaderWin()) );
  connect( &mResizeTimer, SIGNAL(timeout()),
  	   this, SLOT(slotDelayedResize()) );
  connect( &mDelayedMarkTimer, SIGNAL(timeout()),
           this, SLOT(slotTouchMessage()) );

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
			     SLOT(slotUrlCopy()), ac, "copy_address" );
  mCopyURLAction = new KAction( i18n("Copy Link Location"), 0, this,
				SLOT(slotUrlCopy()), ac, "copy_url" );
  mUrlOpenAction = new KAction( i18n("Open URL"), 0, this,
			     SLOT(slotUrlOpen()), ac, "open_url" );
  mAddBookmarksAction = new KAction( i18n("Add to Bookmarks"), "bookmark_add",
                                     0, this, SLOT(slotAddBookmarks()),
                                     ac, "add_bookmarks" );
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

  mForwardAction = ac->action( "message_forward_inline" );
  if (!mForwardAction) {
    mForwardAction = new KAction( i18n("&Inline..."), "mail_forward",
                                  SHIFT+Key_F, this, SLOT(slotForwardMsg()),
                                  ac, "message_forward_inline" );
  }
  mForwardActionMenu->insert( mForwardAction );

  mForwardAttachedAction = ac->action( "message_forward_as_attachment" );
  if (!mForwardAttachedAction) {
    mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", Key_F, this,
					SLOT(slotForwardAttachedMsg()), ac,
					"message_forward_as_attachment" );
  }
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
bool KMReaderWin::update( KMail::ISubject * subject )
{
  if ( static_cast<KMMessage*>(subject) != message() )
  {
    kdDebug(5006) << "KMReaderWin::update - ignoring update" << endl;
    return false;
  }
  if ( mAtmUpdate )
  {
    partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
    if ( node )
    {
      // replace the dwpart of the node
      node->setDwPart( static_cast<KMMessage*>(subject)->lastUpdatedPart() );
      // update the tmp file
      kByteArrayToFile( node->msgPart().bodyDecodedBinary(), mAtmCurrentName,
          false, false, false );
    }
  } else {
    updateReaderWin();
  }
//  mAtmUpdate = false;

  return true;
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMimePartTree( KMMimePartTree* mimePartTree )
{
  mMimePartTree = mimePartTree;
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
    delete mCSSHelper;
    mCSSHelper = new CSSHelper( QPaintDeviceMetrics( mViewer->view() ), this );
    if (message())
      message()->readConfig();
    update( true ); // Force update
    return true;
  }
  return KMReaderWinInherited::event(e);
}


//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = KMKernel::config();

  delete mCSSHelper;
  mCSSHelper = new CSSHelper( QPaintDeviceMetrics( mViewer->view() ), this );

  { // must be done before setHeaderStyleAndStrategy
    KConfigGroup behaviour( KMKernel::config(), "Behaviour" );
    mDelayedMarkAsRead = behaviour.readBoolEntry( "DelayedMarkAsRead", true );
    mDelayedMarkTimeout = behaviour.readNumEntry( "DelayedMarkTime", 0 );
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
void KMReaderWin::initHtmlWidget(void)
{
  mBox = new QHBox(this);

  mColorBar = new HtmlStatusBar( mBox );

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
          SLOT(slotPreUrlOpen()));
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotPreUrlOpen()));
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotPreUrlPopup(const QString &, const QPoint &)));
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

void KMReaderWin::setOverrideCodec( const QTextCodec * codec ) {
  if ( mOverrideCodec == codec )
    return;
  mOverrideCodec = codec;
  update( true );
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

  // (de)register as observer
  if (mMessage)
    mMessage->detach( this );
  if (aMsg)
    aMsg->attach( this );
  mAtmUpdate = false;

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

  if (aMsg) {
    aMsg->setOverrideCodec( overrideCodec() );
    aMsg->setDecodeHTML( htmlMail() );
    mLastStatus = aMsg->status();
  } else {
    mLastStatus = KMMsgStatusUnknown;
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

  if (mDelayedMarkAsRead) {
    if (mDelayedMarkTimeout != 0)
      mDelayedMarkTimer.start( mDelayedMarkTimeout * 1000, TRUE );
    else
      slotTouchMessage();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  if (mLastSerNum > 0) // no risk for a dangling pointer
    return;
  updateReaderWinTimer.stop();
  clear();
  mDelayedMarkTimer.stop();
  mLastSerNum = 0;
  mMessage = 0;
}

// enter items for the "Important changes" list here:
static const char * const kmailChanges[] = {
  I18N_NOOP("Operations on the parent of a closed thread are now performed on all messages of that thread. That means it is now possible for example to delete a whole thread or subthread by closing it and deleting the parent.")
};
static const int numKMailChanges =
  sizeof kmailChanges / sizeof *kmailChanges;

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  I18N_NOOP("KMail can now be embedded in the Kontact container application."),
  I18N_NOOP("Search Folders (a.k.a Virtual Folders)"),
  I18N_NOOP("As-you-type spell checking is supported."),
  I18N_NOOP("Panel applet showing unread message totals."),
  I18N_NOOP("Per folder duplicate mail removal"),
  I18N_NOOP("Drag and drop support of messages onto the composer"),
  I18N_NOOP("Improved threading; threading by subject, sort stable deletion"),
  I18N_NOOP("Numerous search dialog improvements"),
  I18N_NOOP("SMTP pipelining (faster mail submission)."),
  I18N_NOOP("Separate reader window improvements, including new tool bar"),
  I18N_NOOP("Configurable startup folder."),
  I18N_NOOP("IMAP messages are loaded progressively."),
  I18N_NOOP("IMAP attachments are loaded on demand."),
  I18N_NOOP("KMail can check your accounts for new mail on startup."),
  I18N_NOOP("Individual imap folders can be checked for new mail."),
  I18N_NOOP("Ignore Thread and Watch Thread."),
  I18N_NOOP("Messages can have more than one status.")
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
	 "%7: First-time user text (only shown on first start); "
	 "%8: prior KMail version; "
         "%9: generated list of important changes; "
	 "--- end of comment ---",
	 "<h2>Welcome to KMail %1</h2><p>KMail is the email client for the K "
	 "Desktop Environment. It is designed to be fully compatible with "
	 "Internet mailing standards including MIME, SMTP, POP3 and IMAP."
	 "</p>\n"
	 "<ul><li>KMail has many powerful features which are described in the "
	 "<a href=\"%2\">documentation</a></li>\n"
	 "<li>The <a href=\"%3\">KMail homepage</A> offers information about "
	 "new versions of KMail</li></ul>\n"
         "<p><span style='font-size:125%; font-weight:bold;'>"
         "Important changes</span> (compared to KMail %8):</p>\n"
	 "<ul>\n%9</ul>\n"
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

  QString changesItems;
  for ( int i = 0 ; i < numKMailChanges ; i++ )
    changesItems += i18n("<li>%1</li>\n").arg( i18n( kmailChanges[i] ) );

  info = info.arg("1.5").arg( changesItems );

  mViewer->write(content.arg(pointsToPixel( mCSSHelper->bodyFont().pointSize() )).arg(info));
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
  if (!mMsgDisplay) return;

  htmlWriter()->reset();

  KMFolder* folder;
  if (message(&folder))
  {
    if( !kernel->iCalIface().isResourceImapFolder( folder ) ){
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
    htmlWriter()->write( mCSSHelper->htmlHead( isFixedFont() ) + "</body></html>" );
    htmlWriter()->end();
    if( mMimePartTree )
      mMimePartTree->clear();
  }
}

//-----------------------------------------------------------------------------
int KMReaderWin::pointsToPixel(int pointSize) const
{
  const QPaintDeviceMetrics pdm(mViewer->view());

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

  msg->setOverrideCodec( overrideCodec() );

  htmlWriter()->begin();
  htmlWriter()->queue( mCSSHelper->htmlHead( isFixedFont() ) );

  if (!parent())
    setCaption(msg->subject());

  removeTempFiles();

  mColorBar->setNeutralMode();

  parseMsg(msg);

  if( mColorBar->isNeutral() )
    mColorBar->setNormalMode();

  htmlWriter()->queue("</body></html>");
  htmlWriter()->flush();
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
#ifndef NDEBUG
  kdDebug( 5006 )
    << "\n#######\n#######\n#######  parseMsg(KMMessage* aMsg "
    << ( aMsg == message() ? "==" : "!=" )
    << " aMsg )\n#######\n#######";
#endif

  KMMessagePart msgPart;
  QCString subtype, contDisp;
  QByteArray str;

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
    if( mMimePartTree )
      mRootNode->fillMimePartTree( 0,
                                   mMimePartTree,
                                   cntDesc,
                                   mainCntTypeStr,
                                   cntEnc,
                                   cntSize );
  } else if( mMimePartTree ) {
kdDebug(5006) << "\n     ----->  Inserting Root Node into the Mime Part Tree" << endl;
    mRootNode->fillMimePartTree( 0,
                                 mMimePartTree,
                                 cntDesc,
                                 mainCntTypeStr,
                                 cntEnc,
                                 cntSize );
kdDebug(5006) << "\n     <-----  Finished inserting Root Node into Mime Part Tree" << endl;
  } else {
kdDebug(5006) << "\n     ------  Sorry, no Mime Part Tree - can NOT insert Root Node!" << endl;
  }

  partNode* vCardNode = mRootNode->findType( DwMime::kTypeText, DwMime::kSubtypeXVCard );
  bool hasVCard = false;
  if( vCardNode ) {
    const QString vcard = vCardNode->msgPart().bodyToUnicode( overrideCodec() );
    KABC::VCardConverter vc;
    KABC::Addressee a;
    bool isOk = vc.vCardToAddressee(vcard, a, KABC::VCardConverter::v3_0);
    if (!isOk)
      isOk = vc.vCardToAddressee(vcard, a, KABC::VCardConverter::v2_1);
    if( isOk ) {
      hasVCard = true;
      kdDebug(5006) << "FOUND A VALID VCARD" << endl;
      writeMessagePartToTempFile( &vCardNode->msgPart(), vCardNode->nodeId() );
    }
  }
  htmlWriter()->queue( writeMsgHeader(aMsg, hasVCard) );


  // show message content
  ObjectTreeParser otp( this );
  otp.parseObjectTree( mRootNode );

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
kdDebug(5006) << "(aMsg == msg) = "                               << (aMsg == message()) << endl;
kdDebug(5006) << "   (KMMsgStatusUnknown == mLastStatus) = "           << (KMMsgStatusUnknown == mLastStatus) << endl;
kdDebug(5006) << "|| (KMMsgStatusNew     == mLastStatus) = "           << (KMMsgStatusNew     == mLastStatus) << endl;
kdDebug(5006) << "|| (KMMsgStatusUnread  == mLastStatus) = "           << (KMMsgStatusUnread  == mLastStatus) << endl;
kdDebug(5006) << "(mIdOfLastViewedMessage != aMsg->msgId()) = "    << (mIdOfLastViewedMessage != aMsg->msgId()) << endl;
kdDebug(5006) << "   (KMMsgFullyEncrypted == encryptionState) = "     << (KMMsgFullyEncrypted == encryptionState) << endl;
kdDebug(5006) << "|| (KMMsgPartiallyEncrypted == encryptionState) = " << (KMMsgPartiallyEncrypted == encryptionState) << endl;
         // only proceed if we were called the normal way - not by
         // double click on the message (==not running in a separate window)
  if(    (aMsg == message())
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

  // store message id to avoid endless recursions
  setIdOfLastViewedMessage( aMsg->msgId() );

  if( emitReplaceMsgByUnencryptedVersion ) {
kdDebug(5006) << "KMReaderWin  -  invoce saving in decrypted form:" << endl;
    emit replaceMsgByUnencryptedVersion();
  } else {
kdDebug(5006) << "KMReaderWin  -  finished parsing and displaying of message." << endl;
      showHideMimeTree( (DwMime::kTypeMultipart   == rootNodeCntType) ||
                        (DwMime::kTypeApplication == rootNodeCntType) ||
                        (DwMime::kTypeMessage     == rootNodeCntType) ||
                        (DwMime::kTypeModel       == rootNodeCntType) );
  }
}


//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader(KMMessage* aMsg, bool hasVCard)
{
  kdFatal( !headerStyle(), 5006 )
    << "trying to writeMsgHeader() without a header style set!" << endl;
  kdFatal( !headerStrategy(), 5006 )
    << "trying to writeMsgHeader() without a header strategy set!" << endl;
  QString href;
  if (hasVCard)
    href = QString("file:") + KURL::encode_string( mTempFiles.last() );

  return headerStyle()->format( aMsg, headerStrategy(),
				hasVCard ? href : QString::null,
				mPrinting );
}



//-----------------------------------------------------------------------------
QString KMReaderWin::writeMessagePartToTempFile( KMMessagePart* aMsgPart,
                                                 int aPartNum )
{
  QString fileName = aMsgPart->fileName();
  if( fileName.isEmpty() )
    fileName = aMsgPart->name();

  //--- Sven's save attachments to /tmp start ---
  KTempFile *tempFile = new KTempFile( QString::null,
                                       "." + QString::number( aPartNum ) );
  tempFile->setAutoDelete( true );
  QString fname = tempFile->name();
  delete tempFile;

  if( ::access( QFile::encodeName( fname ), W_OK ) != 0 )
    // Not there or not writable
    if( ::mkdir( QFile::encodeName( fname ), 0 ) != 0
        || ::chmod( QFile::encodeName( fname ), S_IRWXU ) != 0 )
      return QString::null; //failed create

  assert( !fname.isNull() );

  mTempDirs.append( fname );
  // strip off a leading path
  int slashPos = fileName.findRev( '/' );
  if( -1 != slashPos )
    fileName = fileName.mid( slashPos + 1 );
  if( fileName.isEmpty() )
    fileName = "unnamed";
  fname += "/" + fileName;

  if( !kByteArrayToFile( aMsgPart->bodyDecodedBinary(), fname, false, false,
			false ) )
    return QString::null;

  mTempFiles.append( fname );

  return fname;
}


//-----------------------------------------------------------------------------
void KMReaderWin::showVCard( KMMessagePart * msgPart ) {
  const QString vCard = msgPart->bodyToUnicode( overrideCodec() );

  VCardViewer *vcv = new VCardViewer(this, vCard, "vCardDialog");
  vcv->show();
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
    if (message()->isNew() || message()->isUnread() || message()->isRead())
      message()->setStatus(KMMsgStatusRead);
    if ( message()->isNew() || message()->isUnread() ) {
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

  if( kernel->groupware().isEnabled()
      && kernel->groupware().handleLink( aUrl, message() ) )
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
void KMReaderWin::slotUrlPopup()
{
  slotUrlPopup( mUrlClicked.path(), mPos );
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
  win->showMsg( overrideCodec(), msg );
  win->resize(550,600);
  win->show();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMsgPart( KMMessagePart* aMsgPart, bool aHTML,
			      const QString& aFileName, const QString& pname )
{
  KCursorSaver busy(KBusyPtr::busy());
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
        showVCard( aMsgPart );
	return;
      }
      htmlWriter()->begin();
      htmlWriter()->queue( mCSSHelper->htmlHead( isFixedFont() ) );

      if (aHTML && (qstricmp(aMsgPart->subtypeStr(), "html")==0)) { // HTML
	// ### this is broken. It doesn't stip off the HTML header and footer!
	htmlWriter()->queue( aMsgPart->bodyToUnicode( overrideCodec() ) );
	mColorBar->setHtmlMode();
      } else { // plain text
	const QCString str = aMsgPart->bodyDecoded();
	ObjectTreeParser otp( this );
	otp.writeBodyStr( str,
			  overrideCodec() ? overrideCodec() : aMsgPart->codec(),
			  message() ? message()->from() : QString::null );
      }
      htmlWriter()->queue("</body></html>");
      htmlWriter()->flush();
      mMainWindow->setCaption(i18n("View Attachment: %1").arg(pname));
  } else if (qstricmp(aMsgPart->typeStr(), "image")==0 ||
             (qstricmp(aMsgPart->typeStr(), "application")==0 &&
              qstricmp(aMsgPart->subtypeStr(), "postscript")))
  {
      if (aFileName.isEmpty())
	  return;  // prevent crash

      if (aFileName.isEmpty()) return;  // prevent crash
      // Open the window with a size so the image fits in (if possible):
      QImageIO *iio = new QImageIO();
      iio->setFileName(aFileName);
      if( iio->read() ) {
          QImage img = iio->image();
#if KDE_IS_VERSION( 3, 1, 90 )
          QRect desk = KGlobalSettings::desktopGeometry(mMainWindow);
#else
          QRect desk = QApplication::desktop()->screen(QApplication::desktop()->screenNumber(mMainWindow))->rect();
#endif
          // determine a reasonable window size
          int width, height;
          if( img.width() < 50 )
              width = 70;
          else if( img.width()+20 < desk.width() )
              width = img.width()+20;
          else
              width = desk.width();
          if( img.height() < 50 )
              height = 70;
          else if( img.height()+20 < desk.height() )
              height = img.height()+20;
          else
              height = desk.height();
          mMainWindow->resize( width, height );
      }
      // Just write the img tag to HTML:
      htmlWriter()->begin();
      htmlWriter()->write( mCSSHelper->htmlHead( isFixedFont() ) );
      htmlWriter()->write( "<img src=\"file:" +
			   KURL::encode_string( aFileName ) +
			   "\" border=\"0\">\n"
			   "</body></html>\n" );
      htmlWriter()->end();
      setCaption( i18n("View Attachment: %1").arg( pname ) );
      show();
  } else {
      MailSourceViewer *viewer = new MailSourceViewer(); // deletes itself
      QString str = aMsgPart->bodyDecoded();
      // A QString cannot handle binary data. So if it's shorter than the
      // attachment, we assume the attachment is binary:
      if( str.length() < (unsigned) aMsgPart->decodedSize() ) {
        str += QString::fromLatin1("\n") + i18n("[KMail: Attachment contains binary data. Trying to show first character.]",
                    "[KMail: Attachment contains binary data. Trying to show first %n characters.]",
                    str.length());
      }
      viewer->setText(str);
      viewer->resize(500, 550);
      viewer->show();
  }
  // ---Sven's view text, html and image attachments in html widget end ---
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
    if (qstricmp(msgPart.typeStr(), "message")==0) {
      atmViewMsg(&msgPart);
    } else if ((qstricmp(msgPart.typeStr(), "text")==0) &&
	       (qstricmp(msgPart.subtypeStr(), "x-vcard")==0)) {
      setMsgPart( &msgPart, htmlMail(), mAtmCurrentName, pname );
    } else {
      KMReaderMainWin *win = new KMReaderMainWin(&msgPart, htmlMail(),
	mAtmCurrentName, pname, overrideCodec() );
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
     showVCard( &msgPart );
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
  if ( !mRootNode )
    return;

  partNode * node = mRootNode->findId( mAtmCurrent );
  if ( !node )
    return;

  const KMMessagePart & msgPart = node->msgPart();

  // prepend the previously used save dir,
  // replace all ':' with '_' because ':' isn't allowed on FAT volumes
  const QString fileName =
    mSaveAttachDir + mAtmCurrentName.section( '/', -1 ).replace( ':', '_' );

  // ### getSaveURL should allow setting "::<attachments>" _and_ a
  // ### proposed filename (currently, it's xor)...
  // ### We could then get rid of mSaveAttachDir!
  const KURL url = KFileDialog::getSaveURL( fileName, QString::null, this,
					    i18n("Save Attachment As") );
  if ( url.isEmpty() )
    return;

  mSaveAttachDir = url.directory() + '/';

  kernel->byteArrayToRemoteFile( msgPart.bodyDecodedBinary(), url );
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmProperties()
{
    KMMsgPartDialogCompat dlg(0,TRUE);

    KCursorSaver busy(KBusyPtr::busy());
    partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
    if( node ) {
        KMMessagePart& msgPart = node->msgPart();

        dlg.setMsgPart(&msgPart);
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
      kdWarning(5006) << "Attempt to reference invalid serial number " << mLastSerNum << "\n" << endl;
    return message;
  }
  return 0;
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotReplyToMsg()
{
  KMCommand *command = new KMReplyToCommand( mMainWindow, message(), copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotReplyAllToMsg()
{
  KMCommand *command = new KMReplyToAllCommand( mMainWindow, message(), copyText() );
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
  KMCommand *command = new KMForwardCommand( mMainWindow, message() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotForwardAttachedMsg()
{
  KMCommand *command = new KMForwardAttachedCommand( mMainWindow, message(), 0 );
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
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "Subject", msg->subject() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailingListFilter()
{
  KMMessage *msg = message();
  if (!msg)
    return;

  KMCommand *command = new KMMailingListFilterCommand( mMainWindow, msg );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotFromFilter()
{
  KMMessage *msg = message();
  if (!msg)
    return;

  AddrSpecList al = msg->extractAddrSpecs( "From" );
  if ( al.empty() )
    return;
  KMCommand *command = new KMFilterCommand( "From",  al.front().asString() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotToFilter()
{
  KMMessage *msg = message();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "To",  msg->to() );
  command->start();
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
  if (mainWidget && (message() && message()->parent()))
  {
    identity = message()->parent()->identity();
    KMCommand *command = new KMUrlClickedCommand( mUrlClicked, identity, this,
						false, mainWidget );
    command->start();
  }
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
  if (mainWidget)
  {
    KMCommand *command = new KMUrlCopyCommand( mUrlClicked, mainWidget );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotPreUrlOpen()
{
  mAtmCurrent = msgPartFromUrl( mUrlClicked );
  if ( mAtmCurrent > 0 )
  {
    // these are needed for update( KMail::ISubject * )
    mAtmUpdate = true;
    mAtmCurrentName = mUrlClicked.path();

    partNode* node = partNodeFromUrl( mUrlClicked );
    KMLoadPartsCommand *command = new KMLoadPartsCommand( node, message() );
    connect( command, SIGNAL( partsRetrieved() ),
        this, SLOT( slotUrlOpen() ) );
    command->start();
  } else
    slotUrlOpen( mUrlClicked, KParts::URLArgs() );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotPreUrlPopup( const QString &aUrl, const QPoint &pos )
{
  KURL url( aUrl );
  mAtmCurrent = msgPartFromUrl( url );
  mUrlClicked = url;
  mPos = pos;
  if ( mAtmCurrent > 0 )
  {
    // these are needed for update( KMail::ISubject * )
    mAtmUpdate = true;
    mAtmCurrentName = url.path();

    partNode* node = partNodeFromUrl( aUrl );
    KMLoadPartsCommand *command = new KMLoadPartsCommand( node, message() );
    connect( command, SIGNAL( partsRetrieved() ),
        this, SLOT( slotUrlPopup() ) );
    command->start();
  } else
    slotUrlPopup( aUrl, pos );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen()
{
  KMCommand *command = new KMUrlOpenCommand( mUrlClicked, this );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotAddBookmarks()
{
    KMCommand *command = new KMAddBookmarksCommand( mUrlClicked, this );
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
  KMMessage *msg = message();
  bool oldStatus = msg->isComplete();
  msg->setComplete( true ); // otherwise imap messages are completely downloaded
  KMCommand *command = new KMShowMsgSrcCommand( mMainWindow, msg,
                                                isFixedFont() );
  command->start();
  msg->setComplete( oldStatus );
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
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( mMainWindow,
							message() );
  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
}

//-----------------------------------------------------------------------------
partNode* KMReaderWin::partNodeFromUrl( const KURL &url )
{
  int id = msgPartFromUrl(url);
  partNode* node = mRootNode ? mRootNode->findId(id) : 0;

  return node;
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotSaveAttachments()
{
  mAtmUpdate = true;
  KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( mMainWindow,
                                                                        message() );
  saveCommand->start();
}

//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"


