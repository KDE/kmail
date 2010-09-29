/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "messageactions.h"

#include "globalsettings.h"
#include "kmreaderwin.h"
#include "kmkernel.h"
#include "mailkernel.h"
#include "kmmainwidget.h"
#include "util.h"

#include "messagecore/annotationdialog.h"
#include "messagecore/mailinglist-magic.h"
#include "messagecore/globalsettings.h"
#include "messagecore/messagehelpers.h"
#include "messageviewer/csshelper.h"
#include "messageviewer/globalsettings.h"

#include <Nepomuk/Resource>

#include <KAction>
#include <KActionMenu>
#include <KActionCollection>
#include <KToggleAction>
#include <KDebug>
#include <KLocale>
#include <KXMLGUIClient>
#include <KStandardDirs>
#include <KRun>
#include <KMenu>

#include <QVariant>
#include <qwidget.h>
#include <akonadi/collection.h>
#include <Akonadi/Monitor>
#include <mailutil.h>

using namespace KMail;

MessageActions::MessageActions( KActionCollection *ac, QWidget* parent ) :
    QObject( parent ),
    mParent( parent ),
    mActionCollection( ac ),
    mMessageView( 0 ),
    mRedirectAction( 0 )
{
  mReplyActionMenu = new KActionMenu( KIcon("mail-reply-sender"), i18nc("Message->","&Reply"), this );
  mActionCollection->addAction( "message_reply_menu", mReplyActionMenu );
  connect( mReplyActionMenu, SIGNAL(triggered(bool)),
           this, SLOT(slotReplyToMsg()) );

  mReplyAction = new KAction( KIcon("mail-reply-sender"), i18n("&Reply..."), this );
  mActionCollection->addAction( "reply", mReplyAction );
  mReplyAction->setShortcut(Qt::Key_R);
  connect( mReplyAction, SIGNAL(triggered(bool)),
           this, SLOT(slotReplyToMsg()) );
  mReplyActionMenu->addAction( mReplyAction );

  mReplyAuthorAction = new KAction( KIcon("mail-reply-sender"), i18n("Reply to A&uthor..."), this );
  mActionCollection->addAction( "reply_author", mReplyAuthorAction );
  mReplyAuthorAction->setShortcut(Qt::SHIFT+Qt::Key_A);
  connect( mReplyAuthorAction, SIGNAL(triggered(bool)),
           this, SLOT(slotReplyAuthorToMsg()) );
  mReplyActionMenu->addAction( mReplyAuthorAction );

  mReplyAllAction = new KAction( KIcon("mail-reply-all"), i18n("Reply to &All..."), this );
  mActionCollection->addAction( "reply_all", mReplyAllAction );
  mReplyAllAction->setShortcut( Qt::Key_A );
  connect( mReplyAllAction, SIGNAL(triggered(bool)),
           this, SLOT(slotReplyAllToMsg()) );
  mReplyActionMenu->addAction( mReplyAllAction );

  mReplyListAction = new KAction( KIcon("mail-reply-list"), i18n("Reply to Mailing-&List..."), this );
  mActionCollection->addAction( "reply_list", mReplyListAction );
  mReplyListAction->setShortcut( Qt::Key_L );
  connect( mReplyListAction, SIGNAL(triggered(bool)),
           this, SLOT(slotReplyListToMsg()) );
  mReplyActionMenu->addAction( mReplyListAction );

  mNoQuoteReplyAction = new KAction( i18n("Reply Without &Quote..."), this );
  mActionCollection->addAction( "noquotereply", mNoQuoteReplyAction );
  mNoQuoteReplyAction->setShortcut( Qt::SHIFT+Qt::Key_R );
  connect( mNoQuoteReplyAction, SIGNAL(triggered(bool)),
           this, SLOT(slotNoQuoteReplyToMsg()) );


  mCreateTodoAction = new KAction( KIcon( "task-new" ), i18n( "Create To-do/Reminder..." ), this );
  mCreateTodoAction->setIconText( i18n( "Create To-do" ) );
  mCreateTodoAction->setHelpText( i18n( "Allows you to create a calendar to-do or reminder from this message" ) );
  mCreateTodoAction->setWhatsThis( i18n( "This option starts the KOrganizer to-do editor with initial values taken from the currently selected message. Then you can edit the to-do to your liking before saving it to your calendar." ) );
  mActionCollection->addAction( "create_todo", mCreateTodoAction );
  connect( mCreateTodoAction, SIGNAL(triggered(bool)),
           this, SLOT(slotCreateTodo()) );
  mKorganizerIsOnSystem = !KStandardDirs::findExe("korganizer").isEmpty();
  mCreateTodoAction->setEnabled( mKorganizerIsOnSystem );

  mStatusMenu = new KActionMenu ( i18n( "Mar&k Message" ), this );
  mActionCollection->addAction( "set_status", mStatusMenu );


  KMMainWidget* mainwin = kmkernel->getKMMainWidget();
  if ( mainwin ) {
    KAction * action = mainwin->akonadiStandardAction( Akonadi::StandardMailActionManager::MarkMailAsRead );
    mStatusMenu->addAction( action );

    action = mainwin->akonadiStandardAction( Akonadi::StandardMailActionManager::MarkMailAsUnread );
    mStatusMenu->addAction( action );
    mStatusMenu->addSeparator();
  }

  mToggleFlagAction = new KToggleAction( KIcon("mail-mark-important"),
                                         i18n("Mark Message as &Important"), this );
  mToggleFlagAction->setShortcut( Qt::CTRL + Qt::Key_I );
  connect( mToggleFlagAction, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusFlag()) );
  mToggleFlagAction->setCheckedState( KGuiItem(i18n("Remove &Important Message Mark")) );
  mToggleFlagAction->setIconText( i18n( "Important" ) );
  mActionCollection->addAction( "status_flag", mToggleFlagAction );
  mStatusMenu->addAction( mToggleFlagAction );

  mToggleToActAction = new KToggleAction( KIcon("mail-mark-task"),
                                          i18n("Mark Message as &Action Item"), this );
  connect( mToggleToActAction, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusToAct()) );
  mToggleToActAction->setCheckedState( KGuiItem(i18n("Remove &Action Item Message Mark")) );
  mToggleToActAction->setIconText( i18n( "Action Item" ) );
  mActionCollection->addAction( "status_toact", mToggleToActAction );
  mStatusMenu->addAction( mToggleToActAction );

  mEditAction = new KAction( KIcon("accessories-text-editor"), i18n("&Edit Message"), this );
  mActionCollection->addAction( "edit", mEditAction );
  connect( mEditAction, SIGNAL(triggered(bool)),
           this, SLOT(editCurrentMessage()) );
  mEditAction->setShortcut( Qt::Key_T );

  mAnnotateAction = new KAction( KIcon( "view-pim-notes" ), i18n( "Add Note..."), this );
  mActionCollection->addAction( "annotate", mAnnotateAction );
  connect( mAnnotateAction, SIGNAL(triggered(bool)),
           this, SLOT(annotateMessage()) );

  mPrintAction = KStandardAction::print( this, SLOT( slotPrintMsg() ), mActionCollection );

  mForwardActionMenu  = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  mActionCollection->addAction("message_forward", mForwardActionMenu );

  mForwardAttachedAction = new KAction( KIcon("mail-forward"),
                                        i18nc( "@action:inmenu Message->Forward->",
                                               "As &Attachment..." ),
                                        this );
  connect( mForwardAttachedAction, SIGNAL( triggered( bool ) ),
           parent, SLOT( slotForwardAttachedMsg() ) );
  mActionCollection->addAction( "message_forward_as_attachment", mForwardAttachedAction );

  mForwardInlineAction = new KAction( KIcon( "mail-forward" ),
                                       i18nc( "@action:inmenu Message->Forward->",
                                              "&Inline..." ),
                                       this );
  connect( mForwardInlineAction, SIGNAL( triggered( bool ) ),
           parent, SLOT( slotForwardInlineMsg() ) );
  mActionCollection->addAction( "message_forward_inline", mForwardInlineAction );

  setupForwardActions();

  mRedirectAction  = new KAction(i18nc("Message->Forward->", "&Redirect..."), this );
  mActionCollection->addAction( "message_forward_redirect", mRedirectAction );
  connect( mRedirectAction, SIGNAL( triggered( bool ) ),
           parent, SLOT( slotRedirectMsg() ) );
  mRedirectAction->setShortcut( QKeySequence( Qt::Key_E ) );
  mForwardActionMenu->addAction( mRedirectAction );

  //FIXME add KIcon("mail-list") as first arguement. Icon can be derived from
  // mail-reply-list icon by removing top layers off svg
  mMailingListActionMenu = new KActionMenu( i18nc( "Message->", "Mailing-&List" ), this );
  connect( mMailingListActionMenu->menu(), SIGNAL(triggered(QAction *)),
         this, SLOT(slotRunUrl(QAction *)) );
  mActionCollection->addAction( "message_list", mMailingListActionMenu );

  mMonitor = new Akonadi::Monitor( this );
  //FIXME: Attachment fetching is not needed here, but on-demand loading is not supported ATM
  mMonitor->itemFetchScope().fetchFullPayload();
  connect( mMonitor, SIGNAL(itemChanged( Akonadi::Item, QSet<QByteArray> )), SLOT(slotItemModified( Akonadi::Item, QSet<QByteArray> )));

  updateActions();
}

