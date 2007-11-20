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

#ifndef KMAIL_MESSAGEACTIONS_H
#define KMAIL_MESSAGEACTIONS_H

#include "kmcommands.h"
#include "kmreaderwin.h"

#include <qobject.h>
#include <qvaluelist.h>

class QWidget;
class KAction;
class KActionMenu;
class KActionCollection;
class KMMessage;

namespace KMail {

/**
  Manages common actions that can be performed on one or more messages.
*/
class MessageActions : public QObject
{
  Q_OBJECT
  public:
    MessageActions( KActionCollection* ac, QWidget *parent );
    void setMessageView( KMReaderWin *msgView );

    void setCurrentMessage( KMMessage *msg );
    void setSelectedSernums( const QValueList<Q_UINT32> &sernums );
    void setSelectedVisibleSernums( const QValueList<Q_UINT32> &sernums );

    KActionMenu* replyMenu() const { return mReplyActionMenu; }
    KAction* replyListAction() const { return mReplyListAction; }
    KAction* createTodoAction() const { return mCreateTodoAction; }

    KActionMenu* messageStatusMenu() const { return mStatusMenu; }

    KAction* editAction() const { return mEditAction; }

  public slots:
    void editCurrentMessage();

  private:
    void updateActions();
    template<typename T> void replyCommand()
    {
      if ( !mCurrentMessage )
        return;
      const QString text = mMessageView ? mMessageView->copyText() : "";
      KMCommand *command = new T( mParent, mCurrentMessage, text );
      command->start();
    }
    void setMessageStatus( KMMsgStatus status, bool toggle = false );

  private slots:
    void slotReplyToMsg();
    void slotReplyAuthorToMsg();
    void slotReplyListToMsg();
    void slotReplyAllToMsg();
    void slotNoQuoteReplyToMsg();
    void slotCreateTodo();
    void slotSetMsgStatusNew();
    void slotSetMsgStatusUnread();
    void slotSetMsgStatusRead();
    void slotSetMsgStatusTodo();
    void slotSetMsgStatusFlag();

  private:
    QWidget *mParent;
    KActionCollection *mActionCollection;
    KMMessage* mCurrentMessage;
    QValueList<Q_UINT32> mSelectedSernums;
    QValueList<Q_UINT32> mVisibleSernums;
    KMReaderWin *mMessageView;

    KActionMenu *mReplyActionMenu;
    KAction *mReplyAction, *mReplyAllAction, *mReplyAuthorAction,
            *mReplyListAction, *mNoQuoteReplyAction;
    KAction *mCreateTodoAction;
    KActionMenu *mStatusMenu;
    KToggleAction *mToggleFlagAction, *mToggleTodoAction;
    KAction *mEditAction;
};

}

#endif

