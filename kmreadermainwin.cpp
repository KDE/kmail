/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
//
// A toplevel KMainWindow derived class for displaying
// single messages or single message parts.
//
// Could be extended to include support for normal main window
// widgets like a toolbar.

#include "kmreadermainwin.h"
#include "kmreaderwin.h"

#include <kicon.h>
#include <kactionmenu.h>
#include <kedittoolbar.h>
#include <klocale.h>
#include <kstandardshortcut.h>
#include <kwindowsystem.h>
#include <kaction.h>
#include <kfontaction.h>
#include <kiconloader.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include <KFontAction>
#include <KFontSizeAction>
#include <kstatusbar.h>
#include <khtml_part.h>
#include "kmcommands.h"
#include "kmenubar.h"
#include "kmenu.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "csshelper.h"
#include "customtemplatesmenu.h"
#include "messageactions.h"
#include "kmmsgdict.h"
#include "mainfolderview.h"

#include <kabc/stdaddressbook.h>
#include <kpimutils/email.h>

KMReaderMainWin::KMReaderMainWin( bool htmlOverride, bool htmlLoadExtOverride,
                                  char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" ),
    mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  //mReaderWin->setShowCompleteMessage( true );
  mReaderWin->setAutoDelete( true );
  mReaderWin->setHtmlOverride( htmlOverride );
  mReaderWin->setHtmlLoadExtOverride( htmlLoadExtOverride );
  mReaderWin->setDecryptMessageOverwrite( true );
  mReaderWin->setShowSignatureDetails( false );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
KMReaderMainWin::KMReaderMainWin( char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" ),
    mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  mReaderWin->setAutoDelete( true );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
KMReaderMainWin::KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QString & encoding, char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" ),
    mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setMsgPart( aMsgPart, aHTML, aFileName, pname );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::initKMReaderMainWin()
{
  setCentralWidget( mReaderWin );
  setupAccel();
  setupGUI( Keys | StatusBar | Create, "kmreadermainwin.rc" );
  mMsgActions->setupForwardingActionsList( this );
  applyMainWindowSettings( KMKernel::config()->group( "Separate Reader Window" ) );
  if( ! mReaderWin->message() ) {
    menuBar()->hide();
    toolBar( "mainToolBar" )->hide();
  }

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );

  connect( mReaderWin->htmlPart(), SIGNAL( onURL( const QString& ) ),
           statusBar(), SLOT( showMessage(const QString& ) ) );
}

//-----------------------------------------------------------------------------
KMReaderMainWin::~KMReaderMainWin()
{
  saveMainWindowSettings( KMKernel::config()->group( "Separate Reader Window" ) );
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::setUseFixedFont( bool useFixedFont )
{
  mReaderWin->setUseFixedFont( useFixedFont );
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::showMsg( const QString & encoding, KMMessage *msg )
{
  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setMsg( msg, true );
  mReaderWin->slotTouchMessage();
  setCaption( msg->subject() );
  mMsg = msg;
  mMsgActions->setCurrentMessage( msg );
  menuBar()->show();
  toolBar( "mainToolBar" )->show();

  connect ( msg->parent(), SIGNAL( destroyed( QObject* ) ), this, SLOT( slotFolderRemoved( QObject* ) ) );

}

void KMReaderMainWin::slotFolderRemoved( QObject* folderPtr )
{
  assert(mMsg);
  assert(folderPtr == mMsg->parent());
  if( mMsg && folderPtr == mMsg->parent() )
    mMsg->setParent( 0 );
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotTrashMsg()
{
  if ( !mMsg )
    return;
  // find the real msg by its sernum
  KMFolder* parent;
  int index;
  KMMsgDict::instance()->getLocation( mMsg->getMsgSerNum(), &parent, &index );
  if (parent) {
    KMMessage *msg = parent->getMsg( index );
    if (msg) {
      // now delete the msg and close this window
      KMTrashMsgCommand *command = new KMTrashMsgCommand( parent, msg );
      command->start();
      close();
    }
  }
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotPrintMsg()
{
  KMPrintCommand *command = new KMPrintCommand( this, mReaderWin->message(),
      mReaderWin->headerStyle(), mReaderWin->headerStrategy(),
      mReaderWin->htmlOverride(), mReaderWin->htmlLoadExtOverride(),
      mReaderWin->isFixedFont(), mReaderWin->overrideEncoding() );
  command->setOverrideFont( mReaderWin->cssHelper()->bodyFont( mReaderWin->isFixedFont(), true /*printing*/ ) );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardInlineMsg()
{
   KMCommand *command = 0;
   if ( mReaderWin->message() && mReaderWin->message()->parent() ) {
    command = new KMForwardCommand( this, mReaderWin->message(),
        mReaderWin->message()->parent()->identity() );
   } else {
    command = new KMForwardCommand( this, mReaderWin->message() );
   }
   command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardAttachedMsg()
{
   KMCommand *command = 0;
   if ( mReaderWin->message() && mReaderWin->message()->parent() ) {
     command = new KMForwardAttachedCommand( this, mReaderWin->message(),
        mReaderWin->message()->parent()->identity() );
   } else {
     command = new KMForwardAttachedCommand( this, mReaderWin->message() );
   }
   command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotRedirectMsg()
{
  KMCommand *command = new KMRedirectCommand( this, mReaderWin->message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomReplyToMsg( const QString &tmpl )
{
  kDebug() << "Reply with template:" << tmpl;
  KMCommand *command = new KMCustomReplyToCommand( this,
                                                   mReaderWin->message(),
                                                   mReaderWin->copyText(),
                                                   tmpl );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomReplyAllToMsg( const QString &tmpl )
{
  kDebug() << "Reply to All with template:" << tmpl;
  KMCommand *command = new KMCustomReplyAllToCommand( this,
                                                      mReaderWin->message(),
                                                      mReaderWin->copyText(),
                                                      tmpl );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomForwardMsg( const QString &tmpl)
{
  kDebug() << "Forward with template:" << tmpl;
  KMCommand *command = new KMCustomForwardCommand( this,
                                                   mReaderWin->message(),
                                                   0, tmpl );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotShowMsgSrc()
{
  KMMessage *msg = mReaderWin->message();
  if ( !msg )
    return;
  KMCommand *command = new KMShowMsgSrcCommand( this, msg,
                                                mReaderWin->isFixedFont() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotConfigChanged()
{
  //readConfig();
  mMsgActions->setupForwardActions();
  mMsgActions->setupForwardingActionsList( this );
}

void KMReaderMainWin::setupAccel()
{
  if ( kmkernel->xmlGuiInstance().isValid() )
    setComponentData( kmkernel->xmlGuiInstance() );

  mMsgActions = new KMail::MessageActions( actionCollection(), this );
  mMsgActions->setMessageView( mReaderWin );
  //----- File Menu
  mSaveAsAction = KStandardAction::saveAs( mReaderWin, SLOT( slotSaveMsg() ),
                                           actionCollection() );
  mSaveAsAction->setShortcut( KStandardShortcut::shortcut( KStandardShortcut::Save ) );

  mPrintAction = KStandardAction::print( this, SLOT( slotPrintMsg() ), actionCollection() );

  mSaveAtmAction  = new KAction(KIcon("mail-attachment"), i18n("Save A&ttachments..."), actionCollection() );
  connect( mSaveAtmAction, SIGNAL(triggered(bool)), mReaderWin, SLOT(slotSaveAttachments()) );

  mTrashAction = new KAction( KIcon( "user-trash" ), i18n("&Move to Trash"), this );
  mTrashAction->setIconText( i18nc( "@action:intoolbar Move to Trash", "Trash" ) );
  mTrashAction->setHelpText( i18n( "Move message to trashcan" ) );
  mTrashAction->setShortcut( QKeySequence( Qt::Key_Delete ) );
  actionCollection()->addAction( "move_to_trash", mTrashAction );
  connect( mTrashAction, SIGNAL(triggered()), this, SLOT(slotTrashMsg()) );

  KAction *closeAction = KStandardAction::close( this, SLOT( close() ), actionCollection() );
  KShortcut closeShortcut = KShortcut(closeAction->shortcuts());
  closeShortcut.setAlternate( QKeySequence(Qt::Key_Escape));
  closeAction->setShortcuts(closeShortcut);

  //----- View Menu
  mViewSourceAction  = new KAction(i18n("&View Source"), this);
  actionCollection()->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool) ), SLOT(slotShowMsgSrc()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  //----- Message Menu

  fontAction = new KFontAction( i18n("Select Font"), this );
  actionCollection()->addAction( "text_font", fontAction );
  fontAction->setFont( mReaderWin->cssHelper()->bodyFont().family() );
  connect( fontAction, SIGNAL( triggered( const QString& ) ),
           SLOT( slotFontAction( const QString& ) ) );
  fontSizeAction = new KFontSizeAction( i18n( "Select Size" ), this );
  fontSizeAction->setFontSize( mReaderWin->cssHelper()->bodyFont().pointSize() );
  actionCollection()->addAction( "text_size", fontSizeAction );
  connect( fontSizeAction, SIGNAL( fontSizeChanged( int ) ),
           SLOT( slotSizeAction( int ) ) );

  mCopyActionMenu = new KActionMenu(i18n("&Copy To"), this);
  actionCollection()->addAction("copy_to", mCopyActionMenu );

  updateMessageMenu();
  updateCustomTemplateMenus();

  connect( mReaderWin, SIGNAL(popupMenu(KMMessage&,const KUrl&,const QPoint&)),
           this, SLOT(slotMsgPopup(KMMessage&,const KUrl&,const QPoint&)) );
  connect( mReaderWin, SIGNAL(urlClicked(const KUrl&,int)),
           mReaderWin, SLOT(slotUrlClicked()) );

  setStandardToolBarMenuEnabled(true);
  KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::updateCustomTemplateMenus()
{
  if ( !mCustomTemplateMenus ) {
    mCustomTemplateMenus.reset( new CustomTemplatesMenu( this, actionCollection() ) );
    connect( mCustomTemplateMenus.get(), SIGNAL(replyTemplateSelected( const QString& )),
             this, SLOT(slotCustomReplyToMsg( const QString& )) );
    connect( mCustomTemplateMenus.get(), SIGNAL(replyAllTemplateSelected( const QString& )),
             this, SLOT(slotCustomReplyAllToMsg( const QString& )) );
    connect( mCustomTemplateMenus.get(), SIGNAL(forwardTemplateSelected( const QString& )),
             this, SLOT(slotCustomForwardMsg( const QString& )) );
  }

  mMsgActions->forwardMenu()->addSeparator();
  mMsgActions->forwardMenu()->addAction( mCustomTemplateMenus->forwardActionMenu() );

  mMsgActions->replyMenu()->addSeparator();
  mMsgActions->replyMenu()->addAction( mCustomTemplateMenus->replyActionMenu() );
  mMsgActions->replyMenu()->addAction( mCustomTemplateMenus->replyAllActionMenu() );
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::updateMessageMenu()
{
  KMMainWidget* mainwin = kmkernel->getKMMainWidget();
  if ( mainwin )
    mainwin->mainFolderView()->folderToPopupMenu( KMail::MainFolderView::CopyMessage, this,
                                                  mCopyActionMenu->menu() );
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::slotMsgPopup( KMMessage &aMsg, const KUrl &aUrl, const QPoint &aPoint )
{
  KMenu *menu = new KMenu;
  mUrl = aUrl;
  mMsg = &aMsg;
  bool urlMenuAdded = false;
  bool copyAdded = false;
  if ( !aUrl.isEmpty() ) {
    if ( aUrl.protocol() == "mailto" ) {
      // popup on a mailto URL
      menu->addAction( mReaderWin->mailToComposeAction() );
      if ( mMsg ) {
        menu->addAction( mReaderWin->mailToReplyAction() );
        menu->addAction( mReaderWin->mailToForwardAction() );
        menu->addSeparator();
      }
      QString email =  KPIMUtils::firstEmailAddress( aUrl.path() );
      KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
      KABC::Addressee::List addresseeList = addressBook->findByEmail( email );

      if ( addresseeList.count() == 0 ) {
        menu->addAction( mReaderWin->addAddrBookAction() );
      } else {
        menu->addAction( mReaderWin->openAddrBookAction() );
      }
      menu->addAction( mReaderWin->copyAction() );
      copyAdded = true;
    } else {
      // popup on a not-mailto URL
      menu->addAction( mReaderWin->urlOpenAction() );
      menu->addAction( mReaderWin->addBookmarksAction() );
      menu->addAction( mReaderWin->urlSaveAsAction() );
      menu->addAction( mReaderWin->copyURLAction() );
    }
    urlMenuAdded = true;
  }
  if ( !mReaderWin->copyText().isEmpty() ) {
    if ( urlMenuAdded ) {
      menu->addSeparator();
    }
    menu->addAction( mMsgActions->replyMenu() );
    menu->addSeparator();
    if( !copyAdded )
      menu->addAction( mReaderWin->copyAction() );
    menu->addAction( mReaderWin->selectAllAction() );
  } else if ( !urlMenuAdded ) {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMsg) {
      // no message
      delete menu;
      return;
    }

    if ( ! ( aMsg.parent() && ( aMsg.parent()->isSent() ||
                                aMsg.parent()->isDrafts() ||
                                aMsg.parent()->isTemplates() ) ) ) {
      // add the reply and forward actions only if we are not in a sent-mail,
      // templates or drafts folder
      //
      // FIXME: needs custom templates added to menu
      // (see KMMainWidget::updateCustomTemplateMenus)
      menu->addAction( mMsgActions->replyMenu() );
      menu->addAction( mMsgActions->forwardMenu() );
      menu->addSeparator();
    }

    updateMessageMenu();
    menu->addAction( mCopyActionMenu );

    menu->addSeparator();
    menu->addAction( mViewSourceAction );
    menu->addAction( mReaderWin->toggleFixFontAction() );
    menu->addAction( mReaderWin->toggleMimePartTreeAction() );
    menu->addSeparator();
    menu->addAction( mPrintAction );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtmAction );
    menu->addAction( mMsgActions->createTodoAction() );
  }
  menu->exec( aPoint, 0 );
  delete menu;
}

void KMReaderMainWin::slotCopySelectedMessagesToFolder( QAction* act )
{
  KMFolder * f = static_cast<KMFolder *>( act->data().value<void *>() );
  if ( !f )
    return;

  KMCommand *command = new KMCopyCommand( f, mMsg );
  command->start();
}

void KMReaderMainWin::slotFontAction( const QString& font)
{
  QFont f( mReaderWin->cssHelper()->bodyFont() );
  f.setFamily( font );
  mReaderWin->cssHelper()->setBodyFont( f );
  mReaderWin->cssHelper()->setPrintFont( f );
  mReaderWin->saveRelativePosition();
  mReaderWin->update();
}

void KMReaderMainWin::slotSizeAction( int size )
{
  QFont f( mReaderWin->cssHelper()->bodyFont() );
  f.setPointSize( size );
  mReaderWin->cssHelper()->setBodyFont( f );
  mReaderWin->cssHelper()->setPrintFont( f );
  mReaderWin->saveRelativePosition();
  mReaderWin->update();
}

void KMReaderMainWin::slotCreateTodo()
{
  if ( !mMsg )
    return;
  KMCommand *command = new CreateTodoCommand( this, mMsg );
  command->start();
}

void KMReaderMainWin::slotEditToolbars()
{
  saveMainWindowSettings( KConfigGroup(KMKernel::config(), "ReaderWindow") );
  KEditToolBar dlg( guiFactory(), this );
  connect( &dlg, SIGNAL(newToolbarConfig()), SLOT(slotUpdateToolbars()) );
  dlg.exec();
}

void KMReaderMainWin::slotUpdateToolbars()
{
  createGUI("kmreadermainwin.rc");
  applyMainWindowSettings( KConfigGroup(KMKernel::config(), "ReaderWindow") );
}

#include "kmreadermainwin.moc"