MessageActions::~MessageActions()
{
}

void MessageActions::setCurrentMessage( const Akonadi::Item &msg )
{
  mMonitor->setItemMonitored( mCurrentItem, false );
  mCurrentItem = msg;

  if ( !msg.isValid() ) {
    mSelectedItems.clear();
    mVisibleItems.clear();
  }

  mMonitor->setItemMonitored( mCurrentItem, true );
  updateActions();
}

void MessageActions::slotItemModified( const Akonadi::Item &  item, const QSet< QByteArray > &  partIdentifiers )
{
  if ( item.id() == mCurrentItem.id() && item.remoteId() == mCurrentItem.remoteId() )
    mCurrentItem = item;
  const int numberOfVisibleItems = mVisibleItems.count();
  for( int i = 0; i < numberOfVisibleItems; ++i ) {
    Akonadi::Item it = mVisibleItems[i];
    if ( item.id() == it.id() && item.remoteId() == it.remoteId() ) {
      mVisibleItems[i] = item;
    }
  }
  updateActions();
}



void MessageActions::setSelectedItem( const Akonadi::Item::List &items )
{
  mSelectedItems = items;
  updateActions();
}

void MessageActions::setSelectedVisibleItems( const Akonadi::Item::List &items )
{
  Q_FOREACH( const Akonadi::Item& item, mVisibleItems ) {
    mMonitor->setItemMonitored( item, false );
  }
  mVisibleItems = items;
  Q_FOREACH( const Akonadi::Item& item, items ) {
    mMonitor->setItemMonitored( item, true );
  }
  updateActions();
}

