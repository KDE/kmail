/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
//#define KMAIL_READER_HTML_DEBUG 1
#include <config-kmail.h>

#include "kmreaderwin.h"

#include "globalsettings.h"
#include "kmversion.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include <kpimutils/kfileio.h>
#include "kmfolderindex.h"
#include "kmcommands.h"
#include "kmmsgpartdlg.h"
#include "mailsourceviewer.h"
#include <QByteArray>
#include <QImageReader>
#include <QCloseEvent>
#include <QEvent>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSignalMapper>
using KMail::MailSourceViewer;
#include "partNode.h"
#include "kmmsgdict.h"
#include "messagesender.h"
#include "kcursorsaver.h"
#include "kmfolder.h"
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
#include "urlhandlermanager.h"
using KMail::URLHandlerManager;
#include "interfaces/observable.h"
#include "util.h"
#include <kicon.h>
#include "broadcaststatus.h"

#include <kmime/kmime_mdn.h>
using namespace KMime;
#ifdef KMAIL_READER_HTML_DEBUG
#include "filehtmlwriter.h"
using KMail::FileHtmlWriter;
#include "teehtmlwriter.h"
using KMail::TeeHtmlWriter;
#endif

#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

#include "kleo/specialjob.h"
#include "kleo/cryptobackend.h"
#include "kleo/cryptobackendfactory.h"

// KABC includes
#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>

// khtml headers
#include <khtml_part.h>
#include <khtmlview.h> // So that we can get rid of the frames
#include <dom/html_element.h>
#include <dom/html_block.h>
#include <dom/html_document.h>
#include <dom/dom_string.h>

#include <kde_file.h>
#include <kactionmenu.h>
// for the click on attachment stuff (dnaber):
#include <kcharsets.h>
#include <kmenu.h>
#include <kstandarddirs.h>  // Sven's : for access and getpid
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kglobalsettings.h>
#include <krun.h>
#include <ktemporaryfile.h>
#include <kdialog.h>
#include <kaction.h>
#include <kfontaction.h>
#include <kiconloader.h>
#include <kcodecs.h>
#include <kascii.h>
#include <kselectaction.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <kconfiggroup.h>

#include <QClipboard>
#include <QCursor>
#include <QTextCodec>
#include <QLayout>
#include <QLabel>
#include <QSplitter>
#include <QStyle>

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
#include <kvbox.h>
#include <QTextDocument>
#endif

