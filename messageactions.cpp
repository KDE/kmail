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
#include "kmcommands.h"
#include "customtemplatesmenu.h"

#include "messagecore/annotationdialog.h"
#include "messagecore/globalsettings.h"
#include "messagecore/mailinglist.h"
#include "messagecore/messagehelpers.h"
#include "messageviewer/csshelper.h"
#include "messageviewer/globalsettings.h"

#include <Nepomuk/Resource>

#include <akonadi/itemfetchjob.h>
#include <akonadi/kmime/messageparts.h>
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
#include <KUriFilterData>
#include <KToolInvocation>
#include <KUriFilter>
#include <KStringHandler>
#include <KPrintPreview>

#include <QVariant>
#include <qwidget.h>
#include <akonadi/collection.h>
#include <Akonadi/Monitor>
#include <mailutil.h>
#include <asyncnepomukresourceretriever.h>
#include <nepomuk/resourcemanager.h>

using namespace KMail;

MessageActions::MessageActions( KActionCollection *ac, QWidget* parent ) :
    QObject( parent ),
    mParent( parent ),
    mActionCollection( ac ),
    mMessageView( 0 ),
    mRedirectAction( 0 ),
    mPrintPreviewAction( 0 ),
    mAsynNepomukRetriever( new MessageCore::AsyncNepomukResourceRetriever( QVector<QUrl>() << Nepomuk::Resource::descriptionUri() << Nepomuk::Resource::annotationUri(), this ) ),
    mCustomTemplatesMenu( 0 )
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


  mListFilterAction = new KAction(i18n("Filter on Mailing-&List..."), this);
  mActionCollection->addAction("mlist_filter", mListFilterAction );
  connect(mListFilterAction, SIGNAL(triggered(bool)), SLOT(slotMailingListFilter()));

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
    action = mainwin->akonadiStandardAction( Akonadi::StandardMailActionManager::MarkMailAsImportant );
    mStatusMenu->addAction( action );

    action = mainwin->akonadiStandardAction( Akonadi::StandardMailActionManager::MarkMailAsActionItem );
    mStatusMenu->addAction( action );

  }

  mEditAction = new KAction( KIcon("accessories-text-editor"), i18n("&Edit Message"), this );
  mActionCollection->addAction( "edit", mEditAction );
  connect( mEditAction, SIGNAL(triggered(bool)),
           this, SLOT(editCurrentMessage()) );
  mEditAction->setShortcut( Qt::Key_T );

  mAnnotateAction = new KAction( KIcon( "view-pim-notes" ), i18n( "Add Note..."), this );
  mActionCollection->addAction( "annotate", mAnnotateAction );
  connect( mAnnotateAction, SIGNAL(triggered(bool)),
           this, SLOT(annotateMessage()) );

  mPrintAction = KStandardAction::print( this, SLOT(slotPrintMsg()), mActionCollection );
  if(KPrintPreview::isAvailable())
    mPrintPreviewAction = KStandardAction::printPreview( this, SLOT(slotPrintPreviewMsg()), mActionCollection );

  mForwardActionMenu  = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  mActionCollection->addAction("message_forward", mForwardActionMenu );

  mForwardAttachedAction = new KAction( KIcon("mail-forward"),
                                        i18nc( "@action:inmenu Message->Forward->",
                                               "As &Attachment..." ),
                                        this );
  connect( mForwardAttachedAction, SIGNAL(triggered(bool)),
           parent, SLOT(slotForwardAttachedMsg()) );
  mActionCollection->addAction( "message_forward_as_attachment", mForwardAttachedAction );

  mForwardInlineAction = new KAction( KIcon( "mail-forward" ),
                                       i18nc( "@action:inmenu Message->Forward->",
                                              "&Inline..." ),
                                       this );
  connect( mForwardInlineAction, SIGNAL(triggered(bool)),
           parent, SLOT(slotForwardInlineMsg()) );
  mActionCollection->addAction( "message_forward_inline", mForwardInlineAction );

  setupForwardActions();

  mRedirectAction  = new KAction(i18nc("Message->Forward->", "&Redirect..."), this );
  mActionCollection->addAction( "message_forward_redirect", mRedirectAction );
  connect( mRedirectAction, SIGNAL(triggered(bool)),
           parent, SLOT(slotRedirectMsg()) );
  mRedirectAction->setShortcut( QKeySequence( Qt::Key_E ) );
  mForwardActionMenu->addAction( mRedirectAction );

  //FIXME add KIcon("mail-list") as first arguement. Icon can be derived from
  // mail-reply-list icon by removing top layers off svg
  mMailingListActionMenu = new KActionMenu( i18nc( "Message->", "Mailing-&List" ), this );
  connect( mMailingListActionMenu->menu(), SIGNAL(triggered(QAction*)),
           this, SLOT(slotRunUrl(QAction*)) );
  mActionCollection->addAction( "mailing_list", mMailingListActionMenu );

  mMonitor = new Akonadi::Monitor( this );
  //FIXME: Attachment fetching is not needed here, but on-demand loading is not supported ATM
  mMonitor->itemFetchScope().fetchPayloadPart( Akonadi::MessagePart::Header );
  connect( mMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), SLOT(slotItemModified(Akonadi::Item,QSet<QByteArray>)));
  connect( mMonitor, SIGNAL(itemRemoved(Akonadi::Item)), SLOT(slotItemRemoved(Akonadi::Item)));

  connect( mAsynNepomukRetriever, SIGNAL(resourceReceived(QUrl,Nepomuk::Resource)), SLOT(updateAnnotateAction(QUrl,Nepomuk::Resource)) );

  mCustomTemplatesMenu = new TemplateParser::CustomTemplatesMenu( parent, ac );

  connect( mCustomTemplatesMenu, SIGNAL(replyTemplateSelected(QString)),
           parent, SLOT(slotCustomReplyToMsg(QString)) );
  connect( mCustomTemplatesMenu, SIGNAL(replyAllTemplateSelected(QString)),
             parent, SLOT(slotCustomReplyAllToMsg(QString)) );
  connect( mCustomTemplatesMenu, SIGNAL(forwardTemplateSelected(QString)),
           parent, SLOT(slotCustomForwardMsg(QString)) );
  connect( KMKernel::self(), SIGNAL(customTemplatesChanged()), mCustomTemplatesMenu, SLOT(update()) );

  forwardMenu()->addSeparator();
  forwardMenu()->addAction( mCustomTemplatesMenu->forwardActionMenu() );
  replyMenu()->addSeparator();
  replyMenu()->addAction( mCustomTemplatesMenu->replyActionMenu() );
  replyMenu()->addAction( mCustomTemplatesMenu->replyAllActionMenu() );

  updateActions();
}