void MessageActions::updateActions()
{
  bool singleMsg = mCurrentItem.isValid();
  Akonadi::Collection parent;
  if ( singleMsg ) //=> valid
    parent = mCurrentItem.parentCollection();
  if ( parent.isValid() ) {
    if ( CommonKernel->folderIsTemplates(parent) )
      singleMsg = false;
  }

  const bool multiVisible = mVisibleItems.count() > 0 || mCurrentItem.isValid();
  const bool flagsAvailable = GlobalSettings::self()->allowLocalFlags()
                              || !(parent.isValid() ? parent.rights() & Akonadi::Collection::ReadOnly : true);

  mCreateTodoAction->setEnabled( singleMsg && mKorganizerIsOnSystem);
  mReplyActionMenu->setEnabled( singleMsg );
  mReplyAction->setEnabled( singleMsg );
  mNoQuoteReplyAction->setEnabled( singleMsg );
  mReplyAuthorAction->setEnabled( singleMsg );
  mReplyAllAction->setEnabled( singleMsg );
  mReplyListAction->setEnabled( singleMsg );
  mNoQuoteReplyAction->setEnabled( singleMsg );

  mAnnotateAction->setEnabled( singleMsg );
  updateAnnotateAction();

  mStatusMenu->setEnabled( multiVisible );
  mToggleFlagAction->setEnabled( flagsAvailable );
  mToggleToActAction->setEnabled( flagsAvailable );

  if ( mCurrentItem.hasPayload<KMime::Message::Ptr>() ) {
    Akonadi::MessageStatus status;
    status.setStatusFromFlags( mCurrentItem.flags() );
    mToggleToActAction->setChecked( status.isToAct() );
    mToggleFlagAction->setChecked( status.isImportant() );

    MessageCore::MailingList mailList;
    mailList = MessageCore::MailingList::detect( MessageCore::Util::message( mCurrentItem ) );

    if ( mailList.features() & ~MessageCore::MailingList::Id ) {
      // A mailing list menu with only a title is pretty boring
      // so make sure theres at least some content
      QString listId;
      if ( mailList.features() & MessageCore::MailingList::Id ) {
        // From a list-id in the form, "Birds of France <bof.yahoo.com>",
        // take "Birds of France" if it exists otherwise "bof.yahoo.com".
        listId = mailList.id();
        const int start = listId.indexOf( '<' );
        if ( start > 0 ) {
          listId.truncate( start - 1 );
        } else if ( start == 0 ) {
          const int end = listId.lastIndexOf( '>' );
          if ( end < 1 ) { // shouldn't happen but account for it anyway
            listId.remove( 0, 1 );
          } else {
            listId = listId.mid( 1, end-1 );
          }
        }
      }
      mMailingListActionMenu->menu()->clear();
      if ( !listId.isEmpty() )
        mMailingListActionMenu->menu()->addTitle( listId );

      if ( mailList.features() & MessageCore::MailingList::ArchivedAt )
        // IDEA: this may be something you want to copy - "Copy in submenu"?
        addMailingListAction( i18n( "Open Message in List Archive" ), KUrl( mailList.archivedAt() ) );
      if ( mailList.features() & MessageCore::MailingList::Post )
        addMailingListActions( i18n( "Post New Message" ), mailList.postURLS() );
      if ( mailList.features() & MessageCore::MailingList::Archive )
        addMailingListActions( i18n( "Go to Archive" ), mailList.archiveURLS() );
      if ( mailList.features() & MessageCore::MailingList::Help )
        addMailingListActions( i18n( "Request Help" ), mailList.helpURLS() );
      if ( mailList.features() & MessageCore::MailingList::Owner )
        addMailingListActions( i18n( "Contact Owner" ), mailList.ownerURLS() );
      if ( mailList.features() & MessageCore::MailingList::Subscribe )
        addMailingListActions( i18n( "Subscribe to List" ), mailList.subscribeURLS() );
      if ( mailList.features() & MessageCore::MailingList::Unsubscribe )
        addMailingListActions( i18n( "Unsubscribe from List" ), mailList.unsubscribeURLS() );
      mMailingListActionMenu->setEnabled( true );
    } else {
      mMailingListActionMenu->setEnabled( false );
    }
  }

  mEditAction->setEnabled( singleMsg );
}

