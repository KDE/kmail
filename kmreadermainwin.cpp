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
#include <KAcceleratorManager>
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

#include <KActionCollection>
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
  QString caption;
  if ( message ) {
    caption = message->subject()->asUnicodeString();
  }
  if(mParentCollection.isValid()) {
    caption += QLatin1String(" - ");
    caption += MailCommon::Util::fullCollectionPath( mParentCollection );
  }
  if(!caption.isEmpty()) {
    setCaption(caption);
  }
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

  //----- Message Menu

  mFontAction = new KFontAction( i18n("Select Font"), this );
  actionCollection()->addAction( "text_font", mFontAction );
  mFontAction->setFont( mReaderWin->cssHelper()->bodyFont().family() );
  connect( mFontAction, SIGNAL(triggered(QString)),
           SLOT(slotFontAction(QString)) );
  mFontSizeAction = new KFontSizeAction( i18n( "Select Size" ), this );
  mFontSizeAction->setFontSize( mReaderWin->cssHelper()->bodyFont().pointSize() );
  actionCollection()->addAction( "text_size", mFontSizeAction );
  connect( mFontSizeAction, SIGNAL(fontSizeChanged(int)),
           SLOT(slotSizeAction(int)) );


  connect( mReaderWin->viewer(), SIGNAL(popupMenu(Akonadi::Item,KUrl,KUrl,QPoint)),
           this, SLOT(slotMessagePopup(Akonadi::Item,KUrl,KUrl,QPoint)) );

  setStandardToolBarMenuEnabled(true);
  KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
}

