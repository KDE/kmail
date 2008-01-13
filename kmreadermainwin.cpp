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

#include <q3accel.h>

#include <kicon.h>
#include <kactionmenu.h>
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
#include "kmcommands.h"
#include "kmenubar.h"
#include "kmenu.h"
#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "csshelper.h"
#include "customtemplatesmenu.h"

#include "kmreadermainwin.h"

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
void KMReaderMainWin::initKMReaderMainWin() {
  mCustomTemplateMenus = 0;
  setCentralWidget( mReaderWin );
  setupAccel();
  setupGUI( ToolBar | Keys | StatusBar | Create, "kmreadermainwin.rc" );
  applyMainWindowSettings( KMKernel::config()->group( "Separate Reader Window" ) );
  if( ! mReaderWin->message() ) {
    menuBar()->hide();
    toolBar( "mainToolBar" )->hide();
  }

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
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
void KMReaderMainWin::slotPrintMsg()
{
  KMPrintCommand *command = new KMPrintCommand( this, mReaderWin->message(),
      mReaderWin->htmlOverride(), mReaderWin->htmlLoadExtOverride(),
      mReaderWin->isFixedFont(), mReaderWin->overrideEncoding() );
  command->setOverrideFont( mReaderWin->cssHelper()->bodyFont( mReaderWin->isFixedFont(), true /*printing*/ ) );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyToMsg()
{
  KMCommand *command = new KMReplyToCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyAuthorToMsg()
{
  KMCommand *command = new KMReplyAuthorCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyAllToMsg()
{
  KMCommand *command = new KMReplyToAllCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyListToMsg()
{
  KMCommand *command = new KMReplyListCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardMsg()
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
  kDebug(5006) << "Reply with template:" << tmpl;
  KMCommand *command = new KMCustomReplyToCommand( this,
                                                   mReaderWin->message(),
                                                   mReaderWin->copyText(),
                                                   tmpl );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomReplyAllToMsg( const QString &tmpl )
{
  kDebug(5006) << "Reply to All with template:" << tmpl;
  KMCommand *command = new KMCustomReplyAllToCommand( this,
                                                      mReaderWin->message(),
                                                      mReaderWin->copyText(),
                                                      tmpl );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomForwardMsg( const QString &tmpl)
{
  kDebug(5006) << "Forward with template:" << tmpl;
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
}

void KMReaderMainWin::setupAccel()
{
  if ( kmkernel->xmlGuiInstance().isValid() )
    setComponentData( kmkernel->xmlGuiInstance() );

  //----- File Menu
  mSaveAsAction = KStandardAction::saveAs( mReaderWin, SLOT( slotSaveMsg() ),
                                           actionCollection() );
  mSaveAsAction->setShortcut( KStandardShortcut::shortcut( KStandardShortcut::Save ) );

  mPrintAction = KStandardAction::print( this, SLOT( slotPrintMsg() ), actionCollection() );

  mSaveAtmAction  = new KAction(KIcon("mail-attachment"), i18n("Save A&ttachments..."), actionCollection() );
  connect( mSaveAtmAction, SIGNAL(triggered(bool)), mReaderWin, SLOT(slotSaveAttachments()) );

  QAction *closeAction = KStandardAction::close( this, SLOT( close() ), actionCollection() );
  KShortcut closeShortcut = KShortcut(closeAction->shortcuts());
  closeShortcut.setAlternate( QKeySequence(Qt::Key_Escape));
  closeAction->setShortcuts(closeShortcut);

  //----- View Menu
  mViewSourceAction  = new KAction(i18n("&View Source"), this);
  actionCollection()->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool) ), SLOT(slotShowMsgSrc()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  //----- Message Menu
  mForwardActionMenu  = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  actionCollection()->addAction("message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL( activated() ), this,
           SLOT( slotForwardMsg() ) );

  mForwardAttachedAction  = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","As &Attachment..."), this);
  actionCollection()->addAction("message_forward_as_attachment", mForwardAttachedAction );
  mForwardAttachedAction->setShortcut(QKeySequence(Qt::Key_F));
  connect(mForwardAttachedAction, SIGNAL(triggered(bool) ), SLOT(slotForwardAttachedMsg()));
  mForwardActionMenu->addAction( mForwardAttachedAction );

  mForwardAction  = new KAction(KIcon("mail-forward"), i18n("&Inline..."), this);
  actionCollection()->addAction("message_forward_inline", mForwardAction );
  connect(mForwardAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));
  mForwardAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F));
  mForwardActionMenu->addAction( mForwardAction );

  mRedirectAction  = new KAction(i18nc("Message->Forward->", "&Redirect..."), this);
  actionCollection()->addAction("message_forward_redirect", mRedirectAction );
  connect(mRedirectAction, SIGNAL(triggered(bool)), SLOT(slotRedirectMsg()));
  mRedirectAction->setShortcut(QKeySequence(Qt::Key_E));
  mForwardActionMenu->addAction( mRedirectAction );

  mReplyActionMenu  = new KActionMenu(KIcon("mail-reply-sender"), i18nc("Message->","&Reply"), this);
  actionCollection()->addAction("message_reply_menu", mReplyActionMenu );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
           SLOT(slotReplyToMsg()) );

  mReplyAction  = new KAction(KIcon("mail-reply-sender"), i18n("&Reply..."), this);
  actionCollection()->addAction("reply", mReplyAction );
  connect(mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()));
  mReplyAction->setShortcut(QKeySequence(Qt::Key_R));
  mReplyActionMenu->addAction( mReplyAction );

  mReplyAuthorAction  = new KAction(KIcon("mail-reply-sender"), i18n("Reply to A&uthor..."), this);
  actionCollection()->addAction("reply_author", mReplyAuthorAction );
  connect(mReplyAuthorAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAuthorToMsg()));
  mReplyAuthorAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_A));
  mReplyActionMenu->addAction( mReplyAuthorAction );

  mReplyAllAction  = new KAction(KIcon("mail-reply-all"), i18n("Reply to &All..."), this);
  actionCollection()->addAction("reply_all", mReplyAllAction );
  connect(mReplyAllAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAllToMsg()));
  mReplyAllAction->setShortcut(QKeySequence(Qt::Key_A));
  mReplyActionMenu->addAction( mReplyAllAction );

  mReplyListAction  = new KAction( KIcon("mail-reply-list"), i18n("Reply to Mailing-&List..."), this);
  actionCollection()->addAction("reply_list", mReplyListAction );
  connect(mReplyListAction, SIGNAL(triggered(bool) ), SLOT(slotReplyListToMsg()));
  mReplyListAction->setShortcut(QKeySequence(Qt::Key_L));
  mReplyActionMenu->addAction( mReplyListAction );

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

  mCreateTodoAction = new KAction( KIcon("mail-mark-task"), i18n("Create Task..."), this);
  actionCollection()->addAction( "create_todo", mCreateTodoAction );
  connect( mCreateTodoAction, SIGNAL(triggered(bool)), SLOT(slotCreateTodo()) );

  mCopyActionMenu = new KActionMenu(i18n("&Copy To"), this);
  actionCollection()->addAction("copy_to", mCopyActionMenu );

  updateMessageMenu();
  updateCustomTemplateMenus();

  Q3Accel *accel = new Q3Accel(mReaderWin, "showMsg()");
  accel->connectItem(accel->insertItem(Qt::Key_Up),
                     mReaderWin, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Qt::Key_Down),
                     mReaderWin, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Qt::Key_PageUp),
                     mReaderWin, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Qt::Key_PageDown),
                     mReaderWin, SLOT(slotScrollNext()));
  accel->connectItem(accel->insertItem(KStandardShortcut::shortcut(KStandardShortcut::Copy).primary()), // ###### misses alternate(). Should be ported away from Q3Accel anyway.
                     mReaderWin, SLOT(slotCopySelectedText()));
  connect( mReaderWin, SIGNAL(popupMenu(KMMessage&,const KUrl&,const QPoint&)),
           this, SLOT(slotMsgPopup(KMMessage&,const KUrl&,const QPoint&)) );
  connect( mReaderWin, SIGNAL(urlClicked(const KUrl&,int)),
           mReaderWin, SLOT(slotUrlClicked()) );
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::updateCustomTemplateMenus()
{
  if (!mCustomTemplateMenus)
  {
    mCustomTemplateMenus = new CustomTemplatesMenu( this, actionCollection() );
    connect( mCustomTemplateMenus, SIGNAL(replyTemplateSelected( const QString& )),
             this, SLOT(slotCustomReplyToMsg( const QString& )) );
    connect( mCustomTemplateMenus, SIGNAL(replyAllTemplateSelected( const QString& )),
             this, SLOT(slotCustomReplyAllToMsg( const QString& )) );
    connect( mCustomTemplateMenus, SIGNAL(forwardTemplateSelected( const QString& )),
             this, SLOT(slotCustomForwardMsg( const QString& )) );
  }

  mForwardActionMenu->addSeparator();
  mForwardActionMenu->addAction( mCustomTemplateMenus->forwardActionMenu() );

  mReplyActionMenu->addSeparator();
  mReplyActionMenu->addAction( mCustomTemplateMenus->replyActionMenu() );
  mReplyActionMenu->addAction( mCustomTemplateMenus->replyAllActionMenu() );
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::updateMessageMenu()
{
  mMenuToFolder.clear();

  KMMainWidget* mainwin = kmkernel->getKMMainWidget();
  if ( mainwin )
    mainwin->folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage, this,
                                              &mMenuToFolder, mCopyActionMenu->menu() );
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::slotMsgPopup(KMMessage &aMsg, const KUrl &aUrl, const QPoint& aPoint)
{
  KMenu * menu = new KMenu;
  mUrl = aUrl;
  mMsg = &aMsg;
  bool urlMenuAdded=false;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto") {
      // popup on a mailto URL
      menu->addAction( mReaderWin->mailToComposeAction() );
      if ( mMsg ) {
        menu->addAction( mReaderWin->mailToReplyAction() );
        menu->addAction( mReaderWin->mailToForwardAction() );
        menu->addSeparator();
      }
      menu->addAction( mReaderWin->addAddrBookAction() );
      menu->addAction( mReaderWin->openAddrBookAction() );
      menu->addAction( mReaderWin->copyAction() );
    } else {
      // popup on a not-mailto URL
      menu->addAction( mReaderWin->urlOpenAction() );
      menu->addAction( mReaderWin->addBookmarksAction() );
      menu->addAction( mReaderWin->urlSaveAsAction() );
      menu->addAction( mReaderWin->copyURLAction() );
    }
    urlMenuAdded=true;
  }
  if(!mReaderWin->copyText().isEmpty()) {
    if ( urlMenuAdded )
      menu->addSeparator();
    menu->addAction( mReplyActionMenu );
    menu->addSeparator();

    menu->addAction( mReaderWin->copyAction() );
    menu->addAction( mReaderWin->selectAllAction() );
  } else if ( !urlMenuAdded )
  {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMsg) // no message
    {
      delete menu;
      return;
    }

    if ( ! ( aMsg.parent() && ( aMsg.parent()->isSent() ||
                                aMsg.parent()->isDrafts() ||
                                aMsg.parent()->isTemplates() ) ) ) {
      // add the reply and forward actions only if we are not in a sent-mail,
      // drafts or templates folder
      menu->addAction( mReplyActionMenu );
      menu->addAction( mForwardActionMenu );
      menu->addSeparator();
    }

    updateMessageMenu();
    menu->addAction( mCopyActionMenu );

    menu->addSeparator();
    menu->addAction( mViewSourceAction );
    menu->addAction( mReaderWin->toggleFixFontAction() );
    menu->addSeparator();
    menu->addAction( mPrintAction );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtmAction );
    menu->addAction( mCreateTodoAction );
  }
  menu->exec(aPoint, 0);
  delete menu;
}

void KMReaderMainWin::copySelectedToFolder( QAction* act )
{
  if (!mMenuToFolder[act])
    return;

  KMCommand *command = new KMCopyCommand( mMenuToFolder[act], mMsg );
  command->start();
}

void KMReaderMainWin::slotFontAction( const QString& font)
{
  QFont f( mReaderWin->cssHelper()->bodyFont() );
  f.setFamily( font );
  mReaderWin->cssHelper()->setBodyFont( f );
  mReaderWin->cssHelper()->setPrintFont( f );
  mReaderWin->update();
}

void KMReaderMainWin::slotSizeAction( int size )
{
  QFont f( mReaderWin->cssHelper()->bodyFont() );
  f.setPointSize( size );
  mReaderWin->cssHelper()->setBodyFont( f );
  mReaderWin->cssHelper()->setPrintFont( f );
  mReaderWin->update();
}

void KMReaderMainWin::slotCreateTodo()
{
  if ( !mMsg )
    return;
  KMCommand *command = new CreateTodoCommand( this, mMsg );
  command->start();
}

#include "kmreadermainwin.moc"