template<typename T> void MessageActions::replyCommand()
{
  if ( !mCurrentItem.isValid() )
    return;
  const QString text = mMessageView ? mMessageView->copyText() : QString();
  KMCommand *command = new T( mParent, mCurrentItem, text );
  connect( command, SIGNAL( completed( KMCommand * ) ),
           this, SIGNAL( replyActionFinished() ) );
  command->start();
}

void MessageActions::slotCreateTodo()
{
  if ( !mCurrentItem.isValid() )
    return;

  MailCommon::Util::createTodoFromMail( mCurrentItem );
}

void MessageActions::setMessageView(KMReaderWin * msgView)
{
  mMessageView = msgView;
}

void MessageActions::setupForwardActions()
{
  disconnect( mForwardActionMenu, SIGNAL( triggered(bool) ), 0, 0 );
  mForwardActionMenu->removeAction( mForwardInlineAction );
  mForwardActionMenu->removeAction( mForwardAttachedAction );

  if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
    mForwardActionMenu->insertAction( mRedirectAction, mForwardInlineAction );
    mForwardActionMenu->insertAction( mRedirectAction, mForwardAttachedAction );
    mForwardInlineAction->setShortcut(QKeySequence(Qt::Key_F));
    mForwardAttachedAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F));
    QObject::connect( mForwardActionMenu, SIGNAL(triggered(bool)),
                      mParent, SLOT(slotForwardInlineMsg()) );
  }
  else {
    mForwardActionMenu->insertAction( mRedirectAction, mForwardAttachedAction );
    mForwardActionMenu->insertAction( mRedirectAction, mForwardInlineAction );
    mForwardInlineAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F));
    mForwardAttachedAction->setShortcut(QKeySequence(Qt::Key_F));
    QObject::connect( mForwardActionMenu, SIGNAL(triggered(bool)),
                      mParent, SLOT(slotForwardAttachedMsg()) );
  }
}