// This function returns the complete data that were in this
// message parts - *after* all encryption has been removed that
// could be removed.
// - This is used to store the message in decrypted form.
void KMReaderWin::objectTreeToDecryptedMsg( partNode* node,
                                            QByteArray& resultingData,
                                            KMMessage& theMessage,
                                            bool weAreReplacingTheRootNode,
                                            int recCount )
{
  kDebug(5006) << "-------------------------------------------------";
  kDebug(5006) << "START" << "(" << recCount << ")";
  if( node ) {
    partNode* curNode = node;
    partNode* dataNode = curNode;
    partNode * child = node->firstChild();
    bool bIsMultipart = false;

    switch( curNode->type() ){
      case DwMime::kTypeText: {
          kDebug(5006) <<"* text *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeHtml:
            kDebug(5006) <<"html";
            break;
          case DwMime::kSubtypeXVCard:
            kDebug(5006) <<"v-card";
            break;
          case DwMime::kSubtypeRichtext:
            kDebug(5006) <<"rich text";
            break;
          case DwMime::kSubtypeEnriched:
            kDebug(5006) <<"enriched";
            break;
          case DwMime::kSubtypePlain:
            kDebug(5006) <<"plain";
            break;
          default:
            kDebug(5006) <<"default";
            break;
          }
        }
        break;
      case DwMime::kTypeMultipart: {
          kDebug(5006) <<"* multipart *";
          bIsMultipart = true;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMixed:
            kDebug(5006) <<"mixed";
            break;
          case DwMime::kSubtypeAlternative:
            kDebug(5006) <<"alternative";
            break;
          case DwMime::kSubtypeDigest:
            kDebug(5006) <<"digest";
            break;
          case DwMime::kSubtypeParallel:
            kDebug(5006) <<"parallel";
            break;
          case DwMime::kSubtypeSigned:
            kDebug(5006) <<"signed";
            break;
          case DwMime::kSubtypeEncrypted: {
              kDebug(5006) <<"encrypted";
              if ( child ) {
                /*
                    ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                */
                partNode* data =
                  child->findType( DwMime::kTypeApplication, DwMime::kSubtypeOctetStream, false, true );
                if ( !data )
                  data = child->findType( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Mime, false, true );
                if ( data && data->firstChild() )
                  dataNode = data;
              }
            }
            break;
          default :
            kDebug(5006) <<"(  unknown subtype  )";
            break;
          }
        }
        break;
      case DwMime::kTypeMessage: {
          kDebug(5006) <<"* message *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeRfc822: {
              kDebug(5006) <<"RfC 822";
              if ( child )
                dataNode = child;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeApplication: {
          kDebug(5006) <<"* application *";
          switch( curNode->subType() ){
          case DwMime::kSubtypePostscript:
            kDebug(5006) <<"postscript";
            break;
          case DwMime::kSubtypeOctetStream: {
              kDebug(5006) <<"octet stream";
              if ( child )
                dataNode = child;
            }
            break;
          case DwMime::kSubtypePgpEncrypted:
            kDebug(5006) <<"pgp encrypted";
            break;
          case DwMime::kSubtypePgpSignature:
            kDebug(5006) <<"pgp signed";
            break;
          case DwMime::kSubtypePkcs7Mime: {
              kDebug(5006) <<"pkcs7 mime";
              // note: subtype Pkcs7Mime can also be signed
              //       and we do NOT want to remove the signature!
              if ( child && curNode->encryptionState() != KMMsgNotEncrypted )
                dataNode = child;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeImage: {
          kDebug(5006) <<"* image *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeJpeg:
            kDebug(5006) <<"JPEG";
            break;
          case DwMime::kSubtypeGif:
            kDebug(5006) <<"GIF";
            break;
          }
        }
        break;
      case DwMime::kTypeAudio: {
          kDebug(5006) <<"* audio *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeBasic:
            kDebug(5006) <<"basic";
            break;
          }
        }
        break;
      case DwMime::kTypeVideo: {
          kDebug(5006) <<"* video *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeMpeg:
            kDebug(5006) <<"mpeg";
            break;
          }
        }
        break;
      case DwMime::kTypeModel:
          kDebug(5006) <<"* model *";
        break;
    }


    DwHeaders& rootHeaders( theMessage.headers() );
    DwBodyPart * part = dataNode->dwPart() ? dataNode->dwPart() : 0;
    DwHeaders * headers(
        (part && part->hasHeaders())
        ? &part->Headers()
        : (  (weAreReplacingTheRootNode || !dataNode->parentNode())
            ? &rootHeaders
            : 0 ) );
    if( dataNode == curNode ) {
      kDebug(5006) <<"dataNode == curNode:  Save curNode without replacing it.";

      // A) Store the headers of this part IF curNode is not the root node
      //    AND we are not replacing a node that already *has* replaced
      //    the root node in previous recursion steps of this function...
      if( headers ) {
        if( dataNode->parentNode() && !weAreReplacingTheRootNode ) {
          kDebug(5006) <<"dataNode is NOT replacing the root node:  Store the headers.";
          resultingData += headers->AsString().c_str();
        } else if( weAreReplacingTheRootNode && part && part->hasHeaders() ){
          kDebug(5006) <<"dataNode replace the root node:  Do NOT store the headers but change";
          kDebug(5006) <<"                                 the Message's headers accordingly.";
          kDebug(5006) <<"              old Content-Type =" << rootHeaders.ContentType().AsString().c_str();
          kDebug(5006) <<"              new Content-Type =" << headers->ContentType(   ).AsString().c_str();
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
      if( headers && bIsMultipart && dataNode->firstChild() )  {
        kDebug(5006) <<"is valid Multipart, processing children:";
        QByteArray boundary = headers->ContentType().Boundary().c_str();
        curNode = dataNode->firstChild();
        // store children of multipart
        while( curNode ) {
          kDebug(5006) <<"--boundary";
          if( resultingData.size() &&
              ( '\n' != resultingData.at( resultingData.size()-1 ) ) )
            resultingData += '\n';
          resultingData += '\n';
          resultingData += "--";
          resultingData += boundary;
          resultingData += '\n';
          // note: We are processing a harmless multipart that is *not*
          //       to be replaced by one of it's children, therefor
          //       we set their doStoreHeaders to true.
          objectTreeToDecryptedMsg( curNode,
                                    resultingData,
                                    theMessage,
                                    false,
                                    recCount + 1 );
          curNode = curNode->nextSibling();
        }
        kDebug(5006) <<"--boundary--";
        resultingData += "\n--";
        resultingData += boundary;
        resultingData += "--\n\n";
        kDebug(5006) <<"Multipart processing children - DONE";
      } else if( part ){
        // store simple part
        kDebug(5006) <<"is Simple part or invalid Multipart, storing body data .. DONE";
        resultingData += part->Body().AsString().c_str();
      }
    } else {
      kDebug(5006) <<"dataNode != curNode:  Replace curNode by dataNode.";
      bool rootNodeReplaceFlag = weAreReplacingTheRootNode || !curNode->parentNode();
      if( rootNodeReplaceFlag ) {
        kDebug(5006) <<"                      Root node will be replaced.";
      } else {
        kDebug(5006) <<"                      Root node will NOT be replaced.";
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
  kDebug(5006) << "END" << "(" << recCount << ")";
}


/*
 ===========================================================================


        E N D    O F     T E M P O R A R Y     M I M E     C O D E


 ===========================================================================
*/











void KMReaderWin::createWidgets() {
  QVBoxLayout * vlay = new QVBoxLayout( this );
  vlay->setMargin( 0 );
  mSplitter = new QSplitter( Qt::Vertical, this );
  mSplitter->setObjectName( "mSplitter" );
  mSplitter->setChildrenCollapsible( false );
  vlay->addWidget( mSplitter );
  mMimePartTree = new KMMimePartTree( this, mSplitter );
  mMimePartTree->setObjectName( "mMimePartTree" );
  mBox = new KHBox( mSplitter );
  setStyleDependantFrameWidth();
  mBox->setFrameStyle( mMimePartTree->frameStyle() );
  mColorBar = new HtmlStatusBar( mBox );
  mColorBar->setObjectName( "mColorBar" );
  mViewer = new KHTMLPart( mBox );
  mViewer->setObjectName( "mViewer" );
  mSplitter->setStretchFactor( mSplitter->indexOf(mMimePartTree), 0 );
  mSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
}

const int KMReaderWin::delay = 150;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent,
                         QWidget *mainWindow,
                         KActionCollection* actionCollection,
                         Qt::WindowFlags aFlags )
  : QWidget(aParent, aFlags | Qt::WDestructiveClose),
    mAttachmentStrategy( 0 ),
    mHeaderStrategy( 0 ),
    mHeaderStyle( 0 ),
    mUpdateReaderWinTimer( 0 ),
    mResizeTimer( 0 ),
    mDelayedMarkTimer( 0 ),
    mOldGlobalOverrideEncoding( "---" ), // init with dummy value
    mCSSHelper( 0 ),
    mRootNode( 0 ),
    mMainWindow( mainWindow ),
    mActionCollection( actionCollection ),
    mMailToComposeAction( 0 ),
    mMailToReplyAction( 0 ),
    mMailToForwardAction( 0 ),
    mAddAddrBookAction( 0 ),
    mOpenAddrBookAction( 0 ),
    mCopyAction( 0 ),
    mCopyURLAction( 0 ),
    mUrlOpenAction( 0 ),
    mUrlSaveAsAction( 0 ),
    mAddBookmarksAction( 0 ),
    mSelectAllAction( 0 ),
    mSelectEncodingAction( 0 ),
    mToggleFixFontAction( 0 ),
    mHtmlWriter( 0 ),
    mSavedRelativePosition( 0 ),
    mDecrytMessageOverwrite( false ),
    mShowSignatureDetails( false ),
    mShowAttachmentQuicklist( true )
{
  mUpdateReaderWinTimer.setObjectName( "mUpdateReaderWinTimer" );
  mDelayedMarkTimer.setObjectName( "mDelayedMarkTimer" );
  mResizeTimer.setObjectName( "mResizeTimer" );
  mSplitterSizes << 180 << 100;
  mMimeTreeMode = 1;
  mMimeTreeAtBottom = true;
  mAutoDelete = false;
  mLastSerNum = 0;
  mWaitingForSerNum = 0;
  mMessage = 0;
  mLastStatus.clear();
  mMsgDisplay = true;
  mPrinting = false;
  mShowColorbar = false;
  mAtmUpdate = false;

  createWidgets();
  createActions();
  initHtmlWidget();
  readConfig();

  mHtmlOverride = false;
  mHtmlLoadExtOverride = false;

  mLevelQuote = GlobalSettings::self()->collapseQuoteLevelSpin() - 1;

  mResizeTimer.setSingleShot( true );
  connect( &mResizeTimer, SIGNAL(timeout()),
           this, SLOT(slotDelayedResize()) );

  mDelayedMarkTimer.setSingleShot( true );
  connect( &mDelayedMarkTimer, SIGNAL(timeout()),
           this, SLOT(slotTouchMessage()) );

  mUpdateReaderWinTimer.setSingleShot( true );
  connect( &mUpdateReaderWinTimer, SIGNAL(timeout()),
           this, SLOT(updateReaderWin()) );

  setMsg( 0, false );
}

void KMReaderWin::createActions()
{
  KActionCollection *ac = mActionCollection;
  if ( !ac ) {
    return;
  }

  KToggleAction *raction = 0;

  // header style
  KActionMenu *headerMenu  = new KActionMenu(i18nc("View->", "&Headers"), this);
  ac->addAction("view_headers", headerMenu );
  headerMenu->setToolTip( i18n("Choose display style of message headers") );

  connect( headerMenu, SIGNAL(activated()),
           this, SLOT(slotCycleHeaderStyles()) );

  QActionGroup *group = new QActionGroup( this );
  raction = new KToggleAction( i18nc("View->headers->", "&Enterprise Headers"), this);
  ac->addAction( "view_headers_enterprise", raction );
  connect( raction, SIGNAL(triggered(bool)), SLOT(slotEnterpriseHeaders()) );
  raction->setToolTip( i18n("Show the list of headers in Enterprise style") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Fancy Headers"), this);
  ac->addAction("view_headers_fancy", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotFancyHeaders()));
  raction->setToolTip( i18n("Show the list of headers in a fancy format") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Brief Headers"), this);
  ac->addAction("view_headers_brief", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotBriefHeaders()));
  raction->setToolTip( i18n("Show brief list of message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Standard Headers"), this);
  ac->addAction("view_headers_standard", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotStandardHeaders()));
  raction->setToolTip( i18n("Show standard list of message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Long Headers"), this);
  ac->addAction("view_headers_long", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotLongHeaders()));
  raction->setToolTip( i18n("Show long list of message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&All Headers"), this);
  ac->addAction("view_headers_all", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotAllHeaders()));
  raction->setToolTip( i18n("Show all message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  // attachment style
  KActionMenu *attachmentMenu  = new KActionMenu(i18nc("View->", "&Attachments"), this);
  ac->addAction("view_attachments", attachmentMenu );
  attachmentMenu->setToolTip( i18n("Choose display style of attachments") );
  connect( attachmentMenu, SIGNAL(activated()),
           this, SLOT(slotCycleAttachmentStrategy()) );

  group = new QActionGroup( this );
  raction  = new KToggleAction(i18nc("View->attachments->", "&As Icons"), this);
  ac->addAction("view_attachments_as_icons", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotIconicAttachments()));
  raction->setToolTip( i18n("Show all attachments as icons. Click to see them.") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->attachments->", "&Smart"), this);
  ac->addAction("view_attachments_smart", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotSmartAttachments()));
  raction->setToolTip( i18n("Show attachments as suggested by sender.") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->attachments->", "&Inline"), this);
  ac->addAction("view_attachments_inline", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotInlineAttachments()));
  raction->setToolTip( i18n("Show all attachments inline (if possible)") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->attachments->", "&Hide"), this);
  ac->addAction("view_attachments_hide", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotHideAttachments()));
  raction->setToolTip( i18n("Do not show attachments in the message viewer") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  // Set Encoding submenu
  mSelectEncodingAction  = new KSelectAction(KIcon("character-set"), i18n("&Set Encoding"), this);
  ac->addAction("encoding", mSelectEncodingAction );
  connect(mSelectEncodingAction,SIGNAL( triggered(int)),
          SLOT( slotSetEncoding() ));
  QStringList encodings = KMMsgBase::supportedEncodings( false );
  encodings.prepend( i18n( "Auto" ) );
  mSelectEncodingAction->setItems( encodings );
  mSelectEncodingAction->setCurrentItem( 0 );

  mMailToComposeAction  = new KAction(i18n("New Message To..."), this);
  ac->addAction("mail_new", mMailToComposeAction );
  connect(mMailToComposeAction, SIGNAL(triggered(bool) ), SLOT(slotMailtoCompose()));
  mMailToReplyAction  = new KAction(i18n("Reply To..."), this);
  ac->addAction("mailto_reply", mMailToReplyAction );
  connect(mMailToReplyAction, SIGNAL(triggered(bool) ), SLOT(slotMailtoReply()));
  mMailToForwardAction  = new KAction(i18n("Forward To..."), this);
  ac->addAction("mailto_forward", mMailToForwardAction );
  connect(mMailToForwardAction, SIGNAL(triggered(bool) ), SLOT(slotMailtoForward()));
  mAddAddrBookAction  = new KAction(i18n("Add to Address Book"), this);
  ac->addAction("add_addr_book", mAddAddrBookAction );
  connect(mAddAddrBookAction, SIGNAL(triggered(bool) ), SLOT(slotMailtoAddAddrBook()));
  mOpenAddrBookAction  = new KAction(i18n("Open in Address Book"), this);
  ac->addAction("openin_addr_book", mOpenAddrBookAction );
  connect(mOpenAddrBookAction, SIGNAL(triggered(bool) ), SLOT(slotMailtoOpenAddrBook()));
  mCopyAction = ac->addAction(KStandardAction::Copy,  "kmail_copy", this, SLOT(slotCopySelectedText()));
  mSelectAllAction  = new KAction(i18n("Select All Text"), this);
  ac->addAction("mark_all_text", mSelectAllAction );
  connect(mSelectAllAction, SIGNAL(triggered(bool) ), SLOT(selectAll()));
  mSelectAllAction->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_A));
  mCopyURLAction  = new KAction(i18n("Copy Link Address"), this);
  ac->addAction("copy_url", mCopyURLAction );
  connect(mCopyURLAction, SIGNAL(triggered(bool) ), SLOT(slotUrlCopy()));
  mUrlOpenAction  = new KAction(i18n("Open URL"), this);
  ac->addAction("open_url", mUrlOpenAction );
  connect(mUrlOpenAction, SIGNAL(triggered(bool) ), SLOT(slotUrlOpen()));
  mAddBookmarksAction  = new KAction(KIcon("bookmark-new"), i18n("Bookmark This Link"), this);
  ac->addAction("add_bookmarks", mAddBookmarksAction );
  connect(mAddBookmarksAction, SIGNAL(triggered(bool) ), SLOT(slotAddBookmarks()));
  mUrlSaveAsAction  = new KAction(i18n("Save Link As..."), this);
  ac->addAction("saveas_url", mUrlSaveAsAction );
  connect(mUrlSaveAsAction, SIGNAL(triggered(bool) ), SLOT(slotUrlSave()));

  mToggleFixFontAction  = new KToggleAction(i18n("Use Fi&xed Font"), this);
  ac->addAction("toggle_fixedfont", mToggleFixFontAction );
  connect(mToggleFixFontAction, SIGNAL(triggered(bool) ), SLOT(slotToggleFixedFont()));
  mToggleFixFontAction->setShortcut(QKeySequence(Qt::Key_X));

  //
  // Scroll actions
  //
  mScrollUpAction = new KAction( i18n("Scroll Message Up"), this );
  mScrollUpAction->setShortcut( QKeySequence( Qt::Key_Up ) );
  ac->addAction( "scroll_up", mScrollUpAction );
  connect( mScrollUpAction, SIGNAL( triggered( bool ) ),
           this, SLOT( slotScrollUp() ) );

  mScrollDownAction = new KAction( i18n("Scroll Message Down"), this );
  mScrollDownAction->setShortcut( QKeySequence( Qt::Key_Down ) );
  ac->addAction( "scroll_down", mScrollDownAction );
  connect( mScrollDownAction, SIGNAL( triggered( bool ) ),
           this, SLOT( slotScrollDown() ) );

  mScrollUpMoreAction = new KAction( i18n("Scroll Message Up (More)"), this );
  mScrollUpMoreAction->setShortcut( QKeySequence( Qt::Key_PageUp ) );
  ac->addAction( "scroll_up_more", mScrollUpMoreAction );
  connect( mScrollUpMoreAction, SIGNAL( triggered( bool ) ),
           this, SLOT( slotScrollPrior() ) );

  mScrollDownMoreAction = new KAction( i18n("Scroll Message Down (More)"), this );
  mScrollDownMoreAction->setShortcut( QKeySequence( Qt::Key_PageDown ) );
  ac->addAction( "scroll_down_more", mScrollDownMoreAction );
  connect( mScrollDownMoreAction, SIGNAL( triggered( bool ) ),
           this, SLOT( slotScrollNext() ) );
}

