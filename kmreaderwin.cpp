/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2009 Laurent Montel <montel@kde.org>

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
#include "kmreaderwin.h"

#include <config-kmail.h>

#include "globalsettings.h"
#include "kmversion.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include <kpimutils/kfileio.h>
#include "kmfolderindex.h"
#include "kmcommands.h"
#include <QByteArray>
#include <QImageReader>
#include <QCloseEvent>
#include <QEvent>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalMapper>
#include "partNode.h"
#include "kmmsgdict.h"
#include "messagesender.h"
#include "libmessageviewer/kcursorsaver.h"
#include "kmfolder.h"
#ifndef USE_AKONADI_VIEWER
#include "libmessageviewer/vcardviewer.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;
#endif
#include "libmessageviewer/headerstrategy.h"
#include "libmessageviewer/headerstyle.h"
#include "libmessageviewer/khtmlparthtmlwriter.h"
using MessageViewer::HtmlWriter;
#include "libmessageviewer/htmlstatusbar.h"
#include "folderjob.h"
using KMail::FolderJob;

#include "libmessageviewer/csshelper.h"
using MessageViewer::CSSHelper;
#include "isubject.h"
using KMail::ISubject;
#include "urlhandlermanager.h"
using KMail::URLHandlerManager;
#include "interfaces/observable.h"
#include "util.h"
#include <kicon.h>
#include "broadcaststatus.h"
#include "libmessageviewer/attachmentdialog.h"
#include "stringutil.h"

#include <kmime/kmime_mdn.h>
using namespace KMime;
#ifndef USE_AKONADI_VIEWER
#ifdef KMAIL_READER_HTML_DEBUG
#include "libmessageviewer/filehtmlwriter.h"
using KMail::FileHtmlWriter;
#include "libmessageviewer/teehtmlwriter.h"
using KMail::TeeHtmlWriter;
#endif
#endif

#ifdef USE_AKONADI_VIEWER
#include "libmessageviewer/viewer.h"
#endif
using namespace MessageViewer;
#include "libmessageviewer/attachmentstrategy.h"

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

using namespace KMail;

// This function returns the complete data that were in this
// message parts - *after* all encryption has been removed that
// could be removed.
// - This is used to store the message in decrypted form.
#ifndef USE_AKONADI_VIEWER
void KMReaderWin::objectTreeToDecryptedMsg( partNode* node,
                                            QByteArray& resultingData,
                                            KMMessage& theMessage,
                                            bool weAreReplacingTheRootNode,
                                            int recCount )
{
  kDebug() << "-------------------------------------------------";
  kDebug() << "START" << "(" << recCount << ")";
  if( node ) {
    partNode* curNode = node;
    partNode* dataNode = curNode;
    partNode * child = node->firstChild();
    bool bIsMultipart = false;

    switch( curNode->type() ){
      case DwMime::kTypeText: {
          kDebug() << "* text *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeHtml:
            kDebug() << "html";
            break;
          case DwMime::kSubtypeXVCard:
            kDebug() << "v-card";
            break;
          case DwMime::kSubtypeRichtext:
            kDebug() << "rich text";
            break;
          case DwMime::kSubtypeEnriched:
            kDebug() << "enriched";
            break;
          case DwMime::kSubtypePlain:
            kDebug() << "plain";
            break;
          default:
            kDebug() << "default";
            break;
          }
        }
        break;
      case DwMime::kTypeMultipart: {
          kDebug() << "* multipart *";
          bIsMultipart = true;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMixed:
            kDebug() << "mixed";
            break;
          case DwMime::kSubtypeAlternative:
            kDebug() << "alternative";
            break;
          case DwMime::kSubtypeDigest:
            kDebug() << "digest";
            break;
          case DwMime::kSubtypeParallel:
            kDebug() << "parallel";
            break;
          case DwMime::kSubtypeSigned:
            kDebug() << "signed";
            break;
          case DwMime::kSubtypeEncrypted: {
              kDebug() << "encrypted";
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
            kDebug() << "(  unknown subtype  )";
            break;
          }
        }
        break;
      case DwMime::kTypeMessage: {
          kDebug() << "* message *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeRfc822: {
              kDebug() << "RfC 822";
              if ( child )
                dataNode = child;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeApplication: {
          kDebug() << "* application *";
          switch( curNode->subType() ){
          case DwMime::kSubtypePostscript:
            kDebug() << "postscript";
            break;
          case DwMime::kSubtypeOctetStream: {
              kDebug() << "octet stream";
              if ( child )
                dataNode = child;
            }
            break;
          case DwMime::kSubtypePgpEncrypted:
            kDebug() << "pgp encrypted";
            break;
          case DwMime::kSubtypePgpSignature:
            kDebug() << "pgp signed";
            break;
          case DwMime::kSubtypePkcs7Mime: {
              kDebug() << "pkcs7 mime";
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
          kDebug() << "* image *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeJpeg:
            kDebug() << "JPEG";
            break;
          case DwMime::kSubtypeGif:
            kDebug() << "GIF";
            break;
          }
        }
        break;
      case DwMime::kTypeAudio: {
          kDebug() << "* audio *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeBasic:
            kDebug() << "basic";
            break;
          }
        }
        break;
      case DwMime::kTypeVideo: {
          kDebug() << "* video *";
          switch( curNode->subType() ){
          case DwMime::kSubtypeMpeg:
            kDebug() << "mpeg";
            break;
          }
        }
        break;
      case DwMime::kTypeModel:
          kDebug() << "* model *";
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
      kDebug() << "dataNode == curNode:  Save curNode without replacing it.";

      // A) Store the headers of this part IF curNode is not the root node
      //    AND we are not replacing a node that already *has* replaced
      //    the root node in previous recursion steps of this function...
      if( headers ) {
        if( dataNode->parentNode() && !weAreReplacingTheRootNode ) {
          kDebug() << "dataNode is NOT replacing the root node:  Store the headers.";
          resultingData += headers->AsString().c_str();
        } else if( weAreReplacingTheRootNode && part && part->hasHeaders() ){
          kDebug() << "dataNode replace the root node:  Do NOT store the headers but change";
          kDebug() << "                                the Message's headers accordingly.";
          kDebug() << "             old Content-Type =" << rootHeaders.ContentType().AsString().c_str();
          kDebug() << "             new Content-Type =" << headers->ContentType(   ).AsString().c_str();
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
        kDebug() << "is valid Multipart, processing children:";
        QByteArray boundary = headers->ContentType().Boundary().c_str();
        curNode = dataNode->firstChild();
        // store children of multipart
        while( curNode ) {
          kDebug() << "--boundary";
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
        kDebug() << "--boundary--";
        resultingData += "\n--";
        resultingData += boundary;
        resultingData += "--\n\n";
        kDebug() << "Multipart processing children - DONE";
      } else if( part ){
        // store simple part
        kDebug() << "is Simple part or invalid Multipart, storing body data .. DONE";
        resultingData += part->Body().AsString().c_str();
      }
    } else {
      kDebug() << "dataNode != curNode:  Replace curNode by dataNode.";
      bool rootNodeReplaceFlag = weAreReplacingTheRootNode || !curNode->parentNode();
      if( rootNodeReplaceFlag ) {
        kDebug() << "                     Root node will be replaced.";
      } else {
        kDebug() << "                     Root node will NOT be replaced.";
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
  kDebug() << "END" << "(" << recCount << ")";
}
#endif

/*
 ===========================================================================


        E N D    O F     T E M P O R A R Y     M I M E     C O D E


 ===========================================================================
*/










#ifndef USE_AKONADI_VIEWER
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
  // Remove the shortcut for the selectAll action from khtml part. It's redefined to
  // CTRL-SHIFT-A in kmail and clashes with kmails CTRL-A action.
  KAction *selectAll = qobject_cast<KAction*>(
          mViewer->actionCollection()->action( "selectAll" ) );
  if ( selectAll ) {
    selectAll->setShortcut( KShortcut() );
  } else {
    kDebug() << "Failed to find khtml's selectAll action to remove it's shortcut";
  }
  // Remove the shortcut for the find action from khtml part. It's redefined to
  // CTRL-F in kmail and clashes with khtml CTRL-F action.
  KAction *afind = qobject_cast<KAction*>(
          mViewer->actionCollection()->action( "find" ) );
  if ( afind ) {
    afind->setShortcut( KShortcut() );
  } else {
    kDebug() << "Failed to find khtml's find action to remove it's shortcut";
  }

  mSplitter->setStretchFactor( mSplitter->indexOf(mMimePartTree), 0 );
  mSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
}

const int KMReaderWin::delay = 150;
#endif

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent,
                         QWidget *mainWindow,
                         KActionCollection* actionCollection,
                         Qt::WindowFlags aFlags )
  : QWidget(aParent, aFlags ),
    mSerNumOfOriginalMessage( 0 ),
    mNodeIdOffset( -1 ),
    mDelayedMarkTimer( 0 ),
    mRootNode( 0 ),
    mMainWindow( mainWindow ),
    mActionCollection( actionCollection ),
    mMailToComposeAction( 0 ),
    mMailToReplyAction( 0 ),
    mMailToForwardAction( 0 ),
    mAddAddrBookAction( 0 ),
    mOpenAddrBookAction( 0 ),
    mUrlSaveAsAction( 0 ),
    mAddBookmarksAction( 0 ),
    mCanStartDrag( false ),
    mShowFullToAddressList( false ),
    mShowFullCcAddressList( false )
{
  mDelayedMarkTimer.setObjectName( "mDelayedMarkTimer" );
  mAutoDelete = false;
  mLastSerNum = 0;
  mWaitingForSerNum = 0;
  mMessage = 0;
  mAtmUpdate = false;
  createActions();
  QVBoxLayout * vlay = new QVBoxLayout( this );
  vlay->setMargin( 0 );
  mViewer = new Viewer( this/*TODO*/,KGlobal::config(),mMainWindow,mActionCollection );
  vlay->addWidget( mViewer );
  readConfig();

  mDelayedMarkTimer.setSingleShot( true );
  connect( &mDelayedMarkTimer, SIGNAL(timeout()),
           this, SLOT(slotTouchMessage()) );
  setMsg( 0, false );
}

void KMReaderWin::createActions()
{
  KActionCollection *ac = mActionCollection;
  if ( !ac ) {
    return;
  }
  //
  // Message Menu
  //

  // new message to
  mMailToComposeAction = new KAction( KIcon( "mail-message-new" ),
                                      i18n( "New Message To..." ), this );
  ac->addAction("mail_new", mMailToComposeAction );
  connect( mMailToComposeAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoCompose()) );

  // reply to
  mMailToReplyAction = new KAction( KIcon( "mail-reply-sender" ),
                                    i18n( "Reply To..." ), this );
  ac->addAction( "mailto_reply", mMailToReplyAction );
  connect( mMailToReplyAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoReply()) );

  // forward to
  mMailToForwardAction = new KAction( KIcon( "mail-forward" ),
                                      i18n( "Forward To..." ), this );
  ac->addAction( "mailto_forward", mMailToForwardAction );
  connect( mMailToForwardAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoForward()) );

  // add to addressbook
  mAddAddrBookAction = new KAction( KIcon( "contact-new" ),
                                    i18n( "Add to Address Book" ), this );
  ac->addAction( "add_addr_book", mAddAddrBookAction );
  connect( mAddAddrBookAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoAddAddrBook()) );

  // open in addressbook
  mOpenAddrBookAction = new KAction( KIcon( "view-pim-contacts" ),
                                     i18n( "Open in Address Book" ), this );
  ac->addAction( "openin_addr_book", mOpenAddrBookAction );
  connect( mOpenAddrBookAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoOpenAddrBook()) );