void MessageActions::setupForwardingActionsList( KXMLGUIClient *guiClient )
{
  QList<QAction*> forwardActionList;
  guiClient->unplugActionList( "forward_action_list" );
  if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
    forwardActionList.append( mForwardInlineAction );
    forwardActionList.append( mForwardAttachedAction );
  }
  else {
    forwardActionList.append( mForwardAttachedAction );
    forwardActionList.append( mForwardInlineAction );
  }
  forwardActionList.append( mRedirectAction );
  guiClient->plugActionList( "forward_action_list", forwardActionList );
}


void MessageActions::slotReplyToMsg()
{
  replyCommand<KMReplyToCommand>();
}

void MessageActions::slotReplyAuthorToMsg()
{
  replyCommand<KMReplyAuthorCommand>();
}

void MessageActions::slotReplyListToMsg()
{
  replyCommand<KMReplyListCommand>();
}

void MessageActions::slotReplyAllToMsg()
{
  replyCommand<KMReplyToAllCommand>();
}

void MessageActions::slotNoQuoteReplyToMsg()
{
  if ( !mCurrentItem.isValid() )
    return;
  KMCommand *command = new KMNoQuoteReplyToCommand( mParent, mCurrentItem );
  command->start();
}

void MessageActions::slotSetMsgStatusFlag()
{
  setMessageStatus( Akonadi::MessageStatus::statusImportant(), true );
}

void MessageActions::slotSetMsgStatusToAct()
{
  setMessageStatus( Akonadi::MessageStatus::statusToAct(), true );
}

void MessageActions::slotRunUrl( QAction *urlAction )
{
  const QVariant q = urlAction->data();
  if ( q.type() == QVariant::String ) {
    new KRun( KUrl( q.toString() ) , mParent );
  }
}

void MessageActions::slotPrintMsg()
{
  const bool htmlOverride = mMessageView ? mMessageView->htmlOverride() : false;
  const bool htmlLoadExtOverride = mMessageView ? mMessageView->htmlLoadExtOverride() : false;
  const bool useFixedFont = mMessageView ? mMessageView->isFixedFont() :
                                           MessageViewer::GlobalSettings::self()->useFixedFont();
  const QString overrideEncoding = mMessageView ? mMessageView->overrideEncoding() :
                                 MessageCore::GlobalSettings::self()->overrideCharacterEncoding();

  // FIXME: This is broken when the Viewer shows an encapsulated message
  const Akonadi::Item message = mMessageView ? mMessageView->message() : mCurrentItem;
  KMPrintCommand *command =
    new KMPrintCommand( mParent, message,
                        mMessageView ? mMessageView->headerStyle() : 0,
                        mMessageView ? mMessageView->headerStrategy() : 0,
                        htmlOverride, htmlLoadExtOverride,
                        useFixedFont, overrideEncoding );

  if ( mMessageView ) {
    command->setAttachmentStrategy( mMessageView->attachmentStrategy() );
    command->setOverrideFont( mMessageView->cssHelper()->bodyFont(
        mMessageView->isFixedFont(), true /*printing*/ ) );
  }
  command->start();
}

