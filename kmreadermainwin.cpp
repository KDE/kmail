/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>
    Copyright (c) 2011 Montel Laurent <montel@kde.org>

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
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include <KFontAction>
#include <KFontSizeAction>
#include <kstatusbar.h>
#include <KMessageBox>
#include "kmcommands.h"
#include "kmenubar.h"
#include "kmenu.h"
#include "kmmainwidget.h"
#include "messageviewer/csshelper.h"
#include "customtemplatesmenu.h"
#include "messageactions.h"
#include "util.h"
#include "mailkernel.h"
#include "foldercollection.h"

#include <akonadi/contact/contactsearchjob.h>
#include <kpimutils/email.h>
#include <kmime/kmime_message.h>

#include <messageviewer/viewer.h>
#include <akonadi/item.h>
#include <akonadi/itemcopyjob.h>
#include <akonadi/itemcreatejob.h>

#include "messagecore/messagehelpers.h"
#include <mailutil.h>

using namespace MailCommon;

KMReaderMainWin::KMReaderMainWin( bool htmlOverride, bool htmlLoadExtOverride,
                                  char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  //mReaderWin->setShowCompleteMessage( true );
  mReaderWin->setHtmlOverride( htmlOverride );
  mReaderWin->setHtmlLoadExtOverride( htmlLoadExtOverride );
  mReaderWin->setDecryptMessageOverwrite( true );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
KMReaderMainWin::KMReaderMainWin( char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
KMReaderMainWin::KMReaderMainWin(KMime::Content* aMsgPart, bool aHTML, const QString & encoding, char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setHtmlOverride( aHTML );
  mReaderWin->setMsgPart( aMsgPart );
  initKMReaderMainWin();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::initKMReaderMainWin()
{
  setCentralWidget( mReaderWin );
  setupAccel();
  setupGUI( Keys | StatusBar | Create, "kmreadermainwin.rc" );
  mMsgActions->setupForwardingActionsList( this );
  applyMainWindowSettings( KMKernel::self()->config()->group( "Separate Reader Window" ) );
  if( ! mReaderWin->message().isValid() ) {
    menuBar()->hide();
    toolBar( "mainToolBar" )->hide();
  }

  connect( kmkernel, SIGNAL(configChanged()),
           this, SLOT(slotConfigChanged()) );
  connect( mReaderWin, SIGNAL(showStatusBarMessage(QString)),
           statusBar(), SLOT(showMessage(QString)) );
}

//-----------------------------------------------------------------------------
KMReaderMainWin::~KMReaderMainWin()
{
  saveMainWindowSettings( KMKernel::self()->config()->group( "Separate Reader Window" ) );
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::setUseFixedFont( bool useFixedFont )
{
  mReaderWin->setUseFixedFont( useFixedFont );
}

void KMReaderMainWin::showMessage( const QString & encoding, const Akonadi::Item &msg, const Akonadi::Collection& parentCollection )
{

  mParentCollection = parentCollection;
  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setMessage( msg, MessageViewer::Viewer::Force );
  KMime::Message::Ptr message = MessageCore::Util::message( msg );
  if ( message )
    setCaption( message->subject()->asUnicodeString() );
  mMsg = msg;
  mMsgActions->setCurrentMessage( msg );

  const bool canChange = mParentCollection.isValid() ? (bool)( mParentCollection.rights() & Akonadi::Collection::CanDeleteItem ) : false;
  mTrashAction->setEnabled( canChange );

  menuBar()->show();
  toolBar( "mainToolBar" )->show();
}

void KMReaderMainWin::showMessage( const QString& encoding, KMime::Message::Ptr message )
{
  if ( !message )
    return;

  Akonadi::Item item;

  item.setPayload<KMime::Message::Ptr>( message );
  item.setMimeType( KMime::Message::mimeType() );

  mMsg = item;
  mMsgActions->setCurrentMessage( item );

  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setMessage( message );
  setCaption( message->subject()->asUnicodeString() );

  mTrashAction->setEnabled( false );

  menuBar()->show();
  toolBar( "mainToolBar" )->show();
}


void KMReaderMainWin::slotReplyOrForwardFinished()
{
  if ( GlobalSettings::self()->closeAfterReplyOrForward() ) {
    close();
  }
}

Akonadi::Collection KMReaderMainWin::parentCollection() const
{
  if ( mParentCollection.isValid() )
    return mParentCollection;
  else
    return mMsg.parentCollection();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotTrashMsg()
{
  if ( !mMsg.isValid() )
    return;
  KMTrashMsgCommand *command = new KMTrashMsgCommand( parentCollection(), mMsg, -1 );
  command->start();
  close();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardInlineMsg()
{
  if ( !mReaderWin->message().hasPayload<KMime::Message::Ptr>() ) return;
   KMCommand *command = 0;
   const Akonadi::Collection parentCol = mReaderWin->message().parentCollection();
   if ( parentCol.isValid() ) {
     QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( parentCol, false );
     if ( fd )
       command = new KMForwardCommand( this, mReaderWin->message(),
                                       fd->identity() );
     else
       command = new KMForwardCommand( this, mReaderWin->message() );
   } else {
    command = new KMForwardCommand( this, mReaderWin->message() );
   }
   connect( command, SIGNAL(completed(KMCommand*)),
            this, SLOT(slotReplyOrForwardFinished()) );
   command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardAttachedMsg()
{
  if ( !mReaderWin->message().hasPayload<KMime::Message::Ptr>() ) return;
   KMCommand *command = 0;
   const Akonadi::Collection parentCol = mReaderWin->message().parentCollection();
   if ( parentCol.isValid() ) {
     QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( parentCol, false );
     if ( fd )
       command = new KMForwardAttachedCommand( this, mReaderWin->message(),
                                               fd->identity() );
     else
       command = new KMForwardAttachedCommand( this, mReaderWin->message() );
   }
   else
     command = new KMForwardAttachedCommand( this, mReaderWin->message() );

   connect( command, SIGNAL(completed(KMCommand*)),
            this, SLOT(slotReplyOrForwardFinished()) );
   command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotRedirectMsg()
{
  if ( !mReaderWin->message().hasPayload<KMime::Message::Ptr>() ) return;
  KMCommand *command = new KMRedirectCommand( this, mReaderWin->message() );
  connect( command, SIGNAL(completed(KMCommand*)),
         this, SLOT(slotReplyOrForwardFinished()) );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomReplyToMsg( const QString &tmpl )
{
  if ( !mReaderWin->message().hasPayload<KMime::Message::Ptr>() ) return;
  KMCommand *command = new KMReplyCommand( this,
                                           mReaderWin->message(),
                                           MessageComposer::ReplySmart,
                                           mReaderWin->copyText(),
                                           false, tmpl );
  connect( command, SIGNAL(completed(KMCommand*)),
           this, SLOT(slotReplyOrForwardFinished()) );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomReplyAllToMsg( const QString &tmpl )
{
  if ( !mReaderWin->message().hasPayload<KMime::Message::Ptr>() ) return;
  KMCommand *command = new KMReplyCommand( this,
                                           mReaderWin->message(),
                                           MessageComposer::ReplyAll,
                                           mReaderWin->copyText(),
                                           false, tmpl );
  connect( command, SIGNAL(completed(KMCommand*)),
           this, SLOT(slotReplyOrForwardFinished()) );

  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotCustomForwardMsg( const QString &tmpl)
{
  if ( !mReaderWin->message().hasPayload<KMime::Message::Ptr>() ) return;
  KMCommand *command = new KMForwardCommand( this,
                                             mReaderWin->message(),
                                             0, tmpl );
  connect( command, SIGNAL(completed(KMCommand*)),
           this, SLOT(slotReplyOrForwardFinished()) );

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
  connect( mMsgActions, SIGNAL(replyActionFinished()),
           this, SLOT(slotReplyOrForwardFinished()) );

  //----- File Menu
  mSaveAsAction = KStandardAction::saveAs( mReaderWin->viewer(), SLOT(slotSaveMessage()),
                                           actionCollection() );
  mSaveAsAction->setShortcut( KStandardShortcut::shortcut( KStandardShortcut::Save ) );

  mSaveAtmAction  = new KAction(KIcon("mail-attachment"), i18n("Save A&ttachments..."), actionCollection() );
  connect( mSaveAtmAction, SIGNAL(triggered(bool)), mReaderWin->viewer(), SLOT(slotAttachmentSaveAll()) );

  mTrashAction = new KAction( KIcon( "user-trash" ), i18n("&Move to Trash"), this );
  mTrashAction->setIconText( i18nc( "@action:intoolbar Move to Trash", "Trash" ) );
  mTrashAction->setHelpText( i18n( "Move message to trashcan" ) );
  mTrashAction->setShortcut( QKeySequence( Qt::Key_Delete ) );
  actionCollection()->addAction( "move_to_trash", mTrashAction );
  connect( mTrashAction, SIGNAL(triggered()), this, SLOT(slotTrashMsg()) );

  KAction *closeAction = KStandardAction::close( this, SLOT(close()), actionCollection() );
  KShortcut closeShortcut = KShortcut(closeAction->shortcuts());
  closeShortcut.setAlternate( QKeySequence(Qt::Key_Escape));
  closeAction->setShortcuts(closeShortcut);

  //----- View Menu
  mViewSourceAction  = new KAction(i18n("&View Source"), this);
  actionCollection()->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool)), mReaderWin->viewer(), SLOT(slotShowMessageSource()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  //----- Message Menu

  fontAction = new KFontAction( i18n("Select Font"), this );
  actionCollection()->addAction( "text_font", fontAction );
  fontAction->setFont( mReaderWin->cssHelper()->bodyFont().family() );
  connect( fontAction, SIGNAL(triggered(QString)),
           SLOT(slotFontAction(QString)) );
  fontSizeAction = new KFontSizeAction( i18n( "Select Size" ), this );
  fontSizeAction->setFontSize( mReaderWin->cssHelper()->bodyFont().pointSize() );
  actionCollection()->addAction( "text_size", fontSizeAction );
  connect( fontSizeAction, SIGNAL(fontSizeChanged(int)),
           SLOT(slotSizeAction(int)) );


  connect( mReaderWin->viewer(), SIGNAL(popupMenu(Akonadi::Item,KUrl,KUrl,QPixmap,QPoint)),
           this, SLOT(slotMessagePopup(Akonadi::Item,KUrl,KUrl,QPixmap,QPoint)) );

  setStandardToolBarMenuEnabled(true);
  KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
}

//-----------------------------------------------------------------------------
KAction *KMReaderMainWin::copyActionMenu()
{
  KMMainWidget* mainwin = kmkernel->getKMMainWidget();
  if ( mainwin )
  {
    KActionMenu *action = new KActionMenu( this );
    action->setIcon( KIcon( "edit-copy") );
    action->setText( i18n("Copy Item To...") );
    mainwin->standardMailActionManager()->standardActionManager()->createActionFolderMenu( action->menu(), Akonadi::StandardActionManager::CopyItemToMenu );
    connect( action->menu(), SIGNAL(triggered(QAction*)), SLOT(slotCopyItem(QAction*)) );

    return action;
  }
  return 0;

}

void KMReaderMainWin::slotCopyItem(QAction*action)
{
  if ( action )
  {
    const QModelIndex index = action->data().value<QModelIndex>();
    QAbstractItemModel *model = const_cast<QAbstractItemModel *>( index.model() );
    const Akonadi::Collection collection = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();

    if ( mMsg.isValid() ) {
      Akonadi::ItemCopyJob *job = new Akonadi::ItemCopyJob( mMsg, collection,this );
      connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCopyResult(KJob*)) );
    }
    else
    {
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( mMsg, collection, this );
      connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCopyResult(KJob*)) );
    }
  }
}

void KMReaderMainWin::slotCopyResult( KJob * job )
{
  if ( job->error() ) {
    KMessageBox::sorry( this, i18n("Can not copy item. %1", job->errorString() ) );
  }
}

void KMReaderMainWin::slotMessagePopup(const Akonadi::Item&aMsg , const KUrl&aUrl, const KUrl&imageUrl, const QPoint& aPoint)
{
  mUrl = aUrl;
  mMsg = aMsg;

  const QString email =  KPIMUtils::firstEmailAddress( aUrl.path() );
  Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob( this );
  job->setLimit( 1 );
  job->setQuery( Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch );
  job->setProperty( "point", aPoint );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDelayedMessagePopup(KJob*)) );
}

void KMReaderMainWin::slotDelayedMessagePopup( KJob *job )
{
  const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
  const bool contactAlreadyExists = !searchJob->contacts().isEmpty();

  const QPoint aPoint = job->property( "point" ).toPoint();
  KMenu *menu = new KMenu;

  bool urlMenuAdded = false;
  bool copyAdded = false;
  if ( !mUrl.isEmpty() ) {
    if ( mUrl.protocol() == QLatin1String( "mailto" ) ) {
      // popup on a mailto URL
      menu->addAction( mReaderWin->mailToComposeAction() );
      if ( mMsg.hasPayload<KMime::Message::Ptr>() ) {
        menu->addAction( mReaderWin->mailToReplyAction() );
        menu->addAction( mReaderWin->mailToForwardAction() );
        menu->addSeparator();
      }

      if ( contactAlreadyExists ) {
        menu->addAction( mReaderWin->openAddrBookAction() );
      } else {
        menu->addAction( mReaderWin->addAddrBookAction() );
      }

      menu->addAction( mReaderWin->copyURLAction() );
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
    menu->addSeparator();
    menu->addAction( mReaderWin->speakTextAction());
  } else if ( !urlMenuAdded ) {
    // popup somewhere else (i.e., not a URL) on the message
    if (!mMsg.hasPayload<KMime::Message::Ptr>() ) {
      // no message
      delete menu;
      return;
    }
    bool replyForwardMenu = false;
    Akonadi::Collection col = parentCollection();
    if ( col.isValid() ) {
      if ( ! ( CommonKernel->folderIsSentMailFolder( col ) ||
               CommonKernel->folderIsDrafts( col ) ||
               CommonKernel->folderIsTemplates( col ) ) ) {
        replyForwardMenu = true;
      }
    } else if ( mMsg.hasPayload<KMime::Message::Ptr>() ) {
      replyForwardMenu = true;
    }
    if ( replyForwardMenu ) {
        // add the reply and forward actions only if we are not in a sent-mail,
        // templates or drafts folder
        menu->addAction( mMsgActions->replyMenu() );
        menu->addAction( mMsgActions->forwardMenu() );
        menu->addSeparator();
    }
    menu->addAction( copyActionMenu() );

    menu->addSeparator();
    menu->addAction( mViewSourceAction );
    menu->addAction( mReaderWin->toggleFixFontAction() );
    menu->addAction( mReaderWin->toggleMimePartTreeAction() );
    menu->addSeparator();
    menu->addAction( mMsgActions->printAction() );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtmAction );
    if ( mMsg.isValid() ) {
      menu->addSeparator();
      menu->addAction( mMsgActions->createTodoAction() );
    }
  }
  menu->exec( aPoint, 0 );
  delete menu;
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

void KMReaderMainWin::slotEditToolbars()
{
  saveMainWindowSettings( KConfigGroup(KMKernel::self()->config(), "ReaderWindow") );
  KEditToolBar dlg( guiFactory(), this );
  connect( &dlg, SIGNAL(newToolBarConfig()), SLOT(slotUpdateToolbars()) );
  dlg.exec();
}

void KMReaderMainWin::slotUpdateToolbars()
{
  createGUI("kmreadermainwin.rc");
  applyMainWindowSettings( KConfigGroup(KMKernel::self()->config(), "ReaderWindow") );
}


#include "kmreadermainwin.moc"