#ifndef USE_AKONADI_VIEWER
  // copy selected text to clipboard
  mCopyAction = ac->addAction( KStandardAction::Copy, "kmail_copy", this,
                               SLOT(slotCopySelectedText()) );
  // copy all text to clipboard
  mSelectAllAction  = new KAction(i18n("Select All Text"), this);
  ac->addAction("mark_all_text", mSelectAllAction );
  connect(mSelectAllAction, SIGNAL(triggered(bool) ), SLOT(selectAll()));
  mSelectAllAction->setShortcut( KStandardShortcut::selectAll() );
  // copy Email address to clipboard
  mCopyURLAction = new KAction( KIcon( "edit-copy" ),
                                i18n( "Copy Link Address" ), this );
  ac->addAction( "copy_url", mCopyURLAction );
  connect( mCopyURLAction, SIGNAL(triggered(bool)), SLOT(slotUrlCopy()) );
  // find text
  KAction *action = new KAction(KIcon("edit-find"), i18n("&Find in Message..."), this);
  ac->addAction("find_in_messages", action );
  connect(action, SIGNAL(triggered(bool)), SLOT(slotFind()));
  // open URL
  mUrlOpenAction = new KAction( KIcon( "document-open" ), i18n( "Open URL" ), this );
  ac->addAction( "open_url", mUrlOpenAction );
  connect( mUrlOpenAction, SIGNAL(triggered(bool)), SLOT(slotUrlOpen()) );
#endif
  // bookmark message
  mAddBookmarksAction = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark This Link" ), this );
  ac->addAction( "add_bookmarks", mAddBookmarksAction );
  connect( mAddBookmarksAction, SIGNAL(triggered(bool)),
           SLOT(slotAddBookmarks()) );

  // save URL as
  mUrlSaveAsAction = new KAction( i18n( "Save Link As..." ), this );
  ac->addAction( "saveas_url", mUrlSaveAsAction );
  connect( mUrlSaveAsAction, SIGNAL(triggered(bool)), SLOT(slotUrlSave()) );
}

void KMReaderWin::setUseFixedFont( bool useFixedFont )
{
  mViewer->setUseFixedFont( useFixedFont );
}

bool KMReaderWin::isFixedFont() const
{
  return mViewer->isFixedFont();
}