MessageActions::~MessageActions()
{
  delete mCustomTemplatesMenu;
}

TemplateParser::CustomTemplatesMenu* MessageActions::customTemplatesMenu() const
{
  return mCustomTemplatesMenu;
}

void MessageActions::setCurrentMessage( const Akonadi::Item &msg )
{
  mMonitor->setItemMonitored( mCurrentItem, false );
  mCurrentItem = msg;

  if ( !msg.isValid() ) {
    mVisibleItems.clear();
  }

  mMonitor->setItemMonitored( mCurrentItem, true );
  updateActions();
}

void MessageActions::slotItemRemoved(const Akonadi::Item& item)
{
  if ( item == mCurrentItem )
  {
    mCurrentItem = Akonadi::Item();
    updateActions();
  }
}

void MessageActions::slotItemModified( const Akonadi::Item &  item, const QSet< QByteArray > &  partIdentifiers )
{
  Q_UNUSED( partIdentifiers );
  if ( item == mCurrentItem )
  {
    mCurrentItem = item;
    const int numberOfVisibleItems = mVisibleItems.count();
    for( int i = 0; i < numberOfVisibleItems; ++i ) {
      Akonadi::Item it = mVisibleItems[i];
      if ( item == it ) {
        mVisibleItems[i] = item;
      }
    }
    updateActions();
  }
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
  const bool hasPayload = mCurrentItem.hasPayload<KMime::Message::Ptr>();
  bool itemValid = mCurrentItem.isValid();
  Akonadi::Collection parent;
  if ( itemValid ) //=> valid
    parent = mCurrentItem.parentCollection();
  if ( parent.isValid() ) {
    if ( CommonKernel->folderIsTemplates(parent) )
      itemValid = false;
  }

  const bool multiVisible = mVisibleItems.count() > 0 || mCurrentItem.isValid();
  const bool uniqItem = ( itemValid||hasPayload ) && ( mVisibleItems.count()<=1 );
  mCreateTodoAction->setEnabled( itemValid && ( mVisibleItems.count()<=1 ) && mKorganizerIsOnSystem);
  mReplyActionMenu->setEnabled( hasPayload );
  mReplyAction->setEnabled( hasPayload );
  mNoQuoteReplyAction->setEnabled( hasPayload );
  mReplyAuthorAction->setEnabled( hasPayload );
  mReplyAllAction->setEnabled( hasPayload );
  mReplyListAction->setEnabled( hasPayload );
  mNoQuoteReplyAction->setEnabled( hasPayload );

  if ( Nepomuk::ResourceManager::instance()->initialized() )
  {
    mAnnotateAction->setEnabled( uniqItem );
    mAsynNepomukRetriever->requestResource( mCurrentItem.url() );
  }
  else
  {
    mAnnotateAction->setEnabled( false );
  }

  mStatusMenu->setEnabled( multiVisible );

  if ( mCurrentItem.hasPayload<KMime::Message::Ptr>() ) {
    if ( mCurrentItem.loadedPayloadParts().contains("RFC822") ) {
      updateMailingListActions( mCurrentItem );
    } else
    {
      Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mCurrentItem );
      job->fetchScope().fetchAllAttributes();
      job->fetchScope().fetchFullPayload( true );
      job->fetchScope().fetchPayloadPart( Akonadi::MessagePart::Header );
      connect( job, SIGNAL(result(KJob*)), SLOT(slotUpdateActionsFetchDone(KJob*)) );
    }
  }
  mEditAction->setEnabled( uniqItem );
}