// little helper function
KToggleAction *KMReaderWin::actionForHeaderStyle( const HeaderStyle * style, const HeaderStrategy * strategy ) {
  if ( !mActionCollection )
    return 0;
  const char * actionName = 0;
  if ( style == HeaderStyle::enterprise() )
    actionName = "view_headers_enterprise";
  if ( style == HeaderStyle::fancy() )
    actionName = "view_headers_fancy";
  else if ( style == HeaderStyle::brief() )
    actionName = "view_headers_brief";
  else if ( style == HeaderStyle::plain() ) {
    if ( strategy == HeaderStrategy::standard() )
      actionName = "view_headers_standard";
    else if ( strategy == HeaderStrategy::rich() )
      actionName = "view_headers_long";
    else if ( strategy == HeaderStrategy::all() )
      actionName = "view_headers_all";
  }
  if ( actionName )
    return static_cast<KToggleAction*>(mActionCollection->action(actionName));
  else
    return 0;
}

KToggleAction *KMReaderWin::actionForAttachmentStrategy( const AttachmentStrategy * as ) {
  if ( !mActionCollection )
    return 0;
  const char * actionName = 0;
  if ( as == AttachmentStrategy::iconic() )
    actionName = "view_attachments_as_icons";
  else if ( as == AttachmentStrategy::smart() )
    actionName = "view_attachments_smart";
  else if ( as == AttachmentStrategy::inlined() )
    actionName = "view_attachments_inline";
  else if ( as == AttachmentStrategy::hidden() )
    actionName = "view_attachments_hide";

  if ( actionName )
    return static_cast<KToggleAction*>(mActionCollection->action(actionName));
  else
    return 0;
}

void KMReaderWin::slotEnterpriseHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::enterprise(),
                             HeaderStrategy::rich() );
}

void KMReaderWin::slotFancyHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::fancy(),
                             HeaderStrategy::rich() );
}

void KMReaderWin::slotBriefHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::brief(),
                             HeaderStrategy::brief() );
}

void KMReaderWin::slotStandardHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::plain(),
                             HeaderStrategy::standard());
}

void KMReaderWin::slotLongHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::plain(),
                             HeaderStrategy::rich() );
}

void KMReaderWin::slotAllHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::plain(),
                             HeaderStrategy::all() );
}

void KMReaderWin::slotLevelQuote( int l )
{
  kDebug(5006) << "Old Level:" << mLevelQuote << "New Level:" << l;

  mLevelQuote = l;
  saveRelativePosition();
  update( true );
}

void KMReaderWin::slotCycleHeaderStyles() {
  const HeaderStrategy * strategy = headerStrategy();
  const HeaderStyle * style = headerStyle();

  const char * actionName = 0;
  if ( style == HeaderStyle::enterprise() ) {
    slotFancyHeaders();
    actionName = "view_headers_fancy";
  }
  if ( style == HeaderStyle::fancy() ) {
    slotBriefHeaders();
    actionName = "view_headers_brief";
  } else if ( style == HeaderStyle::brief() ) {
    slotStandardHeaders();
    actionName = "view_headers_standard";
  } else if ( style == HeaderStyle::plain() ) {
    if ( strategy == HeaderStrategy::standard() ) {
      slotLongHeaders();
      actionName = "view_headers_long";
    } else if ( strategy == HeaderStrategy::rich() ) {
      slotAllHeaders();
      actionName = "view_headers_all";
    } else if ( strategy == HeaderStrategy::all() ) {
      slotEnterpriseHeaders();
      actionName = "view_headers_enterprise";
    }
  }

  if ( actionName )
    static_cast<KToggleAction*>( mActionCollection->action( actionName ) )->setChecked( true );
}


void KMReaderWin::slotIconicAttachments() {
  setAttachmentStrategy( AttachmentStrategy::iconic() );
}

void KMReaderWin::slotSmartAttachments() {
  setAttachmentStrategy( AttachmentStrategy::smart() );
}

void KMReaderWin::slotInlineAttachments() {
  setAttachmentStrategy( AttachmentStrategy::inlined() );
}

void KMReaderWin::slotHideAttachments() {
  setAttachmentStrategy( AttachmentStrategy::hidden() );
}

void KMReaderWin::slotCycleAttachmentStrategy() {
  setAttachmentStrategy( attachmentStrategy()->next() );
  KToggleAction * action = actionForAttachmentStrategy( attachmentStrategy() );
  assert( action );
  action->setChecked( true );
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  delete mHtmlWriter; mHtmlWriter = 0;
  delete mCSSHelper;
  if (mAutoDelete) delete message();
  delete mRootNode; mRootNode = 0;
  removeTempFiles();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotMessageArrived( KMMessage *msg )
{
  if (msg && ((KMMsgBase*)msg)->isMessage()) {
    if ( msg->getMsgSerNum() == mWaitingForSerNum ) {
      setMsg( msg, true );
    } else {
      kDebug( 5006 ) << "Ignoring update";
    }
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::update( KMail::Interface::Observable * observable )
{
  if ( !mAtmUpdate ) {
    // reparse the msg
    kDebug(5006) << "Message";
    updateReaderWin();
    return;
  }

  if ( !mRootNode )
    return;

  KMMessage* msg = static_cast<KMMessage*>( observable );
  assert( msg != 0 );

  // find our partNode and update it
  if ( !msg->lastUpdatedPart() ) {
    kDebug(5006) << "No updated part";
    return;
  }
  partNode* node = mRootNode->findNodeForDwPart( msg->lastUpdatedPart() );
  if ( !node ) {
    kDebug(5006) << "Can't find node for part";
    return;
  }
  node->setDwPart( msg->lastUpdatedPart() );

  // update the tmp file
  // we have to set it writeable temporarily
  ::chmod( QFile::encodeName( mAtmCurrentName ), S_IRWXU );
  QByteArray data = node->msgPart().bodyDecodedBinary();
  if ( node->msgPart().type() == DwMime::kTypeText && data.size() > 0 ) {
    // convert CRLF to LF before writing text attachments to disk
    const size_t newsize = KMail::Util::crlf2lf( data.data(), data.size() );
    data.truncate( newsize );
  }
  KPIMUtils::kByteArrayToFile( data, mAtmCurrentName, false, false, false );
  ::chmod( QFile::encodeName( mAtmCurrentName ), S_IRUSR );

  mAtmUpdate = false;
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
  if (e->type() == QEvent::PaletteChange)
  {
    delete mCSSHelper;
    mCSSHelper = new KMail::CSSHelper( mViewer->view() );
    if (message())
      message()->readConfig();
    update( true ); // Force update
    return true;
  }
  return QWidget::event(e);
}


//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  const KConfigGroup mdnGroup( KMKernel::config(), "MDN" );
  /*should be: const*/ KConfigGroup reader( KMKernel::config(), "Reader" );

  delete mCSSHelper;
  mCSSHelper = new KMail::CSSHelper( mViewer->view() );

  mNoMDNsWhenEncrypted = mdnGroup.readEntry( "not-send-when-encrypted", true );

  mUseFixedFont = reader.readEntry( "useFixedFont", false );
  if ( mToggleFixFontAction )
    mToggleFixFontAction->setChecked( mUseFixedFont );

  mHtmlMail = reader.readEntry( "htmlMail", false );
  mHtmlLoadExternal = reader.readEntry( "htmlLoadExternal", false );

  KToggleAction *raction = actionForHeaderStyle( headerStyle(), headerStrategy() );
  if ( raction )
    raction->setChecked( true );

  setAttachmentStrategy( AttachmentStrategy::create( reader.readEntry( "attachment-strategy", "smart" ) ) );
  raction = actionForAttachmentStrategy( attachmentStrategy() );
  if ( raction )
    raction->setChecked( true );

  // if the user uses OpenPGP then the color bar defaults to enabled
  // else it defaults to disabled
  mShowColorbar = reader.readEntry( "showColorbar", Kpgp::Module::getKpgp()->usePGP() ) ;
  // if the value defaults to enabled and KMail (with color bar) is used for
  // the first time the config dialog doesn't know this if we don't save the
  // value now
  reader.writeEntry( "showColorbar", mShowColorbar );

  mMimeTreeAtBottom = reader.readEntry( "MimeTreeLocation", "bottom" ) != "top";
  const QString s = reader.readEntry( "MimeTreeMode", "smart" );
  if ( s == "never" )
    mMimeTreeMode = 0;
  else if ( s == "always" )
    mMimeTreeMode = 2;
  else
    mMimeTreeMode = 1;

  const int mimeH = reader.readEntry( "MimePaneHeight", 100 );
  const int messageH = reader.readEntry( "MessagePaneHeight", 180 );
  mSplitterSizes.clear();
  if ( mMimeTreeAtBottom )
    mSplitterSizes << messageH << mimeH;
  else
    mSplitterSizes << mimeH << messageH;

  adjustLayout();

  readGlobalOverrideCodec();

  // Note that this call triggers an update, see this call has to be at the
  // bottom when all settings are already est.
  setHeaderStyleAndStrategy( HeaderStyle::create( reader.readEntry( "header-style", "fancy" ) ),
                             HeaderStrategy::create( reader.readEntry( "header-set-displayed", "rich" ) ) );

  if (message())
    update();
  mColorBar->update();
  KMMessage::readConfig();
}


void KMReaderWin::adjustLayout() {
  if ( mMimeTreeAtBottom )
    mSplitter->addWidget( mMimePartTree );
  else
    mSplitter->insertWidget( 0, mMimePartTree );
  mSplitter->setSizes( mSplitterSizes );

  if ( mMimeTreeMode == 2 && mMsgDisplay )
    mMimePartTree->show();
  else
    mMimePartTree->hide();

  if ( mShowColorbar && mMsgDisplay )
    mColorBar->show();
  else
    mColorBar->hide();
}


void KMReaderWin::saveSplitterSizes( KConfigGroup & c ) const {
  if ( !mSplitter || !mMimePartTree )
    return;
  if ( mMimePartTree->isHidden() )
    return; // don't rely on QSplitter maintaining sizes for hidden widgets.

  c.writeEntry( "MimePaneHeight", mSplitter->sizes()[ mMimeTreeAtBottom ? 1 : 0 ] );
  c.writeEntry( "MessagePaneHeight", mSplitter->sizes()[ mMimeTreeAtBottom ? 0 : 1 ] );
}

//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig( bool sync ) const {
  KConfigGroup reader( KMKernel::config(), "Reader" );

  reader.writeEntry( "useFixedFont", mUseFixedFont );
  if ( headerStyle() )
    reader.writeEntry( "header-style", headerStyle()->name() );
  if ( headerStrategy() )
    reader.writeEntry( "header-set-displayed", headerStrategy()->name() );
  if ( attachmentStrategy() )
    reader.writeEntry( "attachment-strategy", attachmentStrategy()->name() );

  saveSplitterSizes( reader );

  if ( sync )
    kmkernel->slotRequestConfigSync();
}

//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  mViewer->widget()->setFocusPolicy(Qt::WheelFocus);
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->setPluginsEnabled(false);
  mViewer->setJScriptEnabled(false); // just make this explicit
  mViewer->setJavaEnabled(false);    // just make this explicit
  mViewer->setMetaRefreshEnabled(false);
  mViewer->setURLCursor( QCursor( Qt::PointingHandCursor ) );
  // Espen 2000-05-14: Getting rid of thick ugly frames
  mViewer->view()->setLineWidth(0);
  // register our own event filter for shift-click
  mViewer->view()->viewport()->installEventFilter( this );

  if ( !htmlWriter() ) {
    mPartHtmlWriter = new KHtmlPartHtmlWriter( mViewer, 0 );
#ifdef KMAIL_READER_HTML_DEBUG
    mHtmlWriter = new TeeHtmlWriter( new FileHtmlWriter( QString() ),
                                     mPartHtmlWriter );
#else
    mHtmlWriter = mPartHtmlWriter;
#endif
  }

  connect(mViewer->browserExtension(),
          SIGNAL(openUrlRequest(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),this,
          SLOT(slotUrlOpen(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)));
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),this,
          SLOT(slotUrlOpen(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)));
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotUrlPopup(const QString &, const QPoint &)));
}