//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  clearBodyPartMementos();
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
      kDebug() << "Ignoring update";
    }
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::update( KMail::Interface::Observable * observable )
{
  if ( !mAtmUpdate ) {
    // reparse the msg
    saveRelativePosition();
    updateReaderWin();
    return;
  }

  if ( !mRootNode )
    return;

  KMMessage* msg = static_cast<KMMessage*>( observable );
  assert( msg != 0 );

  // find our partNode and update it
  if ( !msg->lastUpdatedPart() ) {
    kDebug() << "No updated part";
    return;
  }
  partNode* node = mRootNode->findNodeForDwPart( msg->lastUpdatedPart() );
  if ( !node ) {
    kDebug() << "Can't find node for part";
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
    ++it)
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
void KMReaderWin::readConfig(void)
{
#ifndef USE_AKONADI_VIEWER
  delete mCSSHelper;
  mCSSHelper = new CSSHelper( mViewer->view() );

  mNoMDNsWhenEncrypted = GlobalSettings::self()->notSendWhenEncrypted();

  mUseFixedFont = GlobalSettings::self()->useFixedFont();
  if ( mToggleFixFontAction )
    mToggleFixFontAction->setChecked( mUseFixedFont );

  mHtmlMail = GlobalSettings::self()->htmlMail();
  mHtmlLoadExternal = GlobalSettings::self()->htmlLoadExternal();

  KToggleAction *raction = actionForHeaderStyle( headerStyle(), headerStrategy() );
  if ( raction )
    raction->setChecked( true );

  setAttachmentStrategy( AttachmentStrategy::create( GlobalSettings::self()->attachmentStrategy() ) );
  raction = actionForAttachmentStrategy( attachmentStrategy() );
  if ( raction )
    raction->setChecked( true );

  const int mimeH = GlobalSettings::self()->mimePaneHeight();
  const int messageH = GlobalSettings::self()->messagePaneHeight();
  mSplitterSizes.clear();
  if ( GlobalSettings::self()->mimeTreeLocation() == GlobalSettings::EnumMimeTreeLocation::bottom )
    mSplitterSizes << messageH << mimeH;
  else
    mSplitterSizes << mimeH << messageH;

  adjustLayout();

  readGlobalOverrideCodec();

  // Note that this call triggers an update, see this call has to be at the
  // bottom when all settings are already est.
  setHeaderStyleAndStrategy( HeaderStyle::create( GlobalSettings::self()->headerStyle() ),
                             HeaderStrategy::create( GlobalSettings::self()->headerSetDisplayed() ) );

  if (message())
    update();
  mColorBar->update();
  KMMessage::readConfig();
#endif
}

#ifndef USE_AKONADI_VIEWER
void KMReaderWin::adjustLayout() {
  if ( GlobalSettings::self()->mimeTreeLocation() == GlobalSettings::EnumMimeTreeLocation::bottom )
    mSplitter->addWidget( mMimePartTree );
  else
    mSplitter->insertWidget( 0, mMimePartTree );
  mSplitter->setSizes( mSplitterSizes );

  if ( GlobalSettings::self()->mimeTreeMode() == GlobalSettings::EnumMimeTreeMode::Always &&
       mMsgDisplay )
    mMimePartTree->show();
  else
    mMimePartTree->hide();

  if ( GlobalSettings::self()->showColorbar() && mMsgDisplay )
    mColorBar->show();
  else
    mColorBar->hide();
}
void KMReaderWin::saveSplitterSizes() const {
  if ( !mSplitter || !mMimePartTree )
    return;
  if ( mMimePartTree->isHidden() )
    return; // don't rely on QSplitter maintaining sizes for hidden widgets.

  const bool mimeTreeAtBottom = GlobalSettings::self()->mimeTreeLocation() == GlobalSettings::EnumMimeTreeLocation::bottom;
  GlobalSettings::self()->setMimePaneHeight( mSplitter->sizes()[ mimeTreeAtBottom ? 1 : 0 ] );
  GlobalSettings::self()->setMessagePaneHeight( mSplitter->sizes()[ mimeTreeAtBottom ? 0 : 1 ] );
}

//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig( bool sync ) const {
  GlobalSettings::self()->setUseFixedFont( mUseFixedFont );
  if ( headerStyle() )
    GlobalSettings::self()->setHeaderStyle( headerStyle()->name() );
  if ( headerStrategy() )
    GlobalSettings::self()->setHeaderSetDisplayed( headerStrategy()->name() );
  if ( attachmentStrategy() )
    GlobalSettings::self()->setAttachmentStrategy( attachmentStrategy()->name() );

  saveSplitterSizes();

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
  mViewer->view()->widget()->installEventFilter( this );

  if ( !htmlWriter() ) {
    mPartHtmlWriter = new KHtmlPartHtmlWriter( mViewer, 0 );
#ifdef KMAIL_READER_HTML_DEBUG
    mHtmlWriter = new TeeHtmlWriter( new FileHtmlWriter( QString() ),
                                     mPartHtmlWriter );
#else
    mHtmlWriter = mPartHtmlWriter;
#endif
  }

  // We do a queued connection below, and for that we need to register the meta types of the
  // parameters.
  //
  // Why do we do a queued connection instead of a direct one? slotUrlOpen() handles those clicks,
  // and can end up in the click handler for accepting invitations. That handler can pop up a dialog
  // asking the user for a comment on the invitation reply. This dialog is started with exec(), i.e.
  // executes a sub-eventloop. This sub-eventloop then eventually re-enters the KHTML event handler,
  // which then thinks we started a drag, and therefore adds a silly drag object to the cursor, with
  // urls like x-kmail-whatever/43/8/accept, and we don't want that drag object.
  //
  // Therefore, use queued connections to avoid the reentry of the KHTML event loop, so we don't
  // get the drag object.
  static bool metaTypesRegistered = false;
  if ( !metaTypesRegistered ) {
    qRegisterMetaType<KParts::OpenUrlArguments>( "KParts::OpenUrlArguments" );
    qRegisterMetaType<KParts::BrowserArguments>( "KParts::BrowserArguments" );
    metaTypesRegistered = true;
  }

  connect(mViewer->browserExtension(),
          SIGNAL(openUrlRequest(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),this,
          SLOT(slotUrlOpen(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),
          Qt::QueuedConnection);
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),this,
          SLOT(slotUrlOpen(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),
          Qt::QueuedConnection);
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotUrlPopup(const QString &, const QPoint &)));
}
#endif

void KMReaderWin::setAttachmentStrategy( const AttachmentStrategy * strategy ) {
  mViewer->setAttachmentStrategy( strategy );
}

void KMReaderWin::setHeaderStyleAndStrategy( const HeaderStyle * style,
                                             const HeaderStrategy * strategy ) {
  mViewer->setHeaderStyleAndStrategy( style, strategy );
}
//-----------------------------------------------------------------------------
void KMReaderWin::setOverrideEncoding( const QString & encoding )
{
#ifndef USE_AKONADI_VIEWER
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
      for ( QStringList::const_iterator it = encodings.constBegin(), end = encodings.constEnd(); it != end; ++it, ++i ) {
        if ( KMMsgBase::encodingForName( *it ) == encoding ) {
          mSelectEncodingAction->setCurrentItem( i );
          break;
        }
      }
      if ( i == encodings.size() ) {
        // the value of encoding is unknown => use Auto
        kWarning() <<"Unknown override character encoding \"" << encoding
                       << "\". Using Auto instead.";
        mSelectEncodingAction->setCurrentItem( 0 );
        mOverrideEncoding.clear();
      }
    }
  }
  update( true );
#else
  mViewer->setOverrideEncoding( encoding );