void MessageActions::setMessageStatus( Akonadi::MessageStatus status, bool toggle )
{
    Akonadi::Item::List items = mVisibleItems;
  if ( items.isEmpty() && mCurrentItem.isValid() )
    items.append( mCurrentItem );
  if ( items.empty() )
    return;
  KMCommand *command = new KMSetStatusCommand( status, items, toggle );
  command->start();
}


/**
 * This adds a list of actions to mMailingListActionMenu mapping the identifier item to
 * the url.
 *
 * e.g.: item = "Contact Owner"
 * "Contact Owner (email)" -> KRun( "mailto:bob@arthouseflowers.example.com" )
 * "Contact Owner (web)" -> KRun( "http://arthouseflowers.example.com/contact-owner.php" )
 */
void MessageActions::addMailingListActions( const QString &item, const KUrl::List &list )
{
  foreach ( const KUrl& url, list ) {
    addMailingListAction( item, url );
  }
}

/**
 * This adds a action to mMailingListActionMenu mapping the identifier item to
 * the url. See addMailingListActions above.
 */
void MessageActions::addMailingListAction( const QString &item, const KUrl &url )
{
  QString protocol = url.protocol().toLower();
  QString prettyUrl = url.prettyUrl();
  if ( protocol == "mailto" ) {
    protocol = i18n( "email" );
    prettyUrl.remove( 0, 7 ); // length( "mailto:" )
  } else if ( protocol.startsWith( QLatin1String( "http" ) ) ) {
    protocol = i18n( "web" );
  }
  // item is a mailing list url description passed from the updateActions method above.
  KAction *act = new KAction( i18nc( "%1 is a 'Contact Owner' or similar action. %2 is a protocol normally web or email though could be irc/ftp or other url variant", "%1 (%2)",  item, protocol ) , this );
  const QVariant v(  url.url() );
  act-> setData( v );
  act-> setHelpText( prettyUrl );
  mMailingListActionMenu->addAction( act );
}

void MessageActions::editCurrentMessage()
{
  if ( !mCurrentItem.isValid() )
    return;
  KMCommand *command = 0;
  Akonadi::Collection col = mCurrentItem.parentCollection();
  // edit, unlike send again, removes the message from the folder
  // we only want that for templates and drafts folders
  if ( col.isValid()
       && ( CommonKernel->folderIsDraftOrOutbox( col ) ||
            CommonKernel->folderIsTemplates( col ) )
    )
    command = new KMEditMsgCommand( mParent, mCurrentItem, true );
  else
    command = new KMEditMsgCommand( mParent, mCurrentItem, false );
  command->start();
}

void MessageActions::annotateMessage()
{
  if ( !mCurrentItem.isValid() )
    return;

  KPIM::AnnotationEditDialog *dialog = new KPIM::AnnotationEditDialog( mCurrentItem.url() );
  dialog->setAttribute( Qt::WA_DeleteOnClose );
  dialog->exec();
  updateAnnotateAction();
}

void MessageActions::updateAnnotateAction()
{
  if( mCurrentItem.isValid() ) {
    Nepomuk::Resource resource( mCurrentItem.url() );
    if ( resource.description().isEmpty() )
      mAnnotateAction->setText( i18n( "Add Note..." ) );
    else
      mAnnotateAction->setText( i18n( "Edit Note...") );
  }
}

#include "messageactions.moc"