void KMReaderWin::setAttachmentStrategy( const AttachmentStrategy * strategy ) {
  mAttachmentStrategy = strategy ? strategy : AttachmentStrategy::smart();
  update( true );
}

void KMReaderWin::setHeaderStyleAndStrategy( const HeaderStyle * style,
                                             const HeaderStrategy * strategy ) {
  mHeaderStyle = style ? style : HeaderStyle::fancy();
  mHeaderStrategy = strategy ? strategy : HeaderStrategy::rich();
  update( true );
}

//-----------------------------------------------------------------------------
void KMReaderWin::setOverrideEncoding( const QString & encoding )
{
  if ( encoding == mOverrideEncoding )
    return;

  mOverrideEncoding = encoding;
  if ( mSelectEncodingAction ) {
    if ( encoding.isEmpty() ) {
      mSelectEncodingAction->setCurrentItem( 0 );
    }
    else {
      QStringList encodings = mSelectEncodingAction->items();
      int i = 0;
      for ( QStringList::const_iterator it = encodings.begin(), end = encodings.end(); it != end; ++it, ++i ) {
        if ( KGlobal::charsets()->encodingForName( *it ) == encoding ) {
          mSelectEncodingAction->setCurrentItem( i );
          break;
        }
      }
      if ( i == encodings.size() ) {
        // the value of encoding is unknown => use Auto
        kWarning(5006) <<"Unknown override character encoding \"" << encoding
                       << "\". Using Auto instead.";
        mSelectEncodingAction->setCurrentItem( 0 );
        mOverrideEncoding.clear();
      }
    }
  }
  update( true );
}


void KMReaderWin::setPrintFont( const QFont& font )
{

  mCSSHelper->setPrintFont( font );
}