#endif
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
const QTextCodec * KMReaderWin::overrideCodec() const
{
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
    mOverrideEncoding = KMMsgBase::encodingForName( mSelectEncodingAction->currentText() );
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
#endif
//-----------------------------------------------------------------------------
void KMReaderWin::setOriginalMsg( unsigned long serNumOfOriginalMessage, int nodeIdOffset )
{
  mSerNumOfOriginalMessage = serNumOfOriginalMessage;
  mNodeIdOffset = nodeIdOffset;
}

//-----------------------------------------------------------------------------
void KMReaderWin::setMsg( KMMessage* aMsg, bool force )
{
  if ( aMsg ) {
    kDebug() << "(" << aMsg->getMsgSerNum() <<", last" << mLastSerNum <<")" << aMsg->subject()
             << aMsg->fromStrip() << ", readyToShow" << (aMsg->readyToShow());
  }
#ifdef USE_AKONADI_VIEWER
  //TEMPORARY
  if ( aMsg ) {
    KMime::Message *message = new KMime::Message;
    message->setContent( aMsg->asString() );
    message->parse();
    mViewer->setMessage( message , force ? Viewer::Force : Viewer::Delayed);
    //return;
  }
#endif

#ifndef USE_AKONADI_VIEWER
  // Reset message-transient state
  if (aMsg && aMsg->getMsgSerNum() != mLastSerNum ){
    mLevelQuote = GlobalSettings::self()->collapseQuoteLevelSpin()-1;
    clearBodyPartMementos();
  }
  if ( mPrinting )
    mLevelQuote = -1;
#endif
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
#ifndef USE_AKONADI_VIEWER
    aMsg->setOverrideCodec( overrideCodec() );
#endif
    aMsg->setDecodeHTML( htmlMail() );
#ifndef USE_AKONADI_VIEWER
    mLastStatus = aMsg->status();
#endif
    // FIXME: workaround to disable DND for IMAP load-on-demand
#ifndef USE_AKONADI_VIEWER
    if ( !aMsg->isComplete() )
      mViewer->setDNDEnabled( false );
    else
      mViewer->setDNDEnabled( true );
#endif
  } else {
#ifndef USE_AKONADI_VIEWER
    mLastStatus.clear();
#endif
  }

  // only display the msg if it is complete
  // otherwise we'll get flickering with progressively loaded messages
  if ( complete )
  {
    // Avoid flicker, somewhat of a cludge
    if (force) {
      // stop the timer to avoid calling updateReaderWin twice
#ifndef USE_AKONADI_VIEWER
      mUpdateReaderWinTimer.stop();
#endif
      updateReaderWin();
    }
#ifndef USE_AKONADI_VIEWER
    else if (mUpdateReaderWinTimer.isActive()) {
      mUpdateReaderWinTimer.setInterval( delay );
    } else {
      mUpdateReaderWinTimer.start( 0 );
    }
#endif
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
#ifndef USE_AKONADI_VIEWER
  mUpdateReaderWinTimer.stop();
#endif
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
  mViewer->displaySplashPage( info );
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
#ifndef USE_AKONADI_VIEWER
void KMReaderWin::enableMsgDisplay() {
  mMsgDisplay = true;
  adjustLayout();
}
#endif

//-----------------------------------------------------------------------------

void KMReaderWin::updateReaderWin()
{
  //TODO remove this function
#ifndef USE_AKONADI_VIEWER
  if ( !mMsgDisplay ) {
    return;
  }
  mViewer->setOnlyLocalReferences( !htmlLoadExternal() );
  htmlWriter()->reset();
  KMFolder* folder = 0;
  if (message(&folder))
  {
    if ( GlobalSettings::self()->showColorbar() ) {
      mColorBar->show();
    } else {
      mColorBar->hide();
    }
    displayMessage();
  } else {
    mColorBar->hide();
    mMimePartTree->hide();
    mMimePartTree->clearAndResetSortOrder();
    htmlWriter()->begin( cssHelper()->cssDefinitions( isFixedFont() ) );
    htmlWriter()->write( cssHelper()->htmlHead( isFixedFont() ) + "</body></html>" );
    htmlWriter()->end();
  }
  if ( mSavedRelativePosition ) {
    QScrollBar *scrollBar = mViewer->view()->verticalScrollBar();
    scrollBar->setValue( scrollBar->maximum() * mSavedRelativePosition );
    mSavedRelativePosition = 0;
  }
#endif
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
int KMReaderWin::pointsToPixel(int pointSize) const
{
  return (pointSize * mViewer->view()->logicalDpiY() + 36) / 72;
}

//-----------------------------------------------------------------------------
void KMReaderWin::showHideMimeTree() {
  if ( GlobalSettings::self()->mimeTreeMode() == GlobalSettings::EnumMimeTreeMode::Always )
    mMimePartTree->show();
  else {
    // don't rely on QSplitter maintaining sizes for hidden widgets:
    saveSplitterSizes();
    mMimePartTree->hide();
  }
  if ( mToggleMimePartTreeAction && ( mToggleMimePartTreeAction->isChecked() != mMimePartTree->isVisible() ) )
    mToggleMimePartTreeAction->setChecked( mMimePartTree->isVisible() );
}

void KMReaderWin::displayMessage() {
  KMMessage * msg = message();

  mMimePartTree->clearAndResetSortOrder();
  showHideMimeTree();

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

  // For some reason the DOM is not complete at this moment.
  // But if the below functions are called using a singleShot timer
  // everything just seems to work.
  QTimer::singleShot( 1, this, SLOT( toggleFullAddressList() ) );
  QTimer::singleShot( 1, this, SLOT(injectAttachments()) );
}

//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
  KMMessagePart msgPart;

  assert(aMsg!=0);

  aMsg->setIsBeingParsed( true );

  if ( mRootNode && !mRootNode->processed() ) {
    kWarning() << "The root node is not yet processed! Danger!";
    return;
  } else {
    delete mRootNode;
  }
  mRootNode = partNode::fromMessage( aMsg, this );
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
      kDebug() << "FOUND A VALID VCARD";
      writeMessagePartToTempFile( &vCardNode->msgPart(), vCardNode->nodeId() );
    }
  }
  htmlWriter()->queue( writeMsgHeader(aMsg, hasVCard ? vCardNode : 0, true ) );

  // show message content
  ObjectTreeParser otp( this );
  otp.setAllowAsync( true );
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


kDebug() << "\n\n\nSpecial post-encryption handling:\n1.";
kDebug() << "(aMsg == msg) ="                      << (aMsg == message());
kDebug() << "  mLastStatus.isOfUnknownStatus() =" << mLastStatus.isOfUnknownStatus();
kDebug() << "|| mLastStatus.isNew() ="             << mLastStatus.isNew();
kDebug() << "|| mLastStatus.isUnread) ="           << mLastStatus.isUnread();
kDebug() << "(mIdOfLastViewedMessage != aMsg->msgId()) ="       << (mIdOfLastViewedMessage != aMsg->msgId());
kDebug() << "  (KMMsgFullyEncrypted == encryptionState) ="     << (KMMsgFullyEncrypted == encryptionState);
kDebug() << "|| (KMMsgPartiallyEncrypted == encryptionState) =" << (KMMsgPartiallyEncrypted == encryptionState);
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

    kDebug() << "Calling objectTreeToDecryptedMsg()";

    QByteArray decryptedData;
    // note: The following call may change the message's headers.
    objectTreeToDecryptedMsg( mRootNode, decryptedData, *aMsg );
    kDebug() << "Resulting data:" << decryptedData;

    if( !decryptedData.isEmpty() ) {
      kDebug() << "Composing unencrypted message";
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
      kDebug() << "Resulting message:" << unencryptedMessage->asString();
      kDebug() << "Attach unencrypted message to aMsg";
      aMsg->setUnencryptedMsg( unencryptedMessage );
      emitReplaceMsgByUnencryptedVersion = true;
    }
  }
  }

  // store message id to avoid endless recursions
  setIdOfLastViewedMessage( aMsg->msgId() );

  if( emitReplaceMsgByUnencryptedVersion ) {
    kDebug() << "Invoce saving in decrypted form:";
    emit replaceMsgByUnencryptedVersion();
  } else {
    showHideMimeTree();
  }

  aMsg->setIsBeingParsed( false );
}

