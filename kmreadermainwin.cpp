// kmreadermainwin
// (c) 2002 Don Sanders <sanders@kde.org>
// License: GPL
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
#include <klocale.h>
#include <kstdaccel.h>
#include <kwin.h>
#include <kaction.h>
#include <kiconloader.h>

#include "kmcommands.h"
#include "kmenubar.h"
#include "kpopupmenu.h"
#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"

#include "kmreadermainwin.h"
#include "kmreadermainwin.moc"

KMReaderMainWin::KMReaderMainWin( bool htmlOverride, char *name )
  : KMail::SecondaryWindow( name ), mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  //mReaderWin->setShowCompleteMessage( true );
  mReaderWin->setAutoDelete( true );
  mReaderWin->setHtmlOverride( htmlOverride );
  setCentralWidget( mReaderWin );
  setupAccel();

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
}


KMReaderMainWin::KMReaderMainWin( char *name )
  : KMail::SecondaryWindow( name ), mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  mReaderWin->setAutoDelete( true );
  setCentralWidget( mReaderWin );
  setupAccel();

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
}


KMReaderMainWin::KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QTextCodec *codec, char *name )
  : KMail::SecondaryWindow( name ), mMsg( 0 )
{
  resize( 550, 600 );
  mReaderWin = new KMReaderWin( this, this, actionCollection() ); //new reader
  mReaderWin->setOverrideCodec( codec );
  mReaderWin->setMsgPart( aMsgPart, aHTML, aFileName, pname );
  setCentralWidget( mReaderWin );
  setupAccel();

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
}


KMReaderMainWin::~KMReaderMainWin()
{
  saveMainWindowSettings(KMKernel::config(), "Separate Reader Window");
}


void KMReaderMainWin::showMsg( const QTextCodec *codec, KMMessage *msg )
{
  mReaderWin->setOverrideCodec( codec );
  mReaderWin->setMsg( msg, true );
  setCaption( msg->subject() );
  mMsg = msg;
  toolBar( "mainToolBar" )->show();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotPrintMsg()
{
  KMCommand *command = new KMPrintCommand( this, mReaderWin->message(),
      mReaderWin->htmlOverride(), mReaderWin->overrideCodec() );
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
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );
  KStdAction::close( this, SLOT( close() ), actionCollection() );
  applyMainWindowSettings(KMKernel::config(), "Separate Reader Window");
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

  mForwardActionMenu = new KActionMenu( i18n("Message->","&Forward"),
					"mail_forward", actionCollection(),
					"message_forward" );

  mForwardAction = new KAction( i18n("&Inline..."), "mail_forward",
				SHIFT+Key_F, this, SLOT(slotForwardMsg()),
				actionCollection(), "message_forward" );
  mForwardActionMenu->insert( mForwardAction );

  mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", Key_F, this,
					SLOT(slotForwardAttachedMsg()), actionCollection(),
					"message_forward_as_attachment" );
  mForwardActionMenu->insert( mForwardAttachedAction );

  mRedirectAction = new KAction( i18n("Message->Forward->","&Redirect..."),
				 Key_E, this, SLOT(slotRedirectMsg()),
				 actionCollection(), "message_forward_redirect" );
  mForwardActionMenu->insert( mRedirectAction );

  mReplyActionMenu = new KActionMenu( i18n("Message->","&Reply"),
                                      "mail_reply", actionCollection(),
                                      "message_reply_menu" );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
	   SLOT(slotReplyToMsg()) );

  mReplyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
			      SLOT(slotReplyToMsg()), actionCollection(), "reply" );
  mReplyActionMenu->insert( mReplyAction );

  mReplyAuthorAction = new KAction( i18n("Reply to A&uthor..."), "mail_reply",
                                    SHIFT+Key_A, this,
                                    SLOT(slotReplyAuthorToMsg()),
                                    actionCollection(), "reply_author" );
  mReplyActionMenu->insert( mReplyAuthorAction );

  mReplyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
				 Key_A, this, SLOT(slotReplyAllToMsg()),
				 actionCollection(), "reply_all" );
  mReplyActionMenu->insert( mReplyAllAction );

  mReplyListAction = new KAction( i18n("Reply to Mailing-&List..."),
				  "mail_replylist", Key_L, this,
				  SLOT(slotReplyListToMsg()), actionCollection(),
				  "reply_list" );
  mReplyActionMenu->insert( mReplyListAction );

  mPrintAction = KStdAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  mViewSourceAction = new KAction( i18n("&View Source"), Key_V, this,
                                   SLOT(slotShowMsgSrc()), actionCollection(),
                                   "view_source" );

  createGUI( "kmreadermainwin.rc" );
  menuBar()->hide();
  toolBar( "mainToolBar" )->hide();
}


void KMReaderMainWin::slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu * menu = new KPopupMenu;
  mUrl = aUrl;
  mMsg = &aMsg;

  if (!aUrl.isEmpty()) {
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
      mReaderWin->urlSaveAsAction()->plug( menu );
      mReaderWin->copyURLAction()->plug( menu );
      mReaderWin->addBookmarksAction()->plug( menu );
    }
  } else {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMsg) // no message
    {
      delete menu;
      return;
    }

    mReplyAction->plug( menu );
    mReplyAllAction->plug( menu );
    mReplyAuthorAction->plug( menu );
    mReplyListAction->plug( menu );
    mForwardActionMenu->plug( menu );

    menu->insertSeparator();

    QPopupMenu* copyMenu = new QPopupMenu(menu);
    KMMainWidget* mainwin = kmkernel->getKMMainWidget();
    if ( mainwin )
      mainwin->folderTree()->folderToPopupMenu( false, this, &mMenuToFolder, copyMenu );
    menu->insertItem( i18n("&Copy To" ), copyMenu );
    menu->insertSeparator();
    mReaderWin->toggleFixFontAction()->plug( menu );
    mViewSourceAction->plug( menu );

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
