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
class KAction;
class KActionMenu;
class KActionCollection;
class KXMLGUIClient;
class KMReaderWin;
class KMenu;

namespace Akonadi {
  class Item;
  class Monitor;
}

namespace MessageCore {
  class AsyncNepomukResourceRetriever;
}

namespace Nepomuk2 {
  class Resource;
}

namespace TemplateParser {
  class CustomTemplatesMenu;
}

namespace KMail {

/**
  Manages common actions that can be performed on one or more messages.
*/
class MessageActions : public QObject
{
  Q_OBJECT
  public:
    explicit MessageActions( KActionCollection* ac, QWidget *parent );
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

    void setCurrentMessage(const Akonadi::Item &item , const Akonadi::Item::List &items = Akonadi::Item::List());

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
    KAction* printAction() const { return mPrintAction; }
    KAction* printPreviewAction() const { return mPrintPreviewAction; }
    KAction* listFilterAction() const { return mListFilterAction; }
    KAction *archiveMailAction() const { return mArchiveMailAction; }

    KActionMenu* mailingListActionMenu() const { return mMailingListActionMenu; }
    TemplateParser::CustomTemplatesMenu* customTemplatesMenu() const;

    void addWebShortcutsMenu( KMenu *menu, const QString & text );


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
    void addMailingListAction( const QString &item, const KUrl &url );
    void addMailingListActions( const QString &item, const KUrl::List &list );
    void updateMailingListActions( const Akonadi::Item& messageItem );
    void printMessage(bool preview);
    void clearMailingListActions();


  private slots:
    void updateAnnotateAction(const QUrl& url, const Nepomuk2::Resource& resource);
    void slotItemModified( const Akonadi::Item &  item, const QSet< QByteArray > &  partIdentifiers );
    void slotItemRemoved(const Akonadi::Item& item);

    void slotReplyToMsg();
    void slotReplyAuthorToMsg();
    void slotReplyListToMsg();
    void slotReplyAllToMsg();
    void slotNoQuoteReplyToMsg();
    void slotCreateTodo();
    void slotRunUrl( QAction *urlAction );
    void slotPrintMsg();
    void slotPrintPreviewMsg();

    void slotUpdateActionsFetchDone( KJob* job );
    void slotMailingListFilter();
    void slotHandleWebShortcutAction();
    void slotConfigureWebShortcuts();
    void slotArchiveMail();


  private:
    QList<KAction*> mMailListActionList;
    QWidget *mParent;
    Akonadi::Item mCurrentItem;
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
    KAction *mEditAction, *mAnnotateAction, *mPrintAction, *mPrintPreviewAction;
    bool mKorganizerIsOnSystem;
    Akonadi::Monitor *mMonitor;
    MessageCore::AsyncNepomukResourceRetriever *mAsynNepomukRetriever;
    TemplateParser::CustomTemplatesMenu *mCustomTemplatesMenu;
    KAction *mListFilterAction;
    KAction *mArchiveMailAction;
};

}

#endif

