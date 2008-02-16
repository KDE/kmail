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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qaccel.h>
#include <kapplication.h>
#include <kedittoolbar.h>
#include <klocale.h>
#include <kstdaccel.h>
#include <kwin.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kdebug.h>
#include "kmcommands.h"
#include "kmenubar.h"
#include "kpopupmenu.h"
#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "kmmsgdict.h"
#include "csshelper.h"
#include "messageactions.h"

#include "globalsettings.h"

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
void KMReaderMainWin::initKMReaderMainWin() {
  setCentralWidget( mReaderWin );
  setupAccel();
  setupGUI( Keys | StatusBar | Create, "kmreadermainwin.rc" );
  setupForwardingActionsList();
  applyMainWindowSettings( KMKernel::config(), "Separate Reader Window" );
  if ( ! mReaderWin->message() ) {
    menuBar()->hide();
    toolBar( "mainToolBar" )->hide();
  }

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
}

void KMReaderMainWin::setupForwardingActionsList()
{
  QPtrList<KAction> mForwardActionList;
  if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
      unplugActionList( "forward_action_list" );
      mForwardActionList.append( mForwardInlineAction );
      mForwardActionList.append( mForwardAttachedAction );
      mForwardActionList.append( mForwardDigestAction );
      mForwardActionList.append( mRedirectAction );
      plugActionList( "forward_action_list", mForwardActionList );
  } else {
      unplugActionList( "forward_action_list" );
      mForwardActionList.append( mForwardAttachedAction );
      mForwardActionList.append( mForwardInlineAction );
      mForwardActionList.append( mForwardDigestAction );
      mForwardActionList.append( mRedirectAction );
      plugActionList( "forward_action_list", mForwardActionList );
  }
}

