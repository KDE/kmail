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

#include <KUrl>

#include <qobject.h>
#include <qlist.h>

class QWidget;
class KAction;
class KActionMenu;
class KActionCollection;
class KXMLGUIClient;

namespace Akonadi {
class Item;
}

namespace KMail {

/**
  Manages common actions that can be performed on one or more messages.
*/
class MessageActions : public QObject
{
  Q_OBJECT
  public:
    MessageActions( KActionCollection* ac, QWidget *parent );
    ~MessageActions();
    void setMessageView( KMReaderWin *msgView );

    /**
     * This function adds or updates the actions of the forward action menu, taking the
     * preference whether to forward inline or as attachment by default into account.
     * This has to be called when that preference config has been changed.
     */
    void setupForwardActions();

    /**
     * Sets up action list for forward menu.
     */
    void setupForwardingActionsList( KXMLGUIClient *guiClient );

    void setCurrentMessage( const Akonadi::Item &item );
    void setSelectedItem( const Akonadi::Item::List& items );
    void setSelectedVisibleItems( const Akonadi::Item::List& items );

    KActionMenu* replyMenu() const { return mReplyActionMenu; }
    KAction* replyListAction() const { return mReplyListAction; }
    KAction* createTodoAction() const { return mCreateTodoAction; }
    KAction* forwardInlineAction() const { return mForwardInlineAction; }
    KAction* forwardAttachedAction() const { return mForwardAttachedAction; }
    KAction* redirectAction() const { return mRedirectAction; }

    KActionMenu* messageStatusMenu() const { return mStatusMenu; }
    KActionMenu *forwardMenu() const { return mForwardActionMenu; }

    KAction* editAction() const { return mEditAction; }
    KAction* annotateAction() const { return mAnnotateAction; }

    KActionMenu* mailingListActionMenu() const { return mMailingListActionMenu; }

  public slots:
    void editCurrentMessage();
    void annotateMessage();

  private:
    void updateActions();
    template<typename T> void replyCommand()
    {
      if ( !mCurrentItem.isValid() )
        return;
      const QString text = mMessageView ? mMessageView->copyText() : "";
      KMCommand *command = new T( mParent, mCurrentItem, text );
      command->start();
    }
    void setMessageStatus( KPIM::MessageStatus status, bool toggle = false );
    void addMailingListAction( const QString &item, const KUrl &url );
    void addMailingListActions( const QString &item, const KUrl::List &list );

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
    void slotSetMsgStatusToAct();
    void slotSetMsgStatusFlag();
    void slotRunUrl( QAction *urlAction );

  private:
    QWidget *mParent;
    KActionCollection *mActionCollection;
    Akonadi::Item mCurrentItem;
    Akonadi::Item::List mSelectedItems;
    Akonadi::Item::List mVisibleItems;
    KMReaderWin *mMessageView;

    KActionMenu *mReplyActionMenu;
    KAction *mReplyAction, *mReplyAllAction, *mReplyAuthorAction,
            *mReplyListAction, *mNoQuoteReplyAction,
            *mForwardInlineAction, *mForwardAttachedAction, *mRedirectAction;
    KAction *mCreateTodoAction;
    KActionMenu *mStatusMenu;
    KActionMenu *mForwardActionMenu;
    KActionMenu *mMailingListActionMenu;
    KToggleAction *mToggleFlagAction, *mToggleToActAction;
    KAction *mEditAction, *mAnnotateAction;
    bool mKorganizerIsOnSystem;
};

}

#endif

