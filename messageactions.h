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

#include "messagecomposer/helper/messagefactory.h"
#include <KUrl>

#include <qobject.h>

class QWidget;
class QAction;
class KJob;
class QAction;
class KActionMenu;
class KActionCollection;
class KXMLGUIClient;
class KMReaderWin;
class QMenu;

namespace Akonadi
{
class Item;
}

namespace TemplateParser
{
class CustomTemplatesMenu;
}

namespace KMail
{

/**
  Manages common actions that can be performed on one or more messages.
*/
class MessageActions : public QObject
{
    Q_OBJECT
public:
    explicit MessageActions(KActionCollection *ac, QWidget *parent);
    ~MessageActions();
    void setMessageView(KMReaderWin *msgView);

    /**
     * This function adds or updates the actions of the forward action menu, taking the
     * preference whether to forward inline or as attachment by default into account.
     * This has to be called when that preference config has been changed.
     */
    void setupForwardActions(KActionCollection *ac);

    /**
     * Sets up action list for forward menu.
     */
    void setupForwardingActionsList(KXMLGUIClient *guiClient);

    void setCurrentMessage(const Akonadi::Item &item , const Akonadi::Item::List &items = Akonadi::Item::List());

    KActionMenu *replyMenu() const
    {
        return mReplyActionMenu;
    }
    QAction *replyListAction() const
    {
        return mReplyListAction;
    }
    QAction *createTodoAction() const
    {
        return mCreateTodoAction;
    }
    QAction *forwardInlineAction() const
    {
        return mForwardInlineAction;
    }
    QAction *forwardAttachedAction() const
    {
        return mForwardAttachedAction;
    }
    QAction *redirectAction() const
    {
        return mRedirectAction;
    }

    KActionMenu *messageStatusMenu() const
    {
        return mStatusMenu;
    }
    KActionMenu *forwardMenu() const
    {
        return mForwardActionMenu;
    }

    QAction *editAction() const
    {
        return mEditAction;
    }
    QAction *annotateAction() const
    {
        return mAnnotateAction;
    }
    QAction *printAction() const
    {
        return mPrintAction;
    }
    QAction *printPreviewAction() const
    {
        return mPrintPreviewAction;
    }
    QAction *listFilterAction() const
    {
        return mListFilterAction;
    }

    KActionMenu *mailingListActionMenu() const
    {
        return mMailingListActionMenu;
    }
    TemplateParser::CustomTemplatesMenu *customTemplatesMenu() const;

    void addWebShortcutsMenu(QMenu *menu, const QString &text);

    QAction *debugBalooAction() const
    {
        return mDebugBalooAction;
    }

signals:
    // This signal is emitted when a reply is triggered and the
    // action has finished.
    // This is useful for the stand-alone reader, it might want to close the window in
    // that case.
    void replyActionFinished();

public slots:
    void editCurrentMessage();
    void annotateMessage();

private:
    void updateActions();
    void replyCommand(MessageComposer::ReplyStrategy strategy);
    void addMailingListAction(const QString &item, const KUrl &url);
    void addMailingListActions(const QString &item, const KUrl::List &list);
    void updateMailingListActions(const Akonadi::Item &messageItem);
    void printMessage(bool preview);
    void clearMailingListActions();

private slots:
    void slotItemModified(const Akonadi::Item   &item, const QSet< QByteArray >   &partIdentifiers);
    void slotItemRemoved(const Akonadi::Item &item);

    void slotReplyToMsg();
    void slotReplyAuthorToMsg();
    void slotReplyListToMsg();
    void slotReplyAllToMsg();
    void slotNoQuoteReplyToMsg();
    void slotRunUrl(QAction *urlAction);
    void slotPrintMsg();
    void slotPrintPreviewMsg();

    void slotUpdateActionsFetchDone(KJob *job);
    void slotMailingListFilter();
    void slotHandleWebShortcutAction();
    void slotConfigureWebShortcuts();
    void slotDebugBaloo();

private:
    QList<QAction *> mMailListActionList;
    QWidget *mParent;
    Akonadi::Item mCurrentItem;
    Akonadi::Item::List mVisibleItems;
    KMReaderWin *mMessageView;

    KActionMenu *mReplyActionMenu;
    QAction *mReplyAction, *mReplyAllAction, *mReplyAuthorAction,
            *mReplyListAction, *mNoQuoteReplyAction,
            *mForwardInlineAction, *mForwardAttachedAction, *mRedirectAction;
    QAction *mCreateTodoAction;
    KActionMenu *mStatusMenu;
    KActionMenu *mForwardActionMenu;
    KActionMenu *mMailingListActionMenu;
    QAction *mEditAction, *mAnnotateAction, *mPrintAction, *mPrintPreviewAction;
    TemplateParser::CustomTemplatesMenu *mCustomTemplatesMenu;
    QAction *mListFilterAction;
    QAction *mDebugBalooAction;
};

}

#endif