void MessageActions::slotUpdateActionsFetchDone(KJob* job)
{
  if ( job->error() )
    return;

  Akonadi::ItemFetchJob *fetchJob = static_cast<Akonadi::ItemFetchJob*>( job );
  if ( fetchJob->items().isEmpty() )
    return;
  Akonadi::Item  messageItem = fetchJob->items().first();
  if ( messageItem == mCurrentItem ) {
    mCurrentItem = messageItem;
    updateMailingListActions( messageItem );
  }
}

void MessageActions::updateMailingListActions( const Akonadi::Item& messageItem )
{
  KMime::Message::Ptr message = messageItem.payload<KMime::Message::Ptr>();
  const MessageCore::MailingList mailList = MessageCore::MailingList::detect( message );

  if ( mailList.features() == MessageCore::MailingList::None ) {
    mMailingListActionMenu->setEnabled( false );
    mListFilterAction->setEnabled( false );
    mListFilterAction->setText( i18n("Filter on Mailing-List...") );
  } else {
    // A mailing list menu with only a title is pretty boring
    // so make sure theres at least some content
    QString listId;
    if ( mailList.features() & MessageCore::MailingList::Id ) {
      // From a list-id in the form, "Birds of France <bof.yahoo.com>",
      // take "Birds of France" if it exists otherwise "bof.yahoo.com".
      listId = mailList.id();
      const int start = listId.indexOf( QLatin1Char( '<' ) );
      if ( start > 0 ) {
        listId.truncate( start - 1 );
      } else if ( start == 0 ) {
        const int end = listId.lastIndexOf( QLatin1Char( '>' ) );
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
      addMailingListActions( i18n( "Open Message in List Archive" ), mailList.archivedAtUrls() );
    if ( mailList.features() & MessageCore::MailingList::Post )
      addMailingListActions( i18n( "Post New Message" ), mailList.postUrls() );
    if ( mailList.features() & MessageCore::MailingList::Archive )
      addMailingListActions( i18n( "Go to Archive" ), mailList.archiveUrls() );
    if ( mailList.features() & MessageCore::MailingList::Help )
      addMailingListActions( i18n( "Request Help" ), mailList.helpUrls() );
    if ( mailList.features() & MessageCore::MailingList::Owner )
      addMailingListActions( i18nc( "Contact the owner of the mailing list", "Contact Owner" ), mailList.ownerUrls() );
    if ( mailList.features() & MessageCore::MailingList::Subscribe )
      addMailingListActions( i18n( "Subscribe to List" ), mailList.subscribeUrls() );
    if ( mailList.features() & MessageCore::MailingList::Unsubscribe )
      addMailingListActions( i18n( "Unsubscribe from List" ), mailList.unsubscribeUrls() );
    mMailingListActionMenu->setEnabled( true );

    QByteArray name;
    QString value;
    const QString lname = MailingList::name( message, name, value );
    if ( !lname.isEmpty() )
    {
      mListFilterAction->setEnabled( true );
      mListFilterAction->setText( i18n( "Filter on Mailing-List %1...", lname ) );
    }
  }
}


void MessageActions::replyCommand(MessageComposer::ReplyStrategy strategy)
{
  if ( !mCurrentItem.hasPayload<KMime::Message::Ptr>() ) return;

  const QString text = mMessageView ? mMessageView->copyText() : QString();
  KMCommand *command = new KMReplyCommand( mParent, mCurrentItem, strategy, text );
  connect( command, SIGNAL(completed(KMCommand*)),
           this, SIGNAL(replyActionFinished()) );
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
  disconnect( mForwardActionMenu, SIGNAL(triggered(bool)), 0, 0 );
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
  replyCommand( MessageComposer::ReplySmart );
}

void MessageActions::slotReplyAuthorToMsg()
{
  replyCommand( MessageComposer::ReplyAuthor );
}

void MessageActions::slotReplyListToMsg()
{
  replyCommand( MessageComposer::ReplyList );
}

void MessageActions::slotReplyAllToMsg()
{
  replyCommand( MessageComposer::ReplyAll );
}

void MessageActions::slotNoQuoteReplyToMsg()
{
  if ( !mCurrentItem.hasPayload<KMime::Message::Ptr>() )
    return;
  KMCommand *command = new KMReplyCommand( mParent, mCurrentItem, MessageComposer::ReplySmart, QString(), true );
  command->start();
}

void MessageActions::slotRunUrl( QAction *urlAction )
{
  const QVariant q = urlAction->data();
  if ( q.type() == QVariant::String ) {
    new KRun( KUrl( q.toString() ) , mParent );
  }
}

void MessageActions::slotMailingListFilter()
{
  if ( !mCurrentItem.hasPayload<KMime::Message::Ptr>() )
    return;

  KMCommand *command = new KMMailingListFilterCommand( mParent, mCurrentItem );
  command->start();
}

void MessageActions::printMessage(bool preview)
{
  if ( mMessageView )
  {
    bool result = false;
    if(GlobalSettings::self()->printSelectedText()) {
      result = mMessageView->printSelectedText(preview);
    }
    if(!result) {
      if(preview)
        mMessageView->viewer()->printPreview();
      else
        mMessageView->viewer()->print();
    }
  }
  else
  {
    const bool useFixedFont = MessageViewer::GlobalSettings::self()->useFixedFont();
    const QString overrideEncoding = MessageCore::GlobalSettings::self()->overrideCharacterEncoding();

    const Akonadi::Item message = mCurrentItem;
    KMPrintCommand *command =
      new KMPrintCommand( mParent, message,
                          0,
                          0,
                          false, false,
                          useFixedFont, overrideEncoding );
    command->setPrintPreview(preview);
    command->start();
  }

}

void MessageActions::slotPrintPreviewMsg()
{
  printMessage(true);
}

void MessageActions::slotPrintMsg()
{
  printMessage(false);
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
  if ( protocol == QLatin1String("mailto") ) {
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
  KMCommand *command = 0;
  if ( mCurrentItem.isValid() )
  {
    Akonadi::Collection col = mCurrentItem.parentCollection();
    // edit, unlike send again, removes the message from the folder
    // we only want that for templates and drafts folders
    if ( col.isValid()
         && ( CommonKernel->folderIsDraftOrOutbox( col ) ||
              CommonKernel->folderIsTemplates( col ) )
      )
      command = new KMEditItemCommand( mParent, mCurrentItem, true );
    else
      command = new KMEditItemCommand( mParent, mCurrentItem, false );
    command->start();
  } else if ( mCurrentItem.hasPayload<KMime::Message::Ptr>() ) {
    command = new KMEditMessageCommand( mParent, mCurrentItem.payload<KMime::Message::Ptr>() );
    command->start();
  }
}

void MessageActions::annotateMessage()
{
  if ( !mCurrentItem.isValid() )
    return;

  const QUrl url = mCurrentItem.url();
  MessageCore::AnnotationEditDialog *dialog = new MessageCore::AnnotationEditDialog( url );
  dialog->setAttribute( Qt::WA_DeleteOnClose );
  if ( dialog->exec() )
    mAsynNepomukRetriever->requestResource( url );
}

void MessageActions::updateAnnotateAction( const QUrl &url, const Nepomuk::Resource &resource )
{
  if( mCurrentItem.isValid() && mCurrentItem.url() == url ) {
    if ( resource.description().isEmpty() )
      mAnnotateAction->setText( i18n( "Add Note..." ) );
    else
      mAnnotateAction->setText( i18n( "Edit Note...") );
  }
}

void MessageActions::addWebShortcutsMenu( KMenu *menu, const QString & text )
{
    if ( text.isEmpty() )
        return;

    QString searchText = text;
    searchText = searchText.replace( QLatin1Char('\n'), QLatin1Char(' ') ).replace( QLatin1Char('\r'), QLatin1Char(' ') ).simplified();

    if ( searchText.isEmpty() )
        return;

    KUriFilterData filterData( searchText );

    filterData.setSearchFilteringOptions( KUriFilterData::RetrievePreferredSearchProvidersOnly );

    if ( KUriFilter::self()->filterSearchUri( filterData, KUriFilter::NormalTextFilter ) )
    {
        const QStringList searchProviders = filterData.preferredSearchProviders();

        if ( !searchProviders.isEmpty() )
        {
            KMenu *webShortcutsMenu = new KMenu( menu );
            webShortcutsMenu->setIcon( KIcon( "preferences-web-browser-shortcuts" ) );

            const QString squeezedText = KStringHandler::rsqueeze( searchText, 21 );
            webShortcutsMenu->setTitle( i18n( "Search for '%1' with", squeezedText ) );

            KAction *action = 0;

            foreach( const QString &searchProvider, searchProviders )
            {
                action = new KAction( searchProvider, webShortcutsMenu );
                action->setIcon( KIcon( filterData.iconNameForPreferredSearchProvider( searchProvider ) ) );
                action->setData( filterData.queryForPreferredSearchProvider( searchProvider ) );
                connect( action, SIGNAL(triggered()), this, SLOT(slotHandleWebShortcutAction()) );
                webShortcutsMenu->addAction( action );
            }

            webShortcutsMenu->addSeparator();

            action = new KAction( i18n( "Configure Web Shortcuts..." ), webShortcutsMenu );
            action->setIcon( KIcon( "configure" ) );
            connect( action, SIGNAL(triggered()), this, SLOT(slotConfigureWebShortcuts()) );
            webShortcutsMenu->addAction( action );

            menu->addMenu(webShortcutsMenu);
        }
    }
}

void MessageActions::slotHandleWebShortcutAction()
{
  KAction *action = qobject_cast<KAction*>( sender() );

  if (action)
  {
      KUriFilterData filterData( action->data().toString() );
      if ( KUriFilter::self()->filterSearchUri( filterData, KUriFilter::WebShortcutFilter ) )
      {
          KToolInvocation::invokeBrowser( filterData.uri().url() );
      }
  }
}

void MessageActions::slotConfigureWebShortcuts()
{
 KToolInvocation::kdeinitExec( QLatin1String("kcmshell4"), QStringList() << QLatin1String("ebrowsing") );
}

#include "messageactions.moc"
