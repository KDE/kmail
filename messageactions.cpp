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
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmreaderwin.h"

#include <KAction>
#include <KActionMenu>
#include <KActionCollection>
#include <KToggleAction>
#include <KDebug>
#include <KLocale>
#include <KXMLGUIClient>
#include <KStandardDirs>

#include <qwidget.h>

using namespace KMail;

MessageActions::MessageActions( KActionCollection *ac, QWidget* parent ) :
    QObject( parent ),
    mParent( parent ),
    mActionCollection( ac ),
    mCurrentMessage( 0 ),
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

  KAction *action;

  action = new KAction( KIcon("mail-mark-read"), i18n("Mark Message as &Read"), this );
  action->setHelpText( i18n("Mark selected messages as read") );
  connect( action, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusRead()) );
  mActionCollection->addAction( "status_read", action );
  mStatusMenu->addAction( action );

  action = new KAction( KIcon("mail-mark-unread-new"), i18n("Mark Message as &New"), this );
  action->setHelpText( i18n("Mark selected messages as new") );
  connect( action, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusNew()) );
  mActionCollection->addAction( "status_new", action );
  mStatusMenu->addAction( action );

  action = new KAction( KIcon("mail-mark-unread"), i18n("Mark Message as &Unread"), this );
  action->setHelpText( i18n("Mark selected messages as unread") );
  connect( action, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusUnread()) );
  mActionCollection->addAction( "status_unread", action );
  action->setShortcut( Qt::CTRL+Qt::Key_U );
  mStatusMenu->addAction( action );

  mStatusMenu->addSeparator();

  mToggleFlagAction = new KToggleAction( KIcon("mail-mark-important"),
                                         i18n("Mark Message as &Important"), this );
  mToggleFlagAction->setShortcut( Qt::CTRL + Qt::Key_I );
  connect( mToggleFlagAction, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusFlag()) );
  mToggleFlagAction->setCheckedState( KGuiItem(i18n("Remove &Important Message Mark")) );
  mActionCollection->addAction( "status_flag", mToggleFlagAction );
  mStatusMenu->addAction( mToggleFlagAction );

  mToggleToActAction = new KToggleAction( KIcon("mail-mark-task"),
                                          i18n("Mark Message as &Action Item"), this );
  connect( mToggleToActAction, SIGNAL(triggered(bool)),
           this, SLOT(slotSetMsgStatusToAct()) );
  mToggleToActAction->setCheckedState( KGuiItem(i18n("Remove &Action Item Message Mark")) );
  mActionCollection->addAction( "status_toact", mToggleToActAction );
  mStatusMenu->addAction( mToggleToActAction );

  mEditAction = new KAction( KIcon("accessories-text-editor"), i18n("&Edit Message"), this );
  mActionCollection->addAction( "edit", mEditAction );
  connect( mEditAction, SIGNAL(triggered(bool)),
           this, SLOT(editCurrentMessage()) );
  mEditAction->setShortcut( Qt::Key_T );

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

  updateActions();
}

void MessageActions::setCurrentMessage(KMMessage * msg)
{
  mCurrentMessage = msg;
  if ( !msg ) {
    mSelectedSernums.clear();
    mVisibleSernums.clear();
  }
  updateActions();
}

void MessageActions::setSelectedSernums(const QList< quint32 > & sernums)
{
  mSelectedSernums = sernums;
  updateActions();
}

void MessageActions::setSelectedVisibleSernums(const QList< quint32 > & sernums)
{
  mVisibleSernums = sernums;
  updateActions();
}

void MessageActions::updateActions()
{
  bool singleMsg = (mCurrentMessage != 0);
  if ( mCurrentMessage && mCurrentMessage->parent() ) {
    if ( mCurrentMessage->parent()->isTemplates() )
      singleMsg = false;
  }
  const bool multiVisible = mVisibleSernums.count() > 0 || mCurrentMessage;
  const bool flagsAvailable = GlobalSettings::self()->allowLocalFlags() ||
      !((mCurrentMessage && mCurrentMessage->parent()) ? mCurrentMessage->parent()->isReadOnly() : true);

  mCreateTodoAction->setEnabled( singleMsg && mKorganizerIsOnSystem);
  mReplyActionMenu->setEnabled( singleMsg );
  mReplyAction->setEnabled( singleMsg );
  mNoQuoteReplyAction->setEnabled( singleMsg );
  mReplyAuthorAction->setEnabled( singleMsg );
  mReplyAllAction->setEnabled( singleMsg );
  mReplyListAction->setEnabled( singleMsg );
  mNoQuoteReplyAction->setEnabled( singleMsg );

  mStatusMenu->setEnabled( multiVisible );
  mToggleFlagAction->setEnabled( flagsAvailable );
  mToggleToActAction->setEnabled( flagsAvailable );

  if ( mCurrentMessage ) {
    mToggleToActAction->setChecked( mCurrentMessage->status().isToAct() );
    mToggleFlagAction->setChecked( mCurrentMessage->status().isImportant() );
  }

  mEditAction->setEnabled( singleMsg );
}

void MessageActions::slotCreateTodo()
{
  if ( !mCurrentMessage )
    return;
  KMCommand *command = new CreateTodoCommand( mParent, mCurrentMessage );
  command->start();
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
  if ( !mCurrentMessage )
    return;
  KMCommand *command = new KMNoQuoteReplyToCommand( mParent, mCurrentMessage );
  command->start();
}

void MessageActions::slotSetMsgStatusNew()
{
  setMessageStatus( KPIM::MessageStatus::statusNew() );
}

void MessageActions::slotSetMsgStatusUnread()
{
  setMessageStatus( KPIM::MessageStatus::statusUnread() );
}

void MessageActions::slotSetMsgStatusRead()
{
  setMessageStatus( KPIM::MessageStatus::statusRead() );
}

void MessageActions::slotSetMsgStatusFlag()
{
  setMessageStatus( KPIM::MessageStatus::statusImportant(), true );
}

void MessageActions::slotSetMsgStatusToAct()
{
  setMessageStatus( KPIM::MessageStatus::statusToAct(), true );
}

void MessageActions::setMessageStatus( KPIM::MessageStatus status, bool toggle )
{
  QList<quint32> serNums = mVisibleSernums;
  if ( serNums.isEmpty() && mCurrentMessage )
    serNums.append( mCurrentMessage->getMsgSerNum() );
  if ( serNums.empty() )
    return;
  KMCommand *command = new KMSetStatusCommand( status, serNums, toggle );
  command->start();
}

void MessageActions::editCurrentMessage()
{
  if ( !mCurrentMessage )
    return;
  KMCommand *command = 0;
  KMFolder *folder = mCurrentMessage->parent();
  // edit, unlike send again, removes the message from the folder
  // we only want that for templates and drafts folders
  if ( folder && ( kmkernel->folderIsDraftOrOutbox( folder ) ||
       kmkernel->folderIsTemplates( folder ) ) )
    command = new KMEditMsgCommand( mParent, mCurrentMessage );
  else
    command = new KMResendMessageCommand( mParent, mCurrentMessage );
  command->start();
}

#include "messageactions.moc"