//-----------------------------------------------------------------------------
KAction *KMReaderMainWin::copyActionMenu(QMenu *menu)
{
  KMMainWidget* mainwin = kmkernel->getKMMainWidget();
  if ( mainwin )
  {
    KActionMenu *action = new KActionMenu( menu );
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
  mMsg = aMsg;

  const QString email =  KPIMUtils::firstEmailAddress( aUrl.path() ).toLower();
  if ( aUrl.protocol() == "mailto" && !email.isEmpty()) {
    Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob( this );
    job->setLimit( 1 );
    job->setQuery( Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch );
    job->setProperty( "msg", QVariant::fromValue( mMsg ) );
    job->setProperty( "point", aPoint );
    job->setProperty( "imageUrl", imageUrl );
    job->setProperty( "url", aUrl );
    connect( job, SIGNAL(result(KJob*)), SLOT(slotContactSearchJobForMessagePopupDone(KJob*)) );
  } else {
    showMessagePopup(mMsg, aUrl, imageUrl, aPoint, false, false);
  }
}

void KMReaderMainWin::slotContactSearchJobForMessagePopupDone( KJob *job )
{
  const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
  const bool contactAlreadyExists = !searchJob->contacts().isEmpty();

  const QList<Akonadi::Item> listContact = searchJob->items();
  const bool uniqueContactFound = (listContact.count() == 1);
  if(uniqueContactFound) {
      mReaderWin->setContactItem(listContact.first());
  } else {
      mReaderWin->setContactItem(Akonadi::Item());
  }

  const Akonadi::Item msg = job->property( "msg" ).value<Akonadi::Item>();
  const QPoint aPoint = job->property( "point" ).toPoint();
  const KUrl imageUrl = job->property("imageUrl").value<KUrl>();
  const KUrl url = job->property("url").value<KUrl>();

  showMessagePopup(msg, url, imageUrl, aPoint, contactAlreadyExists, uniqueContactFound);
}

void KMReaderMainWin::showMessagePopup(const Akonadi::Item&msg ,const KUrl&url,const KUrl &imageUrl,const QPoint& aPoint, bool contactAlreadyExists, bool uniqueContactFound)
{
  KMenu *menu = 0;

  bool urlMenuAdded = false;
  bool copyAdded = false;
  const bool messageHasPayload = msg.hasPayload<KMime::Message::Ptr>();
  if ( !url.isEmpty() ) {
    if ( url.protocol() == QLatin1String( "mailto" ) ) {
      // popup on a mailto URL
      menu = new KMenu;
      menu->addAction( mReaderWin->mailToComposeAction() );
      if ( messageHasPayload ) {
        menu->addAction( mReaderWin->mailToReplyAction() );
        menu->addAction( mReaderWin->mailToForwardAction() );
        menu->addSeparator();
      }

      if ( contactAlreadyExists ) {
        if(uniqueContactFound) {
          menu->addAction( mReaderWin->editContactAction() );
        } else {
          menu->addAction( mReaderWin->openAddrBookAction() );
        }
      } else {
        menu->addAction( mReaderWin->addAddrBookAction() );
      }

      menu->addAction( mReaderWin->copyURLAction() );
      copyAdded = true;
      urlMenuAdded = true;
    } else if( url.protocol() != QLatin1String( "attachment" ) ){
      // popup on a not-mailto URL
      if(!menu)
        menu = new KMenu;
      menu->addAction( mReaderWin->urlOpenAction() );
      menu->addAction( mReaderWin->addBookmarksAction() );
      menu->addAction( mReaderWin->urlSaveAsAction() );
      menu->addAction( mReaderWin->copyURLAction() );
      if(!imageUrl.isEmpty()) {
        menu->addSeparator();
        menu->addAction( mReaderWin->copyImageLocation());
        menu->addAction(mReaderWin->downloadImageToDiskAction());
      }
      urlMenuAdded = true;
    }
  }
  const QString selectedText(mReaderWin->copyText());
  if ( !selectedText.isEmpty() ) {
    if(!menu)
      menu = new KMenu;
    if ( urlMenuAdded ) {
      menu->addSeparator();
    }
    if(messageHasPayload) {
      menu->addAction( mMsgActions->replyMenu() );
      menu->addSeparator();
    }
    if( !copyAdded )
      menu->addAction( mReaderWin->copyAction() );
    menu->addAction( mReaderWin->selectAllAction() );
    menu->addSeparator();
    mMsgActions->addWebShortcutsMenu(menu,selectedText);
    menu->addSeparator();
    menu->addAction(mReaderWin->translateAction());
    menu->addSeparator();
    menu->addAction( mReaderWin->speakTextAction());
  } else if ( !urlMenuAdded ) {
    if(!menu)
      menu = new KMenu;

    // popup somewhere else (i.e., not a URL) on the message
    if (messageHasPayload) {
      bool replyForwardMenu = false;
      Akonadi::Collection col = parentCollection();
      if ( col.isValid() ) {
        if ( ! ( CommonKernel->folderIsSentMailFolder( col ) ||
                 CommonKernel->folderIsDrafts( col ) ||
                 CommonKernel->folderIsTemplates( col ) ) ) {
          replyForwardMenu = true;
        }
      } else if ( messageHasPayload ) {
        replyForwardMenu = true;
      }
      if ( replyForwardMenu ) {
          // add the reply and forward actions only if we are not in a sent-mail,
          // templates or drafts folder
          menu->addAction( mMsgActions->replyMenu() );
          menu->addAction( mMsgActions->forwardMenu() );
          menu->addSeparator();
      }
      menu->addAction( copyActionMenu(menu) );

      menu->addSeparator();
      if(!imageUrl.isEmpty()) {
        menu->addSeparator();
        menu->addAction( mReaderWin->copyImageLocation());
        menu->addAction( mReaderWin->downloadImageToDiskAction());
        menu->addSeparator();
      }

      menu->addAction( mReaderWin->viewSourceAction() );
      menu->addAction( mReaderWin->toggleFixFontAction() );
      menu->addAction( mReaderWin->toggleMimePartTreeAction() );
      menu->addAction( mReaderWin->saveAsAction() );
      menu->addAction( mSaveAtmAction );
      if ( msg.isValid() ) {
        menu->addSeparator();
        menu->addAction( mMsgActions->createTodoAction() );
      }
    } else {
      menu->addAction( mReaderWin->toggleFixFontAction() );
      menu->addAction( mReaderWin->toggleMimePartTreeAction() );
    }
  }
  if (menu) {
    KAcceleratorManager::manage(menu);
    menu->exec( aPoint, 0 );
    delete menu;
  }
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