//-----------------------------------------------------------------------------
const QTextCodec * KMReaderWin::overrideCodec() const
{
  kDebug(5006) <<" mOverrideEncoding == '" << mOverrideEncoding <<"'";
  if ( mOverrideEncoding.isEmpty() || mOverrideEncoding == "Auto" ) // Auto
    return 0;
  else
    return KMMsgBase::codecForName( mOverrideEncoding.toLatin1() );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotSetEncoding()
{
  if ( mSelectEncodingAction->currentItem() == 0 ) // Auto
    mOverrideEncoding.clear();
  else
    mOverrideEncoding = KGlobal::charsets()->encodingForName( mSelectEncodingAction->currentText() );
  update( true );
}

//-----------------------------------------------------------------------------
void KMReaderWin::readGlobalOverrideCodec()
{
  // if the global character encoding wasn't changed then there's nothing to do
  if ( GlobalSettings::self()->overrideCharacterEncoding() == mOldGlobalOverrideEncoding )
    return;

  setOverrideEncoding( GlobalSettings::self()->overrideCharacterEncoding() );
  mOldGlobalOverrideEncoding = GlobalSettings::self()->overrideCharacterEncoding();
}

//-----------------------------------------------------------------------------
void KMReaderWin::setMsg(KMMessage* aMsg, bool force)
{
  if (aMsg)
      kDebug(5006) <<"(" << aMsg->getMsgSerNum() <<", last" << mLastSerNum <<")" << aMsg->subject()
        << aMsg->fromStrip() << ", readyToShow" << (aMsg->readyToShow());

  //Reset the level quote if the msg has changed.
  if (aMsg && aMsg->getMsgSerNum() != mLastSerNum ){
    mLevelQuote = GlobalSettings::self()->collapseQuoteLevelSpin()-1;
  }
  if ( mPrinting )
    mLevelQuote = -1;

  bool complete = true;
  if ( aMsg &&
       !aMsg->readyToShow() &&
       (aMsg->getMsgSerNum() != mLastSerNum) &&
       !aMsg->isComplete() )
    complete = false;

  // If not forced and there is aMsg and aMsg is same as mMsg then return
  if (!force && aMsg && mLastSerNum != 0 && aMsg->getMsgSerNum() == mLastSerNum)
    return;

  // (de)register as observer
  if (aMsg && message())
    message()->detach( this );
  if (aMsg)
    aMsg->attach( this );
  mAtmUpdate = false;

  // connect to the updates if we have hancy headers

  mDelayedMarkTimer.stop();

  mMessage = 0;
  if ( !aMsg ) {
    mWaitingForSerNum = 0; // otherwise it has been set
    mLastSerNum = 0;
  } else {
    mLastSerNum = aMsg->getMsgSerNum();
    // Check if the serial number can be used to find the assoc KMMessage
    // If so, keep only the serial number (and not mMessage), to avoid a dangling mMessage
    // when going to another message in the mainwindow.
    // Otherwise, keep only mMessage, this is fine for standalone KMReaderMainWins since
    // we're working on a copy of the KMMessage, which we own.
    if (message() != aMsg) {
      mMessage = aMsg;
      mLastSerNum = 0;
    }
  }

  if (aMsg) {
    aMsg->setOverrideCodec( overrideCodec() );
    aMsg->setDecodeHTML( htmlMail() );
    mLastStatus = aMsg->status();
    // FIXME: workaround to disable DND for IMAP load-on-demand
    if ( !aMsg->isComplete() )
      mViewer->setDNDEnabled( false );
    else
      mViewer->setDNDEnabled( true );
  } else {
    mLastStatus.clear();
  }

  // only display the msg if it is complete
  // otherwise we'll get flickering with progressively loaded messages
  if ( complete )
  {
    // Avoid flicker, somewhat of a cludge
    if (force) {
      // stop the timer to avoid calling updateReaderWin twice
      mUpdateReaderWinTimer.stop();
      updateReaderWin();
    }
    else if (mUpdateReaderWinTimer.isActive()) {
      mUpdateReaderWinTimer.setInterval( delay );
    } else {
      mUpdateReaderWinTimer.start( 0 );
    }
  }

  if ( aMsg && (aMsg->status().isUnread() || aMsg->status().isNew())
       && GlobalSettings::self()->delayedMarkAsRead() ) {
    if ( GlobalSettings::self()->delayedMarkTime() != 0 )
      mDelayedMarkTimer.start( GlobalSettings::self()->delayedMarkTime() * 1000 );
    else
      slotTouchMessage();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  mUpdateReaderWinTimer.stop();
  clear();
  mDelayedMarkTimer.stop();
  mLastSerNum = 0;
  mWaitingForSerNum = 0;
  mMessage = 0;
}

// enter items for the "Important changes" list here:
static const char * const kmailChanges[] = {
  ""
};
static const int numKMailChanges =
  sizeof kmailChanges / sizeof *kmailChanges;

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  ""
};
static const int numKMailNewFeatures =
  sizeof kmailNewFeatures / sizeof *kmailNewFeatures;


//-----------------------------------------------------------------------------
//static
QString KMReaderWin::newFeaturesMD5()
{
  QByteArray str;
  for ( int i = 0 ; i < numKMailChanges ; ++i )
    str += kmailChanges[i];
  for ( int i = 0 ; i < numKMailNewFeatures ; ++i )
    str += kmailNewFeatures[i];
  KMD5 md5( str );
  return md5.base64Digest();
}

//-----------------------------------------------------------------------------
void KMReaderWin::displaySplashPage( const QString &info )
{
  mMsgDisplay = false;
  adjustLayout();

  QString location = KStandardDirs::locate("data", "kmail/about/main.html");
  QString content = KPIMUtils::kFileToByteArray( location );
  content = content.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
  if ( QApplication::isRightToLeft() )
    content = content.arg( "@import \"" + KStandardDirs::locate( "data",
                           "kdeui/about/kde_infopage_rtl.css" ) +  "\";");
  else
    content = content.arg( "" );

  mViewer->begin(KUrl::fromPath( location ));

  QString fontSize = QString::number( pointsToPixel( mCSSHelper->bodyFont().pointSize() ) );
  QString appTitle = i18n("KMail");
  QString catchPhrase = ""; //not enough space for a catch phrase at default window size i18n("Part of the Kontact Suite");
  QString quickDescription = i18n("The email client for the K Desktop Environment.");
  mViewer->write(content.arg(fontSize).arg(appTitle).arg(catchPhrase).arg(quickDescription).arg(info));
  mViewer->end();
}

void KMReaderWin::displayBusyPage()
{
  QString info =
    i18n( "<h2 style='margin-top: 0px;'>Retrieving Folder Contents</h2><p>Please wait . . .</p>&nbsp;" );

  displaySplashPage( info );
}

void KMReaderWin::displayOfflinePage()
{
  QString info =
    i18n( "<h2 style='margin-top: 0px;'>Offline</h2><p>KMail is currently in offline mode. "
        "Click <a href=\"kmail:goOnline\">here</a> to go online . . .</p>&nbsp;" );

  displaySplashPage( info );
}


//-----------------------------------------------------------------------------
void KMReaderWin::displayAboutPage()
{
  KLocalizedString info =
    ki18nc("%1: KMail version; %2: help:// URL; %3: homepage URL; "
         "%4: generated list of new features; "
         "%5: First-time user text (only shown on first start); "
         "%6: generated list of important changes; "
         "--- end of comment ---",
         "<h2 style='margin-top: 0px;'>Welcome to KMail %1</h2><p>KMail is the email client for the K "
         "Desktop Environment. It is designed to be fully compatible with "
         "Internet mailing standards including MIME, SMTP, POP3 and IMAP."
         "</p>\n"
         "<ul><li>KMail has many powerful features which are described in the "
         "<a href=\"%2\">documentation</a></li>\n"
         "<li>The <a href=\"%3\">KMail homepage</A> offers information about "
         "new versions of KMail</li></ul>\n"
         "%6\n" // important changes
         "%4\n" // new features
         "%5\n" // first start info
         "<p>We hope that you will enjoy KMail.</p>\n"
         "<p>Thank you,</p>\n"
         "<p style='margin-bottom: 0px'>&nbsp; &nbsp; The KMail Team</p>")
           .subs( KMAIL_VERSION ) // KMail version
           .subs( "help:/kmail/index.html" ) // KMail help:// URL
           .subs( "http://kontact.kde.org/kmail/" ); // KMail homepage URL

  if ( ( numKMailNewFeatures > 1 ) || ( numKMailNewFeatures == 1 && strlen(kmailNewFeatures[0]) > 0 ) ) {
    QString featuresText =
      i18n("<p>Some of the new features in this release of KMail include "
           "(compared to KMail %1, which is part of KDE %2):</p>\n",
       QString("1.9"), QString("3.5")); // prior KMail and KDE version
    featuresText += "<ul>\n";
    for ( int i = 0 ; i < numKMailChanges ; i++ )
      featuresText += "<li>" + i18n( kmailNewFeatures[i] ) + "</li>\n";
    featuresText += "</ul>\n";
    info = info.subs( featuresText );
  }
  else
    info = info.subs( QString() ); // remove the place holder

  if( kmkernel->firstStart() ) {
    info = info.subs( i18n("<p>Please take a moment to fill in the KMail "
                           "configuration panel at Settings-&gt;Configure "
                           "KMail.\n"
                           "You need to create at least a default identity and "
                           "an incoming as well as outgoing mail account."
                           "</p>\n") );
  } else {
    info = info.subs( QString() ); // remove the place holder
  }

  if ( ( numKMailChanges > 1 ) || ( numKMailChanges == 1 && strlen(kmailChanges[0]) > 0 ) ) {
    QString changesText =
      i18n("<p><span style='font-size:125%; font-weight:bold;'>"
           "Important changes</span> (compared to KMail %1):</p>\n",
       QString("1.9"));
    changesText += "<ul>\n";
    for ( int i = 0 ; i < numKMailChanges ; i++ )
      changesText += i18n("<li>%1</li>\n", i18n( kmailChanges[i] ) );
    changesText += "</ul>\n";
    info = info.subs( changesText );
  }
  else
    info = info.subs( QString() ); // remove the place holder

  displaySplashPage( info.toString() );
}

void KMReaderWin::enableMsgDisplay() {
  mMsgDisplay = true;
  adjustLayout();
}


//-----------------------------------------------------------------------------

void KMReaderWin::updateReaderWin()
{
  if ( !mMsgDisplay ) {
    return;
  }

  mViewer->setOnlyLocalReferences( !htmlLoadExternal() );

  htmlWriter()->reset();

  KMFolder* folder = 0;
  if (message(&folder))
  {
    if ( mShowColorbar ) {
      mColorBar->show();
    } else {
      mColorBar->hide();
    }
    displayMessage();
  } else {
    mColorBar->hide();
    mMimePartTree->hide();
    mMimePartTree->clearAndResetSortOrder();
    htmlWriter()->begin( mCSSHelper->cssDefinitions( isFixedFont() ) );
    htmlWriter()->write( mCSSHelper->htmlHead( isFixedFont() ) + "</body></html>" );
    htmlWriter()->end();
  }

  if ( mSavedRelativePosition ) {
    QScrollArea *scrollview = mViewer->view();
    scrollview->widget()->move( 0,
      qRound( scrollview->widget()->size().height() * mSavedRelativePosition ) );
    mSavedRelativePosition = 0;
  }
}

//-----------------------------------------------------------------------------
int KMReaderWin::pointsToPixel(int pointSize) const
{
  return (pointSize * mViewer->view()->logicalDpiY() + 36) / 72;
}

//-----------------------------------------------------------------------------
void KMReaderWin::showHideMimeTree( bool isPlainTextTopLevel ) {
  if ( mMimeTreeMode == 2 ||
       ( mMimeTreeMode == 1 && !isPlainTextTopLevel ) )
    mMimePartTree->show();
  else {
    // don't rely on QSplitter maintaining sizes for hidden widgets:
    KConfigGroup reader( KMKernel::config(), "Reader" );
    saveSplitterSizes( reader );
    mMimePartTree->hide();
  }
}

void KMReaderWin::displayMessage() {
  KMMessage * msg = message();

  mMimePartTree->clearAndResetSortOrder();
  showHideMimeTree( !msg || // treat no message as "text/plain"
                    ( msg->type() == DwMime::kTypeText
                   && msg->subtype() == DwMime::kSubtypePlain ) );

  if ( !msg )
    return;

  msg->setOverrideCodec( overrideCodec() );

  htmlWriter()->begin( mCSSHelper->cssDefinitions( isFixedFont() ) );
  htmlWriter()->queue( mCSSHelper->htmlHead( isFixedFont() ) );

  if (!parent())
    setWindowTitle(msg->subject());

  removeTempFiles();

  mColorBar->setNeutralMode();

  parseMsg(msg);

  if( mColorBar->isNeutral() )
    mColorBar->setNormalMode();

  htmlWriter()->queue("</body></html>");
  htmlWriter()->flush();

  QTimer::singleShot( 1, this, SLOT(injectAttachments()) );
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
#ifndef NDEBUG
  kDebug( 5006 ) << "aMsg" << ( aMsg == message() ? "==" : "!=" ) << "aMsg )";
#endif

  KMMessagePart msgPart;

  assert(aMsg!=0);

  aMsg->setIsBeingParsed( true );

  if ( mRootNode && !mRootNode->processed() ) {
    kWarning(5006) << "The root node is not yet processed! Danger!";
    return;
  } else {
    delete mRootNode;
  }
  mRootNode = partNode::fromMessage( aMsg );
  const QByteArray mainCntTypeStr = mRootNode->typeString() + '/' + mRootNode->subTypeString();

  QString cntDesc = aMsg->subject();
  if( cntDesc.isEmpty() )
    cntDesc = i18n("( body part )");
  KIO::filesize_t cntSize = aMsg->msgSize();
  QString cntEnc;
  if( aMsg->contentTransferEncodingStr().isEmpty() )
    cntEnc = "7bit";
  else
    cntEnc = aMsg->contentTransferEncodingStr();

  // fill the MIME part tree viewer
  mRootNode->fillMimePartTree( 0, mMimePartTree, cntDesc, mainCntTypeStr,
                               cntEnc, cntSize );

  // Check if any part of this message is a v-card
  // v-cards can be either text/x-vcard or text/directory, so we need to check
  // both.
  partNode* vCardNode = mRootNode->findType( DwMime::kTypeText, DwMime::kSubtypeXVCard );
  if ( !vCardNode )
    vCardNode = mRootNode->findType( DwMime::kTypeText, DwMime::kSubtypeDirectory );

  bool hasVCard = false;
  if( vCardNode ) {
    // ### FIXME: We should only do this if the vCard belongs to the sender,
    // ### i.e. if the sender's email address is contained in the vCard.
    const QByteArray vCard = vCardNode->msgPart().bodyDecodedBinary();
    KABC::VCardConverter t;
    if ( !t.parseVCards( vCard ).isEmpty() ) {
      hasVCard = true;
      kDebug(5006) <<"FOUND A VALID VCARD";
      writeMessagePartToTempFile( &vCardNode->msgPart(), vCardNode->nodeId() );
    }
  }
  htmlWriter()->queue( writeMsgHeader( aMsg, hasVCard, true ) );

  // show message content
  ObjectTreeParser otp( this );
  otp.parseObjectTree( mRootNode );

  // store encrypted/signed status information in the KMMessage
  //  - this can only be done *after* calling parseObjectTree()
  KMMsgEncryptionState encryptionState = mRootNode->overallEncryptionState();
  KMMsgSignatureState  signatureState  = mRootNode->overallSignatureState();
  aMsg->setEncryptionState( encryptionState );
  // Don't reset the signature state to "not signed" (e.g. if one canceled the
  // decryption of a signed messages which has already been decrypted before).
  if ( signatureState != KMMsgNotSigned ||
       aMsg->signatureState() == KMMsgSignatureStateUnknown ) {
    aMsg->setSignatureState( signatureState );
  }

  bool emitReplaceMsgByUnencryptedVersion = false;
  const KConfigGroup reader( KMKernel::config(), "Reader" );
  if ( reader.readEntry( "store-displayed-messages-unencrypted", false ) ) {

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


kDebug(5006) <<"\n\n\nSpecial post-encryption handling:\n1.";
kDebug(5006) <<"(aMsg == msg) ="                      << (aMsg == message());
kDebug(5006) <<"   mLastStatus.isOfUnknownStatus() =" << mLastStatus.isOfUnknownStatus();
kDebug(5006) <<"|| mLastStatus.isNew() ="             << mLastStatus.isNew();
kDebug(5006) <<"|| mLastStatus.isUnread) ="           << mLastStatus.isUnread();
kDebug(5006) <<"(mIdOfLastViewedMessage != aMsg->msgId()) ="       << (mIdOfLastViewedMessage != aMsg->msgId());
kDebug(5006) <<"   (KMMsgFullyEncrypted == encryptionState) ="     << (KMMsgFullyEncrypted == encryptionState);
kDebug(5006) <<"|| (KMMsgPartiallyEncrypted == encryptionState) =" << (KMMsgPartiallyEncrypted == encryptionState);
         // only proceed if we were called the normal way - not by
         // double click on the message (==not running in a separate window)
  if(    (aMsg == message())
         // only proceed if this message was not saved encryptedly before
         // to make sure only *new* messages are saved in decrypted form
      && (    mLastStatus.isOfUnknownStatus()
           || mLastStatus.isNew()
           || mLastStatus.isUnread() )
         // avoid endless recursions
      && (mIdOfLastViewedMessage != aMsg->msgId())
         // only proceed if this message is (at least partially) encrypted
      && (    (KMMsgFullyEncrypted == encryptionState)
           || (KMMsgPartiallyEncrypted == encryptionState) ) ) {

    kDebug(5006) <<"Calling objectTreeToDecryptedMsg()";

    QByteArray decryptedData;
    // note: The following call may change the message's headers.
    objectTreeToDecryptedMsg( mRootNode, decryptedData, *aMsg );
    kDebug(5006) << "Resulting data:" << decryptedData;

    if( !decryptedData.isEmpty() ) {
      kDebug(5006) <<"Composing unencrypted message";
      // try this:
      aMsg->setBody( decryptedData );
      KMMessage* unencryptedMessage = new KMMessage( *aMsg );
      unencryptedMessage->setParent( 0 );
      // because this did not work:
      /*
      DwMessage dwMsg( DwString( aMsg->asString() ) );
      dwMsg.Body() = DwBody( DwString( resultString.data() ) );
      dwMsg.Body().Parse();
      KMMessage* unencryptedMessage = new KMMessage( &dwMsg );
      */
      kDebug(5006) << "Resulting message:" << unencryptedMessage->asString();
      kDebug(5006) << "Attach unencrypted message to aMsg";
      aMsg->setUnencryptedMsg( unencryptedMessage );
      emitReplaceMsgByUnencryptedVersion = true;
    }
  }
  }

  // save current main Content-Type before deleting mRootNode
  const int rootNodeCntType = mRootNode ? mRootNode->type() : DwMime::kTypeText;
  const int rootNodeCntSubtype = mRootNode ? mRootNode->subType() : DwMime::kSubtypePlain;

  // store message id to avoid endless recursions
  setIdOfLastViewedMessage( aMsg->msgId() );

  if( emitReplaceMsgByUnencryptedVersion ) {
    kDebug(5006) << "Invoce saving in decrypted form:";
    emit replaceMsgByUnencryptedVersion();
  } else {
    kDebug(5006) << "Finished parsing and displaying of message.";
    showHideMimeTree( rootNodeCntType == DwMime::kTypeText &&
                      rootNodeCntSubtype == DwMime::kSubtypePlain );
  }

  aMsg->setIsBeingParsed( false );
}


//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader(KMMessage* aMsg, bool hasVCard, bool topLevel)
{
  kFatal( !headerStyle(), 5006 )
    << "trying to writeMsgHeader() without a header style set!";
  kFatal( !headerStrategy(), 5006 )
    << "trying to writeMsgHeader() without a header strategy set!";
  QString href;
  if (hasVCard)
    href = QString("file:") + KUrl::toPercentEncoding( mTempFiles.last() );

  return headerStyle()->format( aMsg, headerStrategy(), href, mPrinting, topLevel );
}



//-----------------------------------------------------------------------------
QString KMReaderWin::writeMessagePartToTempFile( KMMessagePart* aMsgPart,
                                                 int aPartNum )
{
  // If the message part is already written to a file, no point in doing it again.
  // This function is called twice actually, once from the rendering of the attachment
  // in the body and once for the header.
  const partNode * node = mRootNode->findId( aPartNum );
  KUrl existingFileName = tempFileUrlFromPartNode( node );
  if ( !existingFileName.isEmpty() ) {
    return existingFileName.path();
  }

  QString fileName = aMsgPart->fileName();
  if( fileName.isEmpty() )
    fileName = aMsgPart->name();

  QString fname = createTempDir( QString::number( aPartNum ) );
  if ( fname.isEmpty() )
    return QString();

  // strip off a leading path
  int slashPos = fileName.lastIndexOf( '/' );
  if( -1 != slashPos )
    fileName = fileName.mid( slashPos + 1 );
  if( fileName.isEmpty() )
    fileName = "unnamed";
  fname += '/' + fileName;

  QByteArray data = aMsgPart->bodyDecodedBinary();
  if ( aMsgPart->type() == DwMime::kTypeText && data.size() > 0 ) {
    // convert CRLF to LF before writing text attachments to disk
    const size_t newsize = KMail::Util::crlf2lf( data.data(), data.size() );
    data.truncate( newsize );
  }
  if( !KPIMUtils::kByteArrayToFile( data, fname, false, false, false ) )
    return QString();

  mTempFiles.append( fname );
  // make file read-only so that nobody gets the impression that he might
  // edit attached files (cf. bug #52813)
  ::chmod( QFile::encodeName( fname ), S_IRUSR );

  return fname;
}

QString KMReaderWin::createTempDir( const QString &param )
{
  KTemporaryFile *tempFile = new KTemporaryFile();
  tempFile->setSuffix( '.' + param );
  tempFile->open();
  QString fname = tempFile->fileName();
  delete tempFile;

  if ( ::access( QFile::encodeName( fname ), W_OK ) != 0 ) {
    // Not there or not writable
    if( KDE_mkdir( QFile::encodeName( fname ), 0 ) != 0 ||
        ::chmod( QFile::encodeName( fname ), S_IRWXU ) != 0 ) {
      return QString(); //failed create
    }
  }

  assert( !fname.isNull() );

  mTempDirs.append( fname );
  return fname;
}

//-----------------------------------------------------------------------------
void KMReaderWin::showVCard( KMMessagePart * msgPart ) {
  const QByteArray vCard = msgPart->bodyDecodedBinary();

  VCardViewer *vcv = new VCardViewer(this, vCard );
  vcv->setObjectName( "vCardDialog" );
  vcv->show();
}

//-----------------------------------------------------------------------------
void KMReaderWin::printMsg( KMMessage* aMsg )
{
  disconnect( mPartHtmlWriter, SIGNAL( finished() ), this, SLOT( slotPrintMsg() ) );
  connect( mPartHtmlWriter, SIGNAL( finished() ), this, SLOT( slotPrintMsg() ) );
  setMsg( aMsg, true );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotPrintMsg()
{
  disconnect( mPartHtmlWriter, SIGNAL( finished() ), this, SLOT( slotPrintMsg() ) );
  if (!message()) return;
  mViewer->view()->print();
  deleteLater();
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl( const KUrl &aUrl )
{
  if ( aUrl.isEmpty() ) return -1;
  if ( !aUrl.isLocalFile() ) return -1;

  QString path = aUrl.path();
  uint right = path.lastIndexOf( '/' );
  uint left = path.lastIndexOf( '.', right );

  bool ok;
  int res = path.mid( left + 1, right - left - 1 ).toInt( &ok );
  return ( ok ) ? res : -1;
}


//-----------------------------------------------------------------------------
void KMReaderWin::resizeEvent( QResizeEvent * )
{
  if( !mResizeTimer.isActive() )
  {
    //
    // Combine all resize operations that are requested as long a
    // the timer runs.
    //
    mResizeTimer.start( 100 );
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDelayedResize()
{
  mSplitter->setGeometry( 0, 0, width(), height() );
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotTouchMessage()
{
  if ( !message() )
    return;

  if ( !message()->status().isNew() && !message()->status().isUnread() )
    return;

  SerNumList serNums;
  serNums.append( message()->getMsgSerNum() );
  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusRead(), serNums );
  command->start();

  // should we send an MDN?
  if ( mNoMDNsWhenEncrypted &&
       message()->encryptionState() != KMMsgNotEncrypted &&
       message()->encryptionState() != KMMsgEncryptionStateUnknown )
    return;

  KMFolder *folder = message()->parent();
  if ( folder &&
       ( folder->isOutbox() || folder->isSent() || folder->isTrash() ||
         folder->isDrafts() || folder->isTemplates() ) )
    return;

  if ( KMMessage * receipt = message()->createMDN( MDN::ManualAction,
                                                   MDN::Displayed,
                                                   true /* allow GUI */ ) )
    if ( !kmkernel->msgSender()->send( receipt ) ) // send or queue
      KMessageBox::error( this, i18n("Could not send MDN.") );
}


//-----------------------------------------------------------------------------
void KMReaderWin::closeEvent( QCloseEvent *e )
{
  QWidget::closeEvent( e );
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
  int i1 = aUrl.indexOf( showCertMan );
  if( -1 < i1 ) {
    i1 += showCertMan.length();
    int i2 = aUrl.indexOf(" ### ", i1);
    if( i1 < i2 )
    {
      displayName = aUrl.mid( i1, i2-i1 );
      i1 = i2+5;
      i2 = aUrl.indexOf(" ### ", i1);
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
  const KUrl url(aUrl);
  if ( url.protocol() == "kmail" || url.protocol() == "x-kmail"
       || (url.protocol().isEmpty() && url.path().isEmpty()) ) {
    mViewer->setDNDEnabled( false );
  } else {
    mViewer->setDNDEnabled( true );
  }

  if ( aUrl.trimmed().isEmpty() ) {
    KPIM::BroadcastStatus::instance()->reset();
    return;
  }

  mUrlClicked = url;

  const QString msg = URLHandlerManager::instance()->statusBarMessage( url, this );

  kWarning( msg.isEmpty(), 5006 ) << "Unhandled URL hover!";
  KPIM::BroadcastStatus::instance()->setTransientStatusMsg( msg );
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KUrl &aUrl, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)
{
  mUrlClicked = aUrl;

  if ( URLHandlerManager::instance()->handleClick( aUrl, this ) )
    return;

  kWarning( 5006 ) << "Unhandled URL click!";
  emit urlClicked( aUrl, Qt::LeftButton );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  const KUrl url( aUrl );
  mUrlClicked = url;

  if ( URLHandlerManager::instance()->handleContextMenuRequest( url, aPos, this ) )
    return;

  if ( message() ) {
    kWarning( 5006 ) << "Unhandled URL right-click!";
    emit popupMenu( *message(), url, aPos );
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::prepareHandleAttachment( int id, const QString& fileName )
{
  mAtmCurrent = id;
  mAtmCurrentName = fileName;
}

//-----------------------------------------------------------------------------
void KMReaderWin::showAttachmentPopup( int id, const QString & name, const QPoint &p )
{
  prepareHandleAttachment( id, name );
  KMenu *menu = new KMenu();
  QAction *action;

  QSignalMapper *attachmentMapper = new QSignalMapper( menu );
  connect( attachmentMapper, SIGNAL( mapped( int ) ),
           this, SLOT( slotHandleAttachment( int ) ) );

  action = menu->addAction(SmallIcon("document-open"),i18nc("to open", "Open"));
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Open );

  action = menu->addAction(i18n("Open With..."));
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::OpenWith );

  action = menu->addAction(i18nc("to view something", "View") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::View );

  action = menu->addAction(SmallIcon("document-save-as"),i18n("Save As...") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Save );

  action = menu->addAction(SmallIcon("edit-copy"), i18n("Copy") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Copy );

  if ( GlobalSettings::self()->allowAttachmentEditing() ) {
    action = menu->addAction(SmallIcon("document-properties"), i18n("Edit Attachment") );
    connect( action, SIGNAL(triggered()), attachmentMapper, SLOT(map()) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Edit );
  }
  if ( GlobalSettings::self()->allowAttachmentDeletion() ) {
    action = menu->addAction(SmallIcon("edit-delete"), i18n("Delete Attachment") );
    connect( action, SIGNAL(triggered()), attachmentMapper, SLOT(map()) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Delete );
  }
  if ( name.endsWith( ".xia", Qt::CaseInsensitive ) &&
       Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" ) ) {
    action = menu->addAction( i18n( "Decrypt With Chiasmus..." ) );
    connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::ChiasmusEncrypt );
  }
  action = menu->addAction(i18n("Properties") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Properties );
  menu->exec( p );
  delete menu;
}

//-----------------------------------------------------------------------------
void KMReaderWin::setStyleDependantFrameWidth()
{
  if ( !mBox )
    return;
  // set the width of the frame to a reasonable value for the current GUI style
  int frameWidth;
#if 0 // is this hack still needed with kde4?
  if( !qstrcmp( style()->metaObject()->className(), "KeramikStyle" ) )
    frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth ) - 1;
  else
#endif
    frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  if ( frameWidth < 0 )
    frameWidth = 0;
  if ( frameWidth != mBox->lineWidth() )
    mBox->setLineWidth( frameWidth );
}

//-----------------------------------------------------------------------------
void KMReaderWin::styleChange( QStyle& oldStyle )
{
  setStyleDependantFrameWidth();
  QWidget::styleChange( oldStyle );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotHandleAttachment( int choice )
{
  mAtmUpdate = true;
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if ( choice == KMHandleAttachmentCommand::Delete ) {
    slotDeleteAttachment( node );
  } else if ( choice == KMHandleAttachmentCommand::Edit ) {
    slotEditAttachment( node );
  } else if ( choice == KMHandleAttachmentCommand::Copy ) {
    if ( !node )
      return;
    QList<QUrl> urls;
    KUrl kUrl = tempFileUrlFromPartNode( node );
    QUrl url = QUrl::fromPercentEncoding( kUrl.toEncoded() );

    if ( !url.isValid() )
      return;
    urls.append( url );

    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls( urls );
    QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );
  }
  else {
    KMHandleAttachmentCommand* command = new KMHandleAttachmentCommand(
        node, message(), mAtmCurrent, mAtmCurrentName,
        KMHandleAttachmentCommand::AttachmentAction( choice ), KService::Ptr( 0 ), this );
    connect( command, SIGNAL( showAttachment( int, const QString& ) ),
        this, SLOT( slotAtmView( int, const QString& ) ) );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotFind()
{
  mViewer->findText();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotToggleFixedFont()
{
  mUseFixedFont = !mUseFixedFont;
  saveRelativePosition();
  update( true );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotCopySelectedText()
{
  QApplication::clipboard()->setText( mViewer->selectedText() );
}


//-----------------------------------------------------------------------------
void KMReaderWin::atmViewMsg(KMMessagePart* aMsgPart)
{
  assert(aMsgPart!=0);
  KMMessage* msg = new KMMessage;
  msg->fromString(aMsgPart->bodyDecoded());
  assert(msg != 0);
  msg->setMsgSerNum( 0 ); // because lookups will fail
  // some information that is needed for imap messages with LOD
  msg->setParent( message()->parent() );
  msg->setUID(message()->UID());
  msg->setReadyToShow(true);
  KMReaderMainWin *win = new KMReaderMainWin();
  win->showMsg( overrideEncoding(), msg );
  win->show();
}


void KMReaderWin::setMsgPart( partNode * node ) {
  htmlWriter()->reset();
  mColorBar->hide();
  htmlWriter()->begin( mCSSHelper->cssDefinitions( isFixedFont() ) );
  htmlWriter()->write( mCSSHelper->htmlHead( isFixedFont() ) );
  // end ###
  if ( node ) {
    ObjectTreeParser otp( this, 0, true );
    otp.parseObjectTree( node );
  }
  // ### this, too
  htmlWriter()->queue( "</body></html>" );
  htmlWriter()->flush();
}

//-----------------------------------------------------------------------------
void KMReaderWin::setMsgPart( KMMessagePart* aMsgPart, bool aHTML,
                              const QString& aFileName, const QString& pname )
{
  // Cancel scheduled updates of the reader window, as that would stop the
  // timer of the HTML writer, which would make viewing attachment not work
  // anymore as not all HTML is written to the HTML part.
  // We're updating the reader window here ourselves anyway.
  mUpdateReaderWinTimer.stop();

  KCursorSaver busy(KBusyPtr::busy());
  if (kasciistricmp(aMsgPart->typeStr(), "message")==0) {
      // if called from compose win
      KMMessage* msg = new KMMessage;
      assert(aMsgPart!=0);
      msg->fromString(aMsgPart->bodyDecoded());
      mMainWindow->setWindowTitle(msg->subject());
      setMsg(msg, true);
      setAutoDelete(true);
  } else if (kasciistricmp(aMsgPart->typeStr(), "text")==0) {
      if (kasciistricmp(aMsgPart->subtypeStr(), "x-vcard") == 0 ||
          kasciistricmp(aMsgPart->subtypeStr(), "directory") == 0) {
        showVCard( aMsgPart );
        return;
      }
      htmlWriter()->begin( mCSSHelper->cssDefinitions( isFixedFont() ) );
      htmlWriter()->queue( mCSSHelper->htmlHead( isFixedFont() ) );

      if (aHTML && (kasciistricmp(aMsgPart->subtypeStr(), "html")==0)) { // HTML
        // ### this is broken. It doesn't stip off the HTML header and footer!
        htmlWriter()->queue( aMsgPart->bodyToUnicode( overrideCodec() ) );
        mColorBar->setHtmlMode();
      } else { // plain text
        const QByteArray str = aMsgPart->bodyDecoded();
        ObjectTreeParser otp( this );
        otp.writeBodyStr( str,
                          overrideCodec() ? overrideCodec() : aMsgPart->codec(),
                          message() ? message()->from() : QString() );
      }
      htmlWriter()->queue("</body></html>");
      htmlWriter()->flush();
      mMainWindow->setWindowTitle(i18n("View Attachment: %1", pname));
  } else if (kasciistricmp(aMsgPart->typeStr(), "image")==0 ||
             (kasciistricmp(aMsgPart->typeStr(), "application")==0 &&
              kasciistricmp(aMsgPart->subtypeStr(), "postscript")==0))
  {
      if (aFileName.isEmpty()) return;  // prevent crash
      // Open the window with a size so the image fits in (if possible):
      QImageReader *iio = new QImageReader();
      iio->setFileName(aFileName);
      if( iio->canRead() ) {
          QImage img = iio->read();
          QRect desk = KGlobalSettings::desktopGeometry(mMainWindow);
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
      htmlWriter()->begin( mCSSHelper->cssDefinitions( isFixedFont() ) );
      htmlWriter()->write( mCSSHelper->htmlHead( isFixedFont() ) );
      htmlWriter()->write( "<img src=\"file:" +
                           KUrl::toPercentEncoding( aFileName ) +
                           "\" border=\"0\">\n"
                           "</body></html>\n" );
      htmlWriter()->end();
      setWindowTitle( i18n("View Attachment: %1", pname ) );
      show();
  } else {
    htmlWriter()->begin( mCSSHelper->cssDefinitions( isFixedFont() ) );
    htmlWriter()->queue( mCSSHelper->htmlHead( isFixedFont() ) );
    htmlWriter()->queue( "<pre>" );

    QString str = aMsgPart->bodyDecoded();
    // A QString cannot handle binary data. So if it's shorter than the
    // attachment, we assume the attachment is binary:
    if( str.length() < aMsgPart->decodedSize() ) {
      str.prepend( i18np("[KMail: Attachment contains binary data. Trying to show first character.]",
          "[KMail: Attachment contains binary data. Trying to show first %1 characters.]",
                               str.length()) + QChar::fromLatin1('\n') );
    }
    htmlWriter()->queue( Qt::escape( str ) );
    htmlWriter()->queue( "</pre>" );
    htmlWriter()->queue("</body></html>");
    htmlWriter()->flush();
    mMainWindow->setWindowTitle(i18n("View Attachment: %1", pname));
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmView( int id, const QString& name )
{
  partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
  if( node ) {
    mAtmCurrent = id;
    mAtmCurrentName = name;
    if ( mAtmCurrentName.isEmpty() )
      mAtmCurrentName = tempFileUrlFromPartNode( node ).path();

    KMMessagePart& msgPart = node->msgPart();
    QString pname = msgPart.fileName();
    if (pname.isEmpty()) pname=msgPart.name();
    if (pname.isEmpty()) pname=msgPart.contentDescription();
    if (pname.isEmpty()) pname="unnamed";
    // image Attachment is saved already
    if (kasciistricmp(msgPart.typeStr(), "message")==0) {
      atmViewMsg(&msgPart);
    } else if ((kasciistricmp(msgPart.typeStr(), "text")==0) &&
               ( (kasciistricmp(msgPart.subtypeStr(), "x-vcard")==0) ||
                 (kasciistricmp(msgPart.subtypeStr(), "directory")==0) )) {
      setMsgPart( &msgPart, htmlMail(), name, pname );
    } else {
      KMReaderMainWin *win = new KMReaderMainWin(&msgPart, htmlMail(),
          name, pname, overrideEncoding() );
      win->show();
    }
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::openAttachment( int id, const QString & name )
{
  mAtmCurrentName = name;
  mAtmCurrent = id;

  QString str, pname, cmd, fileName;

  partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
  if( !node ) {
    kWarning(5006) << "Could not find node" << id;
    return;
  }
  if ( mAtmCurrentName.isEmpty() )
    mAtmCurrentName = tempFileUrlFromPartNode( node ).path();

  KMMessagePart& msgPart = node->msgPart();
  if (kasciistricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  QByteArray contentTypeStr( msgPart.typeStr() + '/' + msgPart.subtypeStr() );
  kAsciiToLower( contentTypeStr.data() );

  if ( ( qstrcmp( contentTypeStr, "text/directory" ) == 0 ) ||
       ( qstrcmp( contentTypeStr, "text/x-vcard" ) == 0 ) ) {
    showVCard( &msgPart );
    return;
  }

  // determine the MIME type of the attachment
  KMimeType::Ptr mimetype;
  // prefer the value of the Content-Type header
  mimetype = KMimeType::mimeType( QString::fromLatin1( contentTypeStr ) );
  if ( mimetype.isNull() ) {
    // consider the filename if mimetype can not be found by content-type
    mimetype = KMimeType::findByPath( name, 0, true /* no disk access */ );
  }
  if ( ( mimetype->name() == "application/octet-stream" )
       && msgPart.isComplete() ) {
    // consider the attachment's contents if neither the Content-Type header
    // nor the filename give us a clue
    mimetype = KMimeType::findByFileContent( name );
  }

  KService::Ptr offer =
      KMimeTypeTrader::self()->preferredService( mimetype->name(), "Application" );

  QString open_text;
  QString filenameText = msgPart.fileName();
  if ( filenameText.isEmpty() )
    filenameText = msgPart.name();
  if ( offer ) {
    open_text = i18n("&Open with '%1'", offer->name() );
  } else {
    open_text = i18n("&Open With...");
  }
  const QString text = i18n("Open attachment '%1'?\n"
                            "Note that opening an attachment may compromise "
                            "your system's security.",
                         filenameText );
  const int choice = KMessageBox::questionYesNoCancel( this, text,
      i18n("Open Attachment?"), KStandardGuiItem::saveAs(),
      KGuiItem(open_text), KStandardGuiItem::cancel(),
      QString::fromLatin1("askSave") + mimetype->name() );

  if( choice == KMessageBox::Yes ) { // Save
    mAtmUpdate = true;
    KMHandleAttachmentCommand* command = new KMHandleAttachmentCommand( node,
        message(), mAtmCurrent, mAtmCurrentName, KMHandleAttachmentCommand::Save,
        offer, this );
    connect( command, SIGNAL( showAttachment( int, const QString& ) ),
        this, SLOT( slotAtmView( int, const QString& ) ) );
    command->start();
  }
  else if( choice == KMessageBox::No ) { // Open
    KMHandleAttachmentCommand::AttachmentAction action = ( offer ?
        KMHandleAttachmentCommand::Open : KMHandleAttachmentCommand::OpenWith );
    mAtmUpdate = true;
    KMHandleAttachmentCommand* command = new KMHandleAttachmentCommand( node,
        message(), mAtmCurrent, mAtmCurrentName, action, offer, this );
    connect( command, SIGNAL( showAttachment( int, const QString& ) ),
        this, SLOT( slotAtmView( int, const QString& ) ) );
    command->start();
  } else { // Cancel
    kDebug(5006) <<"Canceled opening attachment";
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollUp()
{
  mViewer->view()->scrollBy( 0, -10 );
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  mViewer->view()->scrollBy( 0, 10 );
}

bool KMReaderWin::atBottom() const
{
  KHTMLView *view = mViewer->view();
  return view->contentsY() + view->visibleHeight() >= view->contentsHeight();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotJumpDown()
{
  mViewer->view()->scrollBy( 0, mViewer->view()->visibleHeight() );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollPrior()
{
  mViewer->view()->scrollBy( 0, -(int)(height() * 0.8 ) );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollNext()
{
  mViewer->view()->scrollBy( 0, (int)(height() * 0.8 ) );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentChanged()
{

}

//-----------------------------------------------------------------------------
void KMReaderWin::slotTextSelected(bool)
{
  QString temp = mViewer->selectedText();
  QApplication::clipboard()->setText(temp);
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
}

//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlOverride( bool override )
{
  mHtmlOverride = override;
  if ( message() ) {
    message()->setDecodeHTML( htmlMail() );
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlLoadExtOverride( bool override )
{
  mHtmlLoadExtOverride = override;
}

//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}

//-----------------------------------------------------------------------------
bool KMReaderWin::htmlLoadExternal()
{
  return ((mHtmlLoadExternal && !mHtmlLoadExtOverride) ||
          (!mHtmlLoadExternal && mHtmlLoadExtOverride));
}

//-----------------------------------------------------------------------------
void KMReaderWin::saveRelativePosition()
{
  const QScrollArea *scrollview = mViewer->view();
  mSavedRelativePosition = static_cast<float>( scrollview->widget()->pos().y() ) /
                           scrollview->widget()->size().height();
}


//-----------------------------------------------------------------------------
void KMReaderWin::update( bool force )
{
  KMMessage *msg = message();
  if ( msg ) {
    setMsg( msg, force );
  }
}

//-----------------------------------------------------------------------------
KMMessage *KMReaderWin::message( KMFolder **aFolder ) const
{
  KMFolder*  tmpFolder;
  KMFolder*& folder = aFolder ? *aFolder : tmpFolder;
  folder = 0;
  if (mMessage)
      return mMessage;
  if (mLastSerNum) {
    KMMessage *message = 0;
    int index;
    KMMsgDict::instance()->getLocation( mLastSerNum, &folder, &index );
    if (folder )
      message = folder->getMsg( index );
    if (!message)
      kWarning(5006) <<"Attempt to reference invalid serial number" << mLastSerNum;
    return message;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlClicked()
{
  KMMainWidget *mainWidget = dynamic_cast<KMMainWidget*>(mMainWindow);
  uint identity = 0;
  if ( message() && message()->parent() ) {
    identity = message()->parent()->identity();
  }

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
  // we don't necessarily need a mainWidget for KMUrlCopyCommand so
  // it doesn't matter if the dynamic_cast fails.
  KMCommand *command =
    new KMUrlCopyCommand( mUrlClicked,
                          dynamic_cast<KMMainWidget*>( mMainWindow ) );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen( const KUrl &url )
{
  if ( !url.isEmpty() )
    mUrlClicked = url;
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
partNode * KMReaderWin::partNodeFromUrl( const KUrl & url ) {
  return mRootNode ? mRootNode->findId( msgPartFromUrl( url ) ) : 0 ;
}

partNode * KMReaderWin::partNodeForId( int id ) {
  return mRootNode ? mRootNode->findId( id ) : 0 ;
}


KUrl KMReaderWin::tempFileUrlFromPartNode( const partNode *node )
{
  if (!node)
    return KUrl();

  foreach ( const QString &path, mTempFiles ) {
    int right = path.lastIndexOf( '/' );
    int left = path.lastIndexOf( '.', right );

    bool ok = false;
    int res = path.mid( left + 1, right - left - 1 ).toInt( &ok );
    if ( ok && res == node->nodeId() )
      return KUrl( path );
  }
  return KUrl();
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
void KMReaderWin::slotSaveMsg()
{
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( mMainWindow, message() );

  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
}

//-----------------------------------------------------------------------------
bool KMReaderWin::eventFilter( QObject *, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress ) {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if ( me->button() == Qt::LeftButton && ( me->modifiers() & Qt::ShiftModifier ) ) {
      // special processing for shift+click
      mAtmCurrent = msgPartFromUrl( mUrlClicked );
      if ( mAtmCurrent < 0 ) return false; // not an attachment
      mAtmCurrentName = mUrlClicked.path();
      slotHandleAttachment( KMHandleAttachmentCommand::Save ); // save
      return true; // eat event
    }
  }
  // standard event processing
  return false;
}

void KMReaderWin::slotDeleteAttachment(partNode * node)
{
  if ( KMessageBox::warningContinueCancel( this,
       i18n("Deleting an attachment might invalidate any digital signature on this message."),
       i18n("Delete Attachment"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
       "DeleteAttachmentSignatureWarning" )
     != KMessageBox::Continue ) {
    return;
  }
  KMDeleteAttachmentCommand* command = new KMDeleteAttachmentCommand( node, message(), this );
  command->start();
}

void KMReaderWin::slotEditAttachment(partNode * node)
{
  if ( KMessageBox::warningContinueCancel( this,
        i18n("Modifying an attachment might invalidate any digital signature on this message."),
        i18n("Edit Attachment"), KGuiItem( i18n("Edit"), "document-properties" ), KStandardGuiItem::cancel(),
        "EditAttachmentSignatureWarning" )
        != KMessageBox::Continue ) {
    return;
  }
  KMEditAttachmentCommand* command = new KMEditAttachmentCommand( node, message(), this );
  command->start();
}

KMail::CSSHelper* KMReaderWin::cssHelper() const
{
  return mCSSHelper;
}

bool KMReaderWin::decryptMessage() const
{
  if ( !GlobalSettings::self()->alwaysDecrypt() )
    return mDecrytMessageOverwrite;
  return true;
}

void KMReaderWin::injectAttachments()
{
  // inject attachments in header view
  // we have to do that after the otp has run so we also see encrypted parts
  DOM::Document doc = mViewer->htmlDocument();
  DOM::Element injectionPoint = doc.getElementById( "attachmentInjectionPoint" );
  if ( injectionPoint.isNull() )
    return;

  QString imgpath( KStandardDirs::locate("data","kmail/pics/") );
  QString visibility;
  QString urlHandle;
  QString imgSrc;
  if( !showAttachmentQuicklist() ) {
    urlHandle.append( "kmail:showAttachmentQuicklist" );
    imgSrc.append( "attachmentQuicklistClosed.png" );
  } else {
    urlHandle.append( "kmail:hideAttachmentQuicklist" );
    imgSrc.append( "attachmentQuicklistOpened.png" );
  }

  QColor background = KColorScheme( QPalette::Active, KColorScheme::View ).background().color();
  QString html = renderAttachments( mRootNode, background );
  if ( html.isEmpty() )
    return;

  QString link("");
  if ( headerStyle() == HeaderStyle::fancy() ) {
    link += "<div style=\"text-align: left;\"><a href=\""+urlHandle+"\"><img src=\""+imgpath+imgSrc+"\"/></a></div>";
    html.prepend( link );
    html.prepend( QString::fromLatin1("<div style=\"float:left;\">%1&nbsp;</div>" ).arg(i18n("Attachments:")) );
  } else {
    link += "<div style=\"text-align: right;\"><a href=\""+urlHandle+"\"><img src=\""+imgpath+imgSrc+"\"/></a></div>";
    html.prepend( link );
  }

  assert( injectionPoint.tagName() == "div" );
  static_cast<DOM::HTMLElement>( injectionPoint ).setInnerHTML( html );
}

static QColor nextColor( const QColor & c )
{
  int h, s, v;
  c.getHsv( &h, &s, &v );
  return QColor::fromHsv( (h + 50) % 360, QMAX(s, 64), v );
}

QString KMReaderWin::renderAttachments(partNode * node, const QColor &bgColor )
{
  if ( !node )
    return QString();

  QString html;
  if ( node->firstChild() ) {
    QString subHtml = renderAttachments( node->firstChild(), nextColor( bgColor ) );
    if ( !subHtml.isEmpty() ) {

      QString visibility;
      if( !showAttachmentQuicklist() ) {
        visibility.append( "display:none;" );
      }

      QString margin;
      if ( node != mRootNode || headerStyle() != HeaderStyle::enterprise() )
        margin = "padding:2px; margin:2px; ";
      QString align = "left";
      if ( headerStyle() == HeaderStyle::enterprise() )
        align = "right";
      if ( node->msgPart().typeStr() == "message" || node == mRootNode )
        html += QString::fromLatin1("<div style=\"background:%1; %2"
                "vertical-align:middle; float:%3; %4\">").arg( bgColor.name() ).arg( margin )
                                                         .arg( align ).arg( visibility );
      html += subHtml;
      if ( node->msgPart().typeStr() == "message" || node == mRootNode )
        html += "</div>";
    }
  } else {
    QString label, icon;
    icon = node->msgPart().iconName( KIconLoader::Small );
    label = node->msgPart().contentDescription();
    if( label.isEmpty() )
      label = node->msgPart().name().trimmed();
    if( label.isEmpty() )
      label = node->msgPart().fileName();
    bool typeBlacklisted = node->msgPart().typeStr() == "multipart";
    if ( !typeBlacklisted && node->msgPart().typeStr() == "application" ) {
      typeBlacklisted = node->msgPart().subtypeStr() == "pgp-encrypted"
                     || node->msgPart().subtypeStr() == "pgp-signature"
                     || node->msgPart().subtypeStr() == "pkcs7-mime"
                     || node->msgPart().subtypeStr() == "pkcs7-signature";
    }
    typeBlacklisted = typeBlacklisted || node == mRootNode;
    if ( !label.isEmpty() && !icon.isEmpty() && !typeBlacklisted ) {
      html += "<div style=\"float:left;\">";
      html += QString::fromLatin1( "<span style=\"white-space:nowrap; border-width: 0px; border-left-width: 5px; border-color: %1; 2px; border-left-style: solid;\">" ).arg( bgColor.name() );
      QString fileName = writeMessagePartToTempFile( &node->msgPart(), node->nodeId() );
      QString href = "file:" + KUrl::toPercentEncoding( fileName ) ;
      html += QString::fromLatin1( "<a href=\"" ) + href +
              QString::fromLatin1( "\">" );
      html += "<img style=\"vertical-align:middle;\" src=\"" + icon + "\"/>&nbsp;";
      if ( headerStyle() == HeaderStyle::enterprise() ) {
        QFont bodyFont = mCSSHelper->bodyFont( isFixedFont() );
        QFontMetrics fm( bodyFont );
        html += fm.elidedText( label, Qt::ElideRight, 180 );
      } else {
        html += label;
      }
      html += "</a></span></div> ";
    }
  }

  html += renderAttachments( node->nextSibling(), nextColor ( bgColor ) );
  return html;
}

#include "kmreaderwin.moc"


