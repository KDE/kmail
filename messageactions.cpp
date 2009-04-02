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

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <klocale.h>

#include <qwidget.h>

using namespace KMail;

MessageActions::MessageActions( KActionCollection *ac, QWidget * parent ) :
    QObject( parent ),
    mParent( parent ),
    mActionCollection( ac ),
    mCurrentMessage( 0 ),
    mMessageView( 0 )
{
    mReplyActionMenu = new KActionMenu( i18n("Message->","&Reply"),
                                      "mail_reply", mActionCollection,
                                      "message_reply_menu" );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
           SLOT(slotReplyToMsg()) );

  mReplyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
                              SLOT(slotReplyToMsg()), mActionCollection, "reply" );
  mReplyActionMenu->insert( mReplyAction );

  mReplyAuthorAction = new KAction( i18n("Reply to A&uthor..."), "mail_reply",
                                    SHIFT+Key_A, this,
                                    SLOT(slotReplyAuthorToMsg()),
                                    mActionCollection, "reply_author" );
  mReplyActionMenu->insert( mReplyAuthorAction );

  mReplyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
                                 Key_A, this, SLOT(slotReplyAllToMsg()),
                                 mActionCollection, "reply_all" );
  mReplyActionMenu->insert( mReplyAllAction );

  mReplyListAction = new KAction( i18n("Reply to Mailing-&List..."),
                                  "mail_replylist", Key_L, this,
                                  SLOT(slotReplyListToMsg()), mActionCollection,
                                  "reply_list" );
  mReplyActionMenu->insert( mReplyListAction );

  mNoQuoteReplyAction = new KAction( i18n("Reply Without &Quote..."), SHIFT+Key_R,
    this, SLOT(slotNoQuoteReplyToMsg()), mActionCollection, "noquotereply" );


  mCreateTodoAction = new KAction( i18n("Create Task/Reminder..."), "mail_todo",
                                   0, this, SLOT(slotCreateTodo()), mActionCollection,
                                   "create_todo" );


  mStatusMenu = new KActionMenu ( i18n( "Mar&k Message" ),
                                  mActionCollection, "set_status" );

  mStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Read"), "kmmsgread",
                                          i18n("Mark selected messages as read")),
                                 0, this, SLOT(slotSetMsgStatusRead()),
                                 mActionCollection, "status_read"));

  mStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &New"), "kmmsgnew",
                                          i18n("Mark selected messages as new")),
                                 0, this, SLOT(slotSetMsgStatusNew()),
                                 mActionCollection, "status_new" ));

  mStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Unread"), "kmmsgunseen",
                                          i18n("Mark selected messages as unread")),
                                 0, this, SLOT(slotSetMsgStatusUnread()),
                                 mActionCollection, "status_unread"));

  mStatusMenu->insert( new KActionSeparator( this ) );

  mToggleFlagAction = new KToggleAction(i18n("Mark Message as &Important"), "mail_flag",
                                 0, this, SLOT(slotSetMsgStatusFlag()),
                                 mActionCollection, "status_flag");
  mToggleFlagAction->setCheckedState( i18n("Remove &Important Message Mark") );
  mStatusMenu->insert( mToggleFlagAction );

  mToggleTodoAction = new KToggleAction(i18n("Mark Message as &Action Item"), "mail_todo",
                                 0, this, SLOT(slotSetMsgStatusTodo()),
                                 mActionCollection, "status_todo");
  mToggleTodoAction->setCheckedState( i18n("Remove &Action Item Message Mark") );
  mStatusMenu->insert( mToggleTodoAction );

  mEditAction = new KAction( i18n("&Edit Message"), "edit", Key_T, this,
                            SLOT(editCurrentMessage()), mActionCollection, "edit" );
  mEditAction->plugAccel( mActionCollection->kaccel() );

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

void MessageActions::setSelectedSernums(const QValueList< Q_UINT32 > & sernums)
{
  mSelectedSernums = sernums;
  updateActions();
}

void MessageActions::setSelectedVisibleSernums(const QValueList< Q_UINT32 > & sernums)
{
  mVisibleSernums = sernums;
  updateActions();
}

void MessageActions::updateActions()
{
  bool singleMsg = (mCurrentMessage != 0);
  if ( mCurrentMessage && mCurrentMessage->parent() ) {
    if ( mCurrentMessage->parent()->isSent() ||
         mCurrentMessage->parent()->isTemplates())
      singleMsg = false;
  }
  const bool multiVisible = mVisibleSernums.count() > 0 || mCurrentMessage;
  const bool flagsAvailable = GlobalSettings::self()->allowLocalFlags() ||
      !((mCurrentMessage && mCurrentMessage->parent()) ? mCurrentMessage->parent()->isReadOnly() : true);

  mCreateTodoAction->setEnabled( singleMsg );
  mReplyActionMenu->setEnabled( singleMsg );
  mReplyAction->setEnabled( singleMsg );
  mNoQuoteReplyAction->setEnabled( singleMsg );
  mReplyAuthorAction->setEnabled( singleMsg );
  mReplyAllAction->setEnabled( singleMsg );
  mReplyListAction->setEnabled( singleMsg );
  mNoQuoteReplyAction->setEnabled( singleMsg );

  mStatusMenu->setEnabled( multiVisible );
  mToggleFlagAction->setEnabled( flagsAvailable );
  mToggleTodoAction->setEnabled( flagsAvailable );

  if ( mCurrentMessage ) {
    mToggleTodoAction->setChecked( mCurrentMessage->isTodo() );
    mToggleFlagAction->setChecked( mCurrentMessage->isImportant() );
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
  setMessageStatus( KMMsgStatusNew );
}

void MessageActions::slotSetMsgStatusUnread()
{
  setMessageStatus( KMMsgStatusUnread );
}

void MessageActions::slotSetMsgStatusRead()
{
  setMessageStatus( KMMsgStatusRead );
}

void MessageActions::slotSetMsgStatusFlag()
{
  setMessageStatus( KMMsgStatusFlag, true );
}

void MessageActions::slotSetMsgStatusTodo()
{
  setMessageStatus( KMMsgStatusTodo, true );
}

void MessageActions::setMessageStatus( KMMsgStatus status, bool toggle )
{
  QValueList<Q_UINT32> serNums = mVisibleSernums;
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
