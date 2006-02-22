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

#include <q3accel.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <kapplication.h>
#include <klocale.h>
#include <kstdaccel.h>
#include <kwin.h>
#include <kaction.h>
#include <kiconloader.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include "kmcommands.h"
#include "kmenubar.h"
#include "kmenu.h"
#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"

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
  setupGUI( ToolBar | Keys | StatusBar | Create, "kmreadermainwin.rc" );
  applyMainWindowSettings( KMKernel::config(), "Separate Reader Window" );

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
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
  setCaption( msg->subject() );
  mMsg = msg;
  toolBar( "mainToolBar" )->show();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotPrintMsg()
{
  KMCommand *command = new KMPrintCommand( this, mReaderWin->message(),
      mReaderWin->htmlOverride(), mReaderWin->htmlLoadExtOverride(),
      mReaderWin->isFixedFont(), mReaderWin->overrideEncoding() );
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
   if ( mReaderWin->message()->parent() ) {
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
   if ( mReaderWin->message()->parent() ) {
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

  //----- File Menu
  //mOpenAction = KStdAction::open( this, SLOT( slotOpenMsg() ),
  //                                actionCollection() );

  //mSaveAsAction = new KAction( i18n("Save &As..."), "filesave",
  //                             KStdAccel::shortcut( KStdAccel::Save ),
  //                             this, SLOT( slotSaveMsg() ),
  //                             actionCollection(), "file_save_as" );

  mPrintAction = KStdAction::print( this, SLOT( slotPrintMsg() ),
                                    actionCollection() );

  KAction *closeAction = KStdAction::close( this, SLOT( close() ), actionCollection() );
  KShortcut closeShortcut = closeAction->shortcut();
  closeShortcut.append( KKey(Qt::Key_Escape));
  closeAction->setShortcut(closeShortcut);

  //----- View Menu
  mViewSourceAction = new KAction( i18n("&View Source"), Qt::Key_V, this,
                                   SLOT(slotShowMsgSrc()), actionCollection(),
                                   "view_source" );


  mForwardActionMenu = new KActionMenu( i18n("Message->","&Forward"),
					"mail_forward", actionCollection(),
					"message_forward" );
  connect( mForwardActionMenu, SIGNAL( activated() ), this,
           SLOT( slotForwardMsg() ) );

  mForwardAction = new KAction( i18n("&Inline..."), "mail_forward",
				Qt::SHIFT+Qt::Key_F, this, SLOT(slotForwardMsg()),
				actionCollection(), "message_forward_inline" );
  mForwardActionMenu->insert( mForwardAction );

  mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", Qt::Key_F, this,
					SLOT(slotForwardAttachedMsg()), actionCollection(),
					"message_forward_as_attachment" );
  mForwardActionMenu->insert( mForwardAttachedAction );

  mRedirectAction = new KAction( i18n("Message->Forward->","&Redirect..."),
				 Qt::Key_E, this, SLOT(slotRedirectMsg()),
				 actionCollection(), "message_forward_redirect" );
  mForwardActionMenu->insert( mRedirectAction );

  mReplyActionMenu = new KActionMenu( i18n("Message->","&Reply"),
                                      "mail_reply", actionCollection(),
                                      "message_reply_menu" );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
	   SLOT(slotReplyToMsg()) );

  mReplyAction = new KAction( i18n("&Reply..."), "mail_reply", Qt::Key_R, this,
			      SLOT(slotReplyToMsg()), actionCollection(), "reply" );
  mReplyActionMenu->insert( mReplyAction );

  mReplyAuthorAction = new KAction( i18n("Reply to A&uthor..."), "mail_reply",
                                    Qt::SHIFT+Qt::Key_A, this,
                                    SLOT(slotReplyAuthorToMsg()),
                                    actionCollection(), "reply_author" );
  mReplyActionMenu->insert( mReplyAuthorAction );

  mReplyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
				 Qt::Key_A, this, SLOT(slotReplyAllToMsg()),
				 actionCollection(), "reply_all" );
  mReplyActionMenu->insert( mReplyAllAction );

  mReplyListAction = new KAction( i18n("Reply to Mailing-&List..."),
				  "mail_replylist", Qt::Key_L, this,
				  SLOT(slotReplyListToMsg()), actionCollection(),
				  "reply_list" );
  mReplyActionMenu->insert( mReplyListAction );



  Q3Accel *accel = new Q3Accel(mReaderWin, "showMsg()");
  accel->connectItem(accel->insertItem(Qt::Key_Up),
                     mReaderWin, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Qt::Key_Down),
                     mReaderWin, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Qt::Key_PageUp),
                     mReaderWin, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Qt::Key_PageDown),
                     mReaderWin, SLOT(slotScrollNext()));
  accel->connectItem(accel->insertItem(KStdAccel::shortcut(KStdAccel::Copy)),
                     mReaderWin, SLOT(slotCopySelectedText()));
  connect( mReaderWin, SIGNAL(popupMenu(KMMessage&,const KUrl&,const QPoint&)),
	  this, SLOT(slotMsgPopup(KMMessage&,const KUrl&,const QPoint&)));
  connect(mReaderWin, SIGNAL(urlClicked(const KUrl&,int)),
	  mReaderWin, SLOT(slotUrlClicked()));

}


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

    if( !aMsg.parent()->isSent() && !aMsg.parent()->isDrafts() ) {
      mReplyActionMenu->plug( menu );
      mForwardActionMenu->plug( menu );
      menu->insertSeparator();
    }

    Q3PopupMenu* copyMenu = new Q3PopupMenu(menu);
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
    menu->insertItem(  SmallIcon("filesaveas"), i18n( "Save &As..." ), mReaderWin, SLOT( slotSaveMsg() ) );
    menu->insertItem( i18n("Save Attachments..."), mReaderWin, SLOT(slotSaveAttachments()) );
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

#include "kmreadermainwin.moc"