//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader( KMMessage* aMsg, partNode *vCardNode, bool topLevel )
{
  if ( !headerStyle() ) {
    kFatal() << "Trying to writeMsgHeader() without a header style set!";
  }
  if ( !headerStrategy() ) {
    kFatal() << "trying to writeMsgHeader() without a header strategy set!";
  }
  QString href;
  if ( vCardNode )
    href = vCardNode->asHREF( "body" );
  return headerStyle()->format( aMsg, headerStrategy(), href, mPrinting, topLevel );
}
#endif
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
    return existingFileName.toLocalFile();
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
#ifndef USE_AKONADI_VIEWER
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
#endif

//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl( const KUrl &aUrl )
{
  if ( aUrl.isEmpty() ) return -1;
  if ( !aUrl.isLocalFile() ) return -1;

  QString path = aUrl.toLocalFile();
  uint right = path.lastIndexOf( '/' );
  uint left = path.lastIndexOf( '.', right );

  bool ok;
  int res = path.mid( left + 1, right - left - 1 ).toInt( &ok );
  return ( ok ) ? res : -1;
}

#ifndef USE_AKONADI_VIEWER
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
#endif

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


#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::closeEvent( QCloseEvent *e )
{
  QWidget::closeEvent( e );
  writeConfig();
}
#endif

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

