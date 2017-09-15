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

#include <MessageComposer/MessageFactoryNG>
#include <QUrl>

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

namespace Akonadi {
class Item;
}

namespace TemplateParser {
class CustomTemplatesMenu;
}

namespace KIO {
class KUriFilterSearchProviderActions;
}
namespace KMail {
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

    void setCurrentMessage(const Akonadi::Item &item, const Akonadi::Item::List &items = Akonadi::Item::List());

    KActionMenu *replyMenu() const;
    QAction *replyListAction() const;
    QAction *forwardInlineAction() const;
    QAction *forwardAttachedAction() const;
    QAction *redirectAction() const;

    KActionMenu *messageStatusMenu() const;
    KActionMenu *forwardMenu() const;

    QAction *editAction() const;
    QAction *annotateAction() const;
    QAction *printAction() const;
    QAction *printPreviewAction() const;
    QAction *listFilterAction() const;

    KActionMenu *mailingListActionMenu() const;
    TemplateParser::CustomTemplatesMenu *customTemplatesMenu() const;

    void addWebShortcutsMenu(QMenu *menu, const QString &text);

    QAction *debugAkonadiSearchAction() const;
    QAction *addFollowupReminderAction() const;

Q_SIGNALS:
    // This signal is emitted when a reply is triggered and the
    // action has finished.
    // This is useful for the stand-alone reader, it might want to close the window in
    // that case.
    void replyActionFinished();

public Q_SLOTS:
    void editCurrentMessage();
    void annotateMessage();

private:
    void updateActions();
    void replyCommand(MessageComposer::ReplyStrategy strategy);
    void addMailingListAction(const QString &item, const QUrl &url);
    void addMailingListActions(const QString &item, const QList<QUrl> &list);
    void updateMailingListActions(const Akonadi::Item &messageItem);
    void printMessage(bool preview);
    void clearMailingListActions();

private Q_SLOTS:
    void slotItemModified(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers);
    void slotItemRemoved(const Akonadi::Item &item);

    void slotReplyToMsg();
    void slotReplyAuthorToMsg();
    void slotReplyListToMsg();
    void slotReplyAllToMsg();
    void slotNoQuoteReplyToMsg();
    void slotRunUrl(QAction *urlAction);
    void slotPrintMessage();
    void slotPrintPreviewMsg();

    void slotUpdateActionsFetchDone(KJob *job);
    void slotMailingListFilter();
    void slotDebugAkonadiSearch();

    void slotAddFollowupReminder();
private:
    QList<QAction *> mMailListActionList;
    QWidget *mParent = nullptr;
    Akonadi::Item mCurrentItem;
    Akonadi::Item::List mVisibleItems;
    KMReaderWin *mMessageView = nullptr;

    KActionMenu *mReplyActionMenu = nullptr;
    QAction *mReplyAction = nullptr;
    QAction *mReplyAllAction = nullptr;
    QAction *mReplyAuthorAction = nullptr;
    QAction *mReplyListAction = nullptr;
    QAction *mNoQuoteReplyAction = nullptr;
    QAction *mForwardInlineAction = nullptr;
    QAction *mForwardAttachedAction = nullptr;
    QAction *mRedirectAction = nullptr;
    KActionMenu *mStatusMenu = nullptr;
    KActionMenu *mForwardActionMenu = nullptr;
    KActionMenu *mMailingListActionMenu = nullptr;
    QAction *mEditAction = nullptr;
    QAction *mAnnotateAction = nullptr;
    QAction *mPrintAction = nullptr;
    QAction *mPrintPreviewAction = nullptr;
    TemplateParser::CustomTemplatesMenu *mCustomTemplatesMenu = nullptr;
    QAction *mListFilterAction = nullptr;
    QAction *mAddFollowupReminderAction = nullptr;
    QAction *mDebugAkonadiSearchAction = nullptr;
    KIO::KUriFilterSearchProviderActions *mWebShortcutMenuManager = nullptr;
};
}

#endif