//-----------------------------------------------------------------------------
KMReaderMainWin::~KMReaderMainWin()
{
  saveMainWindowSettings( KMKernel::config(), "Separate Reader Window" );
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
  if ( parent && !parent->isTrash() ) {
    // open the folder (ref counted)
    parent->open("trashmsg");
    KMMessage *msg = parent->getMsg( index );
    if (msg) {
      KMDeleteMsgCommand *command = new KMDeleteMsgCommand( parent, msg );
      command->start();
    }
    parent->close("trashmsg");
  }
  close();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotFind()
{
  mReaderWin->slotFind();
}

void KMReaderMainWin::slotFindNext()
{
  mReaderWin->slotFindNext();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCopy()
{
  mReaderWin->slotCopySelectedText();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotMarkAll()
{
  mReaderWin->selectAll();
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
void KMReaderMainWin::slotForwardInlineMsg()
{
   KMCommand *command = 0;
   if ( mReaderWin->message() && mReaderWin->message()->parent() ) {
    command = new KMForwardInlineCommand( this, mReaderWin->message(),
        mReaderWin->message()->parent()->identity() );
   } else {
    command = new KMForwardInlineCommand( this, mReaderWin->message() );
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
void KMReaderMainWin::slotForwardDigestMsg()
{
   KMCommand *command = 0;
   if ( mReaderWin->message() && mReaderWin->message()->parent() ) {
     command = new KMForwardDigestCommand( this, mReaderWin->message(),
        mReaderWin->message()->parent()->identity() );
   } else {
     command = new KMForwardDigestCommand( this, mReaderWin->message() );
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
  if ( kmkernel->xmlGuiInstance() )
    setInstance( kmkernel->xmlGuiInstance() );

  mMsgActions = new KMail::MessageActions( actionCollection(), this );
  mMsgActions->setMessageView( mReaderWin );
  //----- File Menu
  //mOpenAction = KStdAction::open( this, SLOT( slotOpenMsg() ),
  //                                actionCollection() );

  //mSaveAsAction = new KAction( i18n("Save &As..."), "filesave",
  //                             KStdAccel::shortcut( KStdAccel::Save ),
  //                             this, SLOT( slotSaveMsg() ),
  //                             actionCollection(), "file_save_as" );

  mSaveAsAction = KStdAction::saveAs( mReaderWin, SLOT( slotSaveMsg() ),
				      actionCollection() );
  mSaveAsAction->setShortcut( KStdAccel::shortcut( KStdAccel::Save ) );
  mPrintAction = KStdAction::print( this, SLOT( slotPrintMsg() ),
                                    actionCollection() );

  KAction *closeAction = KStdAction::close( this, SLOT( close() ), actionCollection() );
  KShortcut closeShortcut = closeAction->shortcut();
  closeShortcut.append( KKey(Key_Escape));
  closeAction->setShortcut(closeShortcut);

  //----- Edit Menu
  KStdAction::copy( this, SLOT( slotCopy() ), actionCollection() );
  KStdAction::selectAll( this, SLOT( slotMarkAll() ), actionCollection() );
  KStdAction::find( this, SLOT(slotFind()), actionCollection() );
  KStdAction::findNext( this, SLOT( slotFindNext() ), actionCollection() );
  mTrashAction = new KAction( KGuiItem( i18n( "&Move to Trash" ), "edittrash",
                              i18n( "Move message to trashcan" ) ),
                              Key_Delete, this, SLOT( slotTrashMsg() ),
                              actionCollection(), "move_to_trash" );

  //----- View Menu
  mViewSourceAction = new KAction( i18n("&View Source"), Key_V, this,
                                   SLOT(slotShowMsgSrc()), actionCollection(),
                                   "view_source" );


  mForwardActionMenu = new KActionMenu( i18n("Message->","&Forward"),
					"mail_forward", actionCollection(),
					"message_forward" );
      mForwardInlineAction = new KAction( i18n("&Inline..."),
                                      "mail_forward", SHIFT+Key_F, this,
                                      SLOT(slotForwardInlineMsg()),
                                      actionCollection(),
                                      "message_forward_inline" );

      mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
                                        "mail_forward", Key_F, this,
                                        SLOT(slotForwardAttachedMsg()),
                                        actionCollection(),
                                        "message_forward_as_attachment" );

      mForwardDigestAction = new KAction( i18n("Message->Forward->","As Di&gest..."),
                                      "mail_forward", 0, this,
                                      SLOT(slotForwardDigestMsg()),
                                      actionCollection(),
                                      "message_forward_as_digest" );

      mRedirectAction = new KAction( i18n("Message->Forward->","&Redirect..."),
                                 "mail_forward", Key_E, this,
                                 SLOT(slotRedirectMsg()),
                                 actionCollection(),
                                 "message_forward_redirect" );

  if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
      mForwardActionMenu->insert( mForwardInlineAction );
      mForwardActionMenu->insert( mForwardAttachedAction );
      mForwardInlineAction->setShortcut( Key_F );
      mForwardAttachedAction->setShortcut( SHIFT+Key_F );
      connect( mForwardActionMenu, SIGNAL(activated()), this,
               SLOT(slotForwardInlineMsg()) );
  } else {
      mForwardActionMenu->insert( mForwardAttachedAction );
      mForwardActionMenu->insert( mForwardInlineAction );
      mForwardInlineAction->setShortcut( SHIFT+Key_F );
      mForwardAttachedAction->setShortcut( Key_F );
      connect( mForwardActionMenu, SIGNAL(activated()), this,
               SLOT(slotForwardAttachedMsg()) );
  }

  mForwardActionMenu->insert( mForwardDigestAction );
  mForwardActionMenu->insert( mRedirectAction );

  fontAction = new KFontAction( "Select Font", 0, actionCollection(),
                               "text_font" );
  fontAction->setFont( mReaderWin->cssHelper()->bodyFont().family() );
  connect( fontAction, SIGNAL( activated( const QString& ) ),
           SLOT( slotFontAction( const QString& ) ) );
  fontSizeAction = new KFontSizeAction( "Select Size", 0, actionCollection(),
                                       "text_size" );
  fontSizeAction->setFontSize( mReaderWin->cssHelper()->bodyFont().pointSize() );
  connect( fontSizeAction, SIGNAL( fontSizeChanged( int ) ),
           SLOT( slotSizeAction( int ) ) );

  QAccel *accel = new QAccel(mReaderWin, "showMsg()");
  accel->connectItem(accel->insertItem(Key_Up),
                     mReaderWin, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
                     mReaderWin, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
                     mReaderWin, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Key_Next),
                     mReaderWin, SLOT(slotScrollNext()));
  accel->connectItem(accel->insertItem(KStdAccel::shortcut(KStdAccel::Copy)),
                     mReaderWin, SLOT(slotCopySelectedText()));
  connect( mReaderWin, SIGNAL(popupMenu(KMMessage&,const KURL&,const QPoint&)),
	  this, SLOT(slotMsgPopup(KMMessage&,const KURL&,const QPoint&)));
  connect(mReaderWin, SIGNAL(urlClicked(const KURL&,int)),
	  mReaderWin, SLOT(slotUrlClicked()));

  setStandardToolBarMenuEnabled(true);
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
}


void KMReaderMainWin::slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu * menu = new KPopupMenu;
  mUrl = aUrl;
  mMsg = &aMsg;
  bool urlMenuAdded=false;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto") {
      // popup on a mailto URL
      mReaderWin->mailToComposeAction()->plug( menu );
      if ( mMsg ) {
        mReaderWin->mailToReplyAction()->plug( menu );
        mReaderWin->mailToForwardAction()->plug( menu );
        menu->insertSeparator();
      }
      mReaderWin->addAddrBookAction()->plug( menu );
      mReaderWin->openAddrBookAction()->plug( menu );
      mReaderWin->copyAction()->plug( menu );
    } else {
      // popup on a not-mailto URL
      mReaderWin->urlOpenAction()->plug( menu );
      mReaderWin->addBookmarksAction()->plug( menu );
      mReaderWin->urlSaveAsAction()->plug( menu );
      mReaderWin->copyURLAction()->plug( menu );
    }
    urlMenuAdded=true;
  }
  if(mReaderWin && !mReaderWin->copyText().isEmpty()) {
    if ( urlMenuAdded )
      menu->insertSeparator();
    mMsgActions->replyMenu()->plug( menu );
    menu->insertSeparator();

    mReaderWin->copyAction()->plug( menu );
    mReaderWin->selectAllAction()->plug( menu );
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
      // templates or drafts folder
      //
      // FIXME: needs custom templates added to menu
      // (see KMMainWidget::updateCustomTemplateMenus)
      mMsgActions->replyMenu()->plug( menu );
      mForwardActionMenu->plug( menu );
      menu->insertSeparator();
    }

    QPopupMenu* copyMenu = new QPopupMenu(menu);
    KMMainWidget* mainwin = kmkernel->getKMMainWidget();
    if ( mainwin )
      mainwin->folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage, this,
          &mMenuToFolder, copyMenu );
    menu->insertItem( i18n("&Copy To" ), copyMenu );
    menu->insertSeparator();
    mViewSourceAction->plug( menu );
    mReaderWin->toggleFixFontAction()->plug( menu );
    menu->insertSeparator();
    mPrintAction->plug( menu );
    mSaveAsAction->plug( menu );
    menu->insertItem( i18n("Save Attachments..."), mReaderWin, SLOT(slotSaveAttachments()) );
    mMsgActions->createTodoAction()->plug( menu );
  }
  menu->exec(aPoint, 0);
  delete menu;
}

void KMReaderMainWin::copySelectedToFolder( int menuId )
{
  if (!mMenuToFolder[menuId])
    return;

  KMCommand *command = new KMCopyCommand( mMenuToFolder[menuId], mMsg );
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
  saveMainWindowSettings( KMKernel::config(), "ReaderWindow" );
  KEditToolbar dlg( guiFactory(), this );
  connect( &dlg, SIGNAL(newToolbarConfig()), SLOT(slotUpdateToolbars()) );
  dlg.exec();
}

void KMReaderMainWin::slotUpdateToolbars()
{
  createGUI("kmreadermainwin.rc");
  applyMainWindowSettings(KMKernel::config(), "ReaderWindow");
}

#include "kmreadermainwin.moc"