#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOn(const QString &aUrl)
{
  const KUrl url(aUrl);
  if ( url.protocol() == "kmail" || url.protocol() == "x-kmail" || url.protocol() == "attachment"
       || (url.protocol().isEmpty() && url.path().isEmpty()) ) {
    mViewer->setDNDEnabled( false );
  } else {
    mViewer->setDNDEnabled( true );
  }
  if ( aUrl.trimmed().isEmpty() ) {
    KPIM::BroadcastStatus::instance()->reset();
    mHoveredUrl = KUrl();
    return;
  }

  mHoveredUrl = url;

  const QString msg = URLHandlerManager::instance()->statusBarMessage( url, this );

  if ( msg.isEmpty() ) {
    kWarning() << "Unhandled URL hover!";
  }
  KPIM::BroadcastStatus::instance()->setTransientStatusMsg( msg );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KUrl &aUrl, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)
{
  mClickedUrl = aUrl;

  if ( URLHandlerManager::instance()->handleClick( aUrl, this ) )
    return;

  kWarning() << "Unhandled URL click!";
  emit urlClicked( aUrl, Qt::LeftButton );
}
//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  const KUrl url( aUrl );
  mClickedUrl = url;

  if ( URLHandlerManager::instance()->handleContextMenuRequest( url, aPos, this ) )
    return;

  if ( message() ) {
    kWarning() << "Unhandled URL right-click!";
    emit popupMenu( *message(), url, aPos );
  }
}
// Checks if the given node has a parent node that is a DIV which has an ID attribute
// with the value specified here
static bool hasParentDivWithId( const DOM::Node &start, const QString &id )
{
  if ( start.isNull() )
    return false;

  if ( start.nodeName().string() == "div" ) {
    for ( unsigned int i = 0; i < start.attributes().length(); i++ ) {
      if ( start.attributes().item( i ).nodeName().string() == "id" &&
           start.attributes().item( i ).nodeValue().string() == id )
        return true;
    }
  }

  if ( !start.parentNode().isNull() )
    return hasParentDivWithId( start.parentNode(), id );
  else return false;
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
  const bool attachmentInHeader = hasParentDivWithId( mViewer->nodeUnderMouse(), "attachmentInjectionPoint" );
  const bool hasScrollbar = mViewer->view()->verticalScrollBar()->isVisible();
  if ( attachmentInHeader && hasScrollbar ) {
    action = menu->addAction( i18n( "Scroll To" ) );
    connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::ScrollTo );
  }
  action = menu->addAction(SmallIcon("document-save-as"),i18n("Save As...") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Save );

  action = menu->addAction(SmallIcon("edit-copy"), i18n("Copy") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Copy );

  const bool canChange = message()->parent() ? !message()->parent()->isReadOnly() : false;

  if ( GlobalSettings::self()->allowAttachmentEditing() ) {
    action = menu->addAction(SmallIcon("document-properties"), i18n("Edit Attachment") );
    connect( action, SIGNAL(triggered()), attachmentMapper, SLOT(map()) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Edit );
    action->setEnabled( canChange );
  }
  if ( GlobalSettings::self()->allowAttachmentDeletion() ) {
    action = menu->addAction(SmallIcon("edit-delete"), i18n("Delete Attachment") );
    connect( action, SIGNAL(triggered()), attachmentMapper, SLOT(map()) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Delete );
    action->setEnabled( canChange );
  }
  if ( name.endsWith( QLatin1String(".xia"), Qt::CaseInsensitive ) &&
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
  } else if ( choice == KMHandleAttachmentCommand::ScrollTo ) {
    scrollToAttachment( node );
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
#endif
//-----------------------------------------------------------------------------
void KMReaderWin::slotFind()
{
#ifndef USE_AKONADI_VIEWER
  mViewer->findText();
#else
  mViewer->slotFind();
#endif
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::slotToggleFixedFont()
{
  mUseFixedFont = !mUseFixedFont;
  update( true );
}
//-----------------------------------------------------------------------------
void KMReaderWin::slotToggleMimePartTree()
{
  if ( mToggleMimePartTreeAction->isChecked() )
    GlobalSettings::self()->setMimeTreeMode( GlobalSettings::EnumMimeTreeMode::Always );
  else
    GlobalSettings::self()->setMimeTreeMode( GlobalSettings::EnumMimeTreeMode::Never );
  showHideMimeTree();
}
#endif
//-----------------------------------------------------------------------------
void KMReaderWin::slotCopySelectedText()
{
  QString selection = mViewer->selectedText();
  selection.replace( QChar::Nbsp, ' ' );
  QApplication::clipboard()->setText( selection );
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::atmViewMsg( KMMessagePart* aMsgPart, int nodeId )
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
  QString encodeStr;
#ifndef USE_AKONADI_VIEWER
  encodeStr = overrideEncoding();
#else
  encodeStr = mViewer->overrideEncoding();
#endif
  win->showMsg( encodeStr, msg, message()->getMsgSerNum(), nodeId );
  win->show();
}
#endif

void KMReaderWin::setMsgPart( partNode * node ) {
#ifndef USE_AKONADI_VIEWER
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
#endif
}

//-----------------------------------------------------------------------------
void KMReaderWin::setMsgPart( KMMessagePart* aMsgPart, bool aHTML,
                              const QString& aFileName, const QString& pname )
{
#ifndef  USE_AKONADI_VIEWER
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
      delete iio;
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
#endif
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmView( int id, const QString& name )
{
#ifndef USE_AKONADI_VIEWER
  partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
  if( node ) {
    mAtmCurrent = id;
    mAtmCurrentName = name;
    if ( mAtmCurrentName.isEmpty() )
      mAtmCurrentName = tempFileUrlFromPartNode( node ).toLocalFile();

    KMMessagePart& msgPart = node->msgPart();
    QString pname = msgPart.fileName();
    if (pname.isEmpty()) pname=msgPart.name();
    if (pname.isEmpty()) pname=msgPart.contentDescription();
    if (pname.isEmpty()) pname="unnamed";
    // image Attachment is saved already
    if (kasciistricmp(msgPart.typeStr(), "message")==0) {
      atmViewMsg( &msgPart,id );
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
#endif
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::openAttachment( int id, const QString & name )
{
  mAtmCurrentName = name;
  mAtmCurrent = id;

  QString str, pname, cmd, fileName;

  partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
  if( !node ) {
    kWarning() << "Could not find node" << id;
    return;
  }
  if ( mAtmCurrentName.isEmpty() )
    mAtmCurrentName = tempFileUrlFromPartNode( node ).toLocalFile();

  KMMessagePart& msgPart = node->msgPart();
  if (kasciistricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg( &msgPart, id );
    return;
  }

  QByteArray contentTypeStr( msgPart.typeStr() + '/' + msgPart.subtypeStr() );
  kAsciiToLower( contentTypeStr.data() );

  // determine the MIME type of the attachment
  KMimeType::Ptr mimetype;
  // prefer the value of the Content-Type header
  mimetype = KMimeType::mimeType( QString::fromLatin1( contentTypeStr ), KMimeType::ResolveAliases );
  if ( !mimetype.isNull() && mimetype->is( KABC::Addressee::mimeType() ) ) {
    showVCard( &msgPart );
    return;
  }

  // special case treatment on mac
  if ( KMail::Util::handleUrlOnMac( mAtmCurrentName ) )
    return;

  if ( mimetype.isNull() || mimetype->name() == "application/octet-stream" ) {
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

  QString filenameText = msgPart.fileName();
  if ( filenameText.isEmpty() )
    filenameText = msgPart.name();

  AttachmentDialog dialog( this, filenameText, offer ? offer->name() : QString(),
                                  QString::fromLatin1( "askSave_" ) + mimetype->name() );
  const int choice = dialog.exec();

  if ( choice == AttachmentDialog::Save ) {
    mAtmUpdate = true;
    KMHandleAttachmentCommand* command = new KMHandleAttachmentCommand( node,
        message(), mAtmCurrent, mAtmCurrentName, KMHandleAttachmentCommand::Save,
        offer, this );
    connect( command, SIGNAL( showAttachment( int, const QString& ) ),
        this, SLOT( slotAtmView( int, const QString& ) ) );
    command->start();
  }
  else if ( ( choice == AttachmentDialog::Open ) ||
            ( choice == AttachmentDialog::OpenWith ) ) {
    KMHandleAttachmentCommand::AttachmentAction action;
    if ( choice == AttachmentDialog::Open )
      action = KMHandleAttachmentCommand::Open;
    else
      action = KMHandleAttachmentCommand::OpenWith;
    mAtmUpdate = true;
    KMHandleAttachmentCommand* command = new KMHandleAttachmentCommand( node,
        message(), mAtmCurrent, mAtmCurrentName, action, offer, this );
    connect( command, SIGNAL( showAttachment( int, const QString& ) ),
        this, SLOT( slotAtmView( int, const QString& ) ) );
    command->start();
  } else { // Cancel
    kDebug() << "Canceled opening attachment";
  }
}
#endif
//-----------------------------------------------------------------------------
QString KMReaderWin::copyText()
{
  QString temp = mViewer->selectedText();
  return temp;
}

#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentDone()
{
}
#endif
//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlOverride( bool override )
{
  mViewer->setHtmlOverride( override );
}

bool KMReaderWin::htmlOverride() const
{
  return mViewer->htmlOverride();
}

//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlLoadExtOverride( bool override )
{
  mViewer->setHtmlLoadExtOverride( override );
}

//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return mViewer->htmlMail();
}

//-----------------------------------------------------------------------------
bool KMReaderWin::htmlLoadExternal()
{
  return mViewer->htmlLoadExternal();
}

//-----------------------------------------------------------------------------
void KMReaderWin::saveRelativePosition()
{
  mViewer->saveRelativePosition();
}


//-----------------------------------------------------------------------------
void KMReaderWin::update( bool force )
{
#ifndef USE_AKONADI_VIEWER
  KMMessage *msg = message();
  if ( msg ) {
    saveRelativePosition();
    setMsg( msg, force );
  }
#else
  //TODO
#endif
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
    if ( !message ) {
      kWarning() << "Attempt to reference invalid serial number" << mLastSerNum;
    }
    return message;
  }
  return 0;
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlClicked()
{
  KMMainWidget *mainWidget = dynamic_cast<KMMainWidget*>(mMainWindow);
  uint identity = 0;
  if ( message() && message()->parent() ) {
    identity = message()->parent()->identity();
  }

  KMCommand *command = new KMUrlClickedCommand( mClickedUrl, identity, this,
                                                false, mainWidget );
  command->start();
}
#endif
//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoCompose()
{
  KMCommand *command = new KMMailtoComposeCommand( mClickedUrl, message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoForward()
{
  KMCommand *command = new KMMailtoForwardCommand( mMainWindow, mClickedUrl,
                                                   message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoAddAddrBook()
{
  KMCommand *command = new KMMailtoAddAddrBookCommand( mClickedUrl,
                                                       mMainWindow );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoOpenAddrBook()
{
  KMCommand *command = new KMMailtoOpenAddrBookCommand( mClickedUrl,
                                                        mMainWindow );
  command->start();
}
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlCopy()
{
  // we don't necessarily need a mainWidget for KMUrlCopyCommand so
  // it doesn't matter if the dynamic_cast fails.
  KMCommand *command =
    new KMUrlCopyCommand( mClickedUrl,
                          dynamic_cast<KMMainWidget*>( mMainWindow ) );
  command->start();
}
#endif
//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen( const KUrl &url )
{
  if ( !url.isEmpty() )
    mClickedUrl = url;
  KMCommand *command = new KMUrlOpenCommand( mClickedUrl, this );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotAddBookmarks()
{
    KMCommand *command = new KMAddBookmarksCommand( mClickedUrl, this );
    command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlSave()
{
  KMCommand *command = new KMUrlSaveCommand( mClickedUrl, mMainWindow );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoReply()
{
  KMCommand *command = new KMMailtoReplyCommand( mMainWindow, mClickedUrl,
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
#ifndef USE_AKONADI_VIEWER
//-----------------------------------------------------------------------------
void KMReaderWin::slotSaveAttachments()
{
  mAtmUpdate = true;
  KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( mMainWindow,
                                                                        message() );
  saveCommand->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::saveAttachment( const KUrl &tempFileName )
{
  mAtmCurrent = msgPartFromUrl( tempFileName );
  mAtmCurrentName = mClickedUrl.toLocalFile();
  slotHandleAttachment( KMHandleAttachmentCommand::Save ); // save
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
#endif
//-----------------------------------------------------------------------------
bool KMReaderWin::eventFilter( QObject *, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress ) {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if ( me->button() == Qt::LeftButton && ( me->modifiers() & Qt::ShiftModifier ) ) {
      // special processing for shift+click
      URLHandlerManager::instance()->handleShiftClick( mHoveredUrl, this );
      return true;
    }

    if ( me->button() == Qt::LeftButton ) {
      mCanStartDrag = URLHandlerManager::instance()->willHandleDrag( mHoveredUrl, this );
      mLastClickPosition = me->pos();
    }
  }

  if ( e->type() ==  QEvent::MouseButtonRelease ) {
    mCanStartDrag = false;
  }

  if ( e->type() == QEvent::MouseMove ) {
    QMouseEvent* me = static_cast<QMouseEvent*>( e );

    if ( ( mLastClickPosition - me->pos() ).manhattanLength() > KGlobalSettings::dndEventDelay() ) {
      if ( mCanStartDrag && !mHoveredUrl.isEmpty() && mHoveredUrl.protocol() == "attachment" ) {
        mCanStartDrag = false;
        URLHandlerManager::instance()->handleDrag( mHoveredUrl, this );
#ifndef USE_AKONADI_VIEWER
        slotUrlOn( QString() );
#endif
        return true;
      }
    }
  }

  // standard event processing
  return false;
}

void KMReaderWin::fillCommandInfo( partNode *node, KMMessage **msg, int *nodeId )
{
  Q_ASSERT( msg && nodeId );

  if ( mSerNumOfOriginalMessage != 0 ) {
    KMFolder *folder = 0;
    int index = -1;
    KMMsgDict::instance()->getLocation( mSerNumOfOriginalMessage, &folder, &index );
    if ( folder && index != -1 )
      *msg = folder->getMsg( index );

    if ( !( *msg ) ) {
      kWarning() << "Unable to find the original message, aborting attachment deletion!";
      return;
    }

    *nodeId = node->nodeId() + mNodeIdOffset;
  }
  else {
    *nodeId = node->nodeId();
    *msg = message();
  }
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

  int nodeId = -1;
  KMMessage *msg = 0;
  fillCommandInfo( node, &msg, &nodeId );
  if ( msg && nodeId != -1 ) {
    KMDeleteAttachmentCommand* command = new KMDeleteAttachmentCommand( nodeId, msg, this );
    command->start();
  }

  // If we are operating on a copy of parts of the message, make sure to update the copy as well.
  if ( mSerNumOfOriginalMessage != 0 && message() ) {
    message()->deleteBodyPart( node->nodeId() );
    update( true );
  }
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

  int nodeId = -1;
  KMMessage *msg = 0;
  fillCommandInfo( node, &msg, &nodeId );
  if ( msg && nodeId != -1 ) {
    KMEditAttachmentCommand* command = new KMEditAttachmentCommand( nodeId, msg, this );
    command->start();
  }

  // FIXME: If we are operating on a copy of parts of the message, make sure to update the copy as well.
}

CSSHelper* KMReaderWin::cssHelper() const
{
  return mViewer->cssHelper();
}

bool KMReaderWin::decryptMessage() const
{
  return mViewer->decryptMessage();
}
#ifndef USE_AKONADI_VIEWER
void KMReaderWin::scrollToAttachment( const partNode *node )
{
  DOM::Document doc = mViewer->htmlDocument();

  // The anchors for this are created in ObjectTreeParser::parseObjectTree()
  mViewer->gotoAnchor( QString::fromLatin1( "att%1" ).arg( node->nodeId() ) );

  // Remove any old color markings which might be there
  const partNode *root = node->topLevelParent();
  for ( int i = 0; i <= root->totalChildCount() + 1; i++ ) {
    DOM::Element attachmentDiv = doc.getElementById( QString( "attachmentDiv%1" ).arg( i + 1 ) );
    if ( !attachmentDiv.isNull() )
      attachmentDiv.removeAttribute( "style" );
  }

  // Now, color the div of the attachment in yellow, so that the user sees what happened.
  // We created a special marked div for this in writeAttachmentMarkHeader() in ObjectTreeParser,
  // find and modify that now.
  DOM::Element attachmentDiv = doc.getElementById( QString( "attachmentDiv%1" ).arg( node->nodeId() ) );
  if ( attachmentDiv.isNull() ) {
    kWarning() << "Could not find attachment div for attachment" << node->nodeId();
    return;
  }
  attachmentDiv.setAttribute( "style", QString( "border:2px solid %1" )
      .arg( cssHelper()->pgpWarnColor().name() ) );

  // Update rendering, otherwise the rendering is not updated when the user clicks on an attachment
  // that causes scrolling and the open attachment dialog
  doc.updateRendering();
}

void KMReaderWin::toggleFullAddressList()
{
  toggleFullAddressList( "To" );
  toggleFullAddressList( "Cc" );
}
DOM::HTMLElement KMReaderWin::getHTMLElementById( const QString &id )
{
  Q_ASSERT( !id.isNull() );
  Q_ASSERT( !id.isEmpty() );
  DOM::Document doc = mViewer->htmlDocument();
  return static_cast<DOM::HTMLElement>( doc.getElementById( id ) );
}

void KMReaderWin::toggleFullAddressList( const QString &field )
{
  // First inject the current icon
  DOM::HTMLElement tag = getHTMLElementById( "iconFull" + field + "AddressList" );
  if ( tag.isNull() )
    return;
  Q_ASSERT( tag.tagName() == "span" );

  QString imgpath( KStandardDirs::locate( "data","kmail/pics/" ) );
  QString urlHandle;
  QString imgSrc;
  bool doShow = ( field == "To" && showFullToAddressList() ) || ( field == "Cc" && showFullCcAddressList() );
  if ( doShow ) {
    urlHandle.append( "kmail:hideFull" + field + "AddressList" );
    imgSrc.append( "quicklistOpened.png" );
  } else {
    urlHandle.append( "kmail:showFull" + field + "AddressList" );
    imgSrc.append( "quicklistClosed.png" );
  }

  QString link = "<span style=\"text-align: right;\"><a href=\"" + urlHandle + "\"><img src=\"" + imgpath + imgSrc + "\"/></a></span>";
  tag.setInnerHTML( link );

  // Then show/hide the full address list
  DOM::HTMLElement dotsTag = getHTMLElementById( "dotsFull" + field + "AddressList" );
  Q_ASSERT( dotsTag.tagName() == "span" );

  tag = getHTMLElementById( "hiddenFull" + field + "AddressList" );
  Q_ASSERT( tag.tagName() == "span" );

  if (doShow ) {
    dotsTag.addCSSProperty( "display", "none" );
    tag.removeCSSProperty( "display" );
  } else {
    tag.addCSSProperty( "display", "none" );
    dotsTag.removeCSSProperty( "display" );
  }
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
    imgSrc.append( "quicklistClosed.png" );
  } else {
    urlHandle.append( "kmail:hideAttachmentQuicklist" );
    imgSrc.append( "quicklistOpened.png" );
  }

  QColor background = KColorScheme( QPalette::Active, KColorScheme::View ).background().color();
  QString html = renderAttachments( mRootNode, background );
  if ( html.isEmpty() )
    return;

  QString link;
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
  return QColor::fromHsv( (h + 50) % 360, qMax(s, 64), v );
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
      if ( node->msgPart().typeStr().toLower() == "message" || node == mRootNode )
        html += QString::fromLatin1("<div style=\"background:%1; %2"
                "vertical-align:middle; float:%3; %4\">").arg( bgColor.name() ).arg( margin )
                                                         .arg( align ).arg( visibility );
      html += subHtml;
      if ( node->msgPart().typeStr().toLower() == "message" || node == mRootNode )
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
    bool typeBlacklisted = node->msgPart().typeStr().toLower() == "multipart";
    if ( !typeBlacklisted ) {
      typeBlacklisted = StringUtil::isCryptoPart( node->msgPart().typeStr(), node->msgPart().subtypeStr(),
                                                  node->msgPart().fileName() );
    }
    typeBlacklisted = typeBlacklisted || node == mRootNode;
    bool firstTextChildOfEncapsulatedMsg = node->msgPart().typeStr().toLower() == "text" &&
                                           node->msgPart().subtypeStr().toLower() == "plain" &&
                                           node->parentNode() &&
                                           node->parentNode()->msgPart().typeStr().toLower() == "message";
    typeBlacklisted = typeBlacklisted || firstTextChildOfEncapsulatedMsg;
    if ( !label.isEmpty() && !icon.isEmpty() && !typeBlacklisted ) {
      html += "<div style=\"float:left;\">";
      html += QString::fromLatin1( "<span style=\"white-space:nowrap; border-width: 0px; border-left-width: 5px; border-color: %1; 2px; border-left-style: solid;\">" ).arg( bgColor.name() );
      QString fileName = writeMessagePartToTempFile( &node->msgPart(), node->nodeId() );
      QString href = node->asHREF( "header" );
      html += QString::fromLatin1( "<a href=\"" ) + href +
              QString::fromLatin1( "\">" );
      html += "<img style=\"vertical-align:middle;\" src=\"" + icon + "\"/>&nbsp;";
      if ( headerStyle() == HeaderStyle::enterprise() ) {
        QFont bodyFont = cssHelper()->bodyFont( isFixedFont() );
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
#endif
using namespace KMail::Interface;

void KMReaderWin::setBodyPartMemento( const partNode *node,
                                      const QByteArray &which,
                                      BodyPartMemento *memento )
{
  const QByteArray index = node->path() + ':' + which.toLower();

  const std::map<QByteArray,BodyPartMemento*>::iterator it =
    mBodyPartMementoMap.lower_bound( index );

  if ( it != mBodyPartMementoMap.end() && it->first == index ) {
    if ( memento && memento == it->second ) {
      return;
    }

    delete it->second;

    if ( memento ) {
      it->second = memento;
    } else {
      mBodyPartMementoMap.erase( it );
    }
  } else {
    if ( memento ) {
      mBodyPartMementoMap.insert( it, std::make_pair( index, memento ) );
    }
  }

  if ( Observable * o = memento ? memento->asObservable() : 0 ) {
    o->attach( this );
  }
}

BodyPartMemento *KMReaderWin::bodyPartMemento( const partNode *node,
                                               const QByteArray &which ) const
{
  const QByteArray index = node->path() + ':' + which.toLower();
  const std::map<QByteArray,BodyPartMemento*>::const_iterator it =
    mBodyPartMementoMap.find( index );

  if ( it == mBodyPartMementoMap.end() ) {
    return 0;
  } else {
    return it->second;
  }
}

void KMReaderWin::clearBodyPartMementos()
{
  for ( std::map<QByteArray,BodyPartMemento*>::const_iterator
          it = mBodyPartMementoMap.begin(), end = mBodyPartMementoMap.end();
        it != end; ++it ) {
    delete it->second;
  }
  mBodyPartMementoMap.clear();
}

bool KMReaderWin::showFullToAddressList() const
{
  return mShowFullToAddressList;
}

void KMReaderWin::setShowFullToAddressList( bool showFullToAddressList )
{
  mShowFullToAddressList = showFullToAddressList;
}

bool KMReaderWin::showFullCcAddressList() const
{
  return mShowFullCcAddressList;
}

void KMReaderWin::setShowFullCcAddressList( bool showFullCcAddressList )
{
  mShowFullCcAddressList = showFullCcAddressList;
}

bool KMReaderWin::showSignatureDetails() const
{
  return mViewer->showSignatureDetails();
}

void KMReaderWin::setShowSignatureDetails( bool showDetails )
{
  mViewer->setShowSignatureDetails( showDetails );
}
bool KMReaderWin::showAttachmentQuicklist() const
{
  return mViewer->showAttachmentQuicklist();
}

void KMReaderWin::setShowAttachmentQuicklist( bool showAttachmentQuicklist )
{
  mViewer->setShowAttachmentQuicklist( showAttachmentQuicklist );
}

bool KMReaderWin::htmlLoadExtOverride() const
{
  return mViewer->htmlLoadExtOverride();
}
void KMReaderWin::setDecryptMessageOverwrite( bool overwrite )
{
  mViewer->setDecryptMessageOverwrite( overwrite );
}
const AttachmentStrategy * KMReaderWin::attachmentStrategy() const
{
  return mViewer->attachmentStrategy();
}

QString KMReaderWin::overrideEncoding() const
{
  return mViewer->overrideEncoding();
}


KToggleAction *KMReaderWin::toggleFixFontAction()
{
  return mViewer->toggleFixFontAction();
}

KAction *KMReaderWin::toggleMimePartTreeAction()
{
  return mViewer->toggleMimePartTreeAction();
}

KAction *KMReaderWin::selectAllAction()
{
  return mViewer->selectAllAction();
}

const HeaderStrategy * KMReaderWin::headerStrategy() const {
  return mViewer->headerStrategy();
}
const HeaderStyle * KMReaderWin::headerStyle() const {
  return mViewer->headerStyle();
}


KHTMLPart * KMReaderWin::htmlPart() const
{
  return mViewer->htmlPart();
}

KAction *KMReaderWin::copyURLAction()
{
  return mViewer->copyURLAction();
}

KAction *KMReaderWin::copyAction()
{
  return mViewer->copyAction();
}
KAction *KMReaderWin::urlOpenAction()
{
  return mViewer->urlOpenAction();
}
void KMReaderWin::setPrinting(bool enable)
{
  mViewer->setPrinting( enable );
}

void KMReaderWin::clear(bool force )
{
  mViewer->clear( force ? Viewer::Force : Viewer::Delayed );
}

void KMReaderWin::setMessage( Akonadi::Item item, Viewer::UpdateMode updateMode)
{
  mViewer->setMessageItem( item, updateMode );
}

#include "kmreaderwin.moc"


