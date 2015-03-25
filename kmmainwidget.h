/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2002 Don Sanders <sanders@kde.org>

  Based on the work of Stefan Taferner <taferner@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __KMMAINWIDGET
#define __KMMAINWIDGET

#include "kmail_export.h"
#include "kmreaderwin.h" //for inline actions
#include "kmkernel.h" // for access to config

#include "foldertreewidget.h"

#include <kxmlguiclient.h>
#include "messageactions.h"
#include <kactioncollection.h>
#include <foldercollection.h>

#include <QPointer>
#include <QTimer>
#include <Akonadi/KMime/StandardMailActionManager>
#include <AkonadiCore/tag.h>
#include <messagelist/core/view.h>

namespace MailTransport
{
class Transport;
}
namespace Akonadi
{
class Tag;
}

namespace KMime
{
class Message;
}
class QUrl;
class QVBoxLayout;
class QSplitter;
class KMLaunchExternalComponent;
class DisplayMessageFormatActionMenu;
class QAction;
class KActionMenu;
class KToggleAction;
class KMMetaFilterActionCommand;
class CollectionPane;
class KMCommand;
class KMMoveCommand;
class KRecentFilesAction;
class ManageShowCollectionProperties;
class KActionMenuTransport;
template <typename T, typename S> class QMap;

namespace KIO
{
class Job;
}

namespace KMail
{
class SearchWindow;
class VacationScriptIndicatorWidget;
class TagActionManager;
class FolderShortcutActionManager;
}

namespace KSieveUi
{
class SieveDebugDialog;
class Vacation;
class ManageSieveScriptsDialog;
class VacationManager;
}

namespace MailCommon
{
class FolderSelectionDialog;
class FavoriteCollectionWidget;
}

class KMAIL_EXPORT KMMainWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QList<KMMainWidget *> PtrList;

    KMMainWidget(QWidget *parent, KXMLGUIClient *aGUIClient,
                 KActionCollection *actionCollection,
                 KSharedConfig::Ptr config = KMKernel::self()->config());
    virtual ~KMMainWidget();
    void destruct();

    /** Read configuration options before widgets are created. */
    void readPreConfig();

    /** Read configuration for current folder. */
    void readFolderConfig();

    /** Write configuration for current folder. */
    void writeFolderConfig();

    /** Read configuration options after widgets are created. */
    void readConfig();

    /** Write configuration options. */
    void writeConfig(bool force = true);

    void writeReaderConfig();

    /** Easy access to main components of the window. */
    KMReaderWin *messageView() const
    {
        return mMsgView;
    }
    /** Access to the header list pane. */
    CollectionPane *messageListPane() const
    {
        return mMessagePane;
    }

    QSharedPointer<MailCommon::FolderCollection> currentFolder() const;

    static void cleanup();
    QAction *action(const QString &name)
    {
        return mActionCollection->action(name);
    }
    KActionMenu *filterMenu() const
    {
        return mFilterMenu;
    }
    KActionMenu *mailingListActionMenu() const
    {
        return mMsgActions->mailingListActionMenu();
    }
    QAction *editAction() const
    {
        return mMsgActions->editAction();
    }
    QAction *sendAgainAction() const
    {
        return mSendAgainAction;
    }
    QAction *sendQueuedAction() const
    {
        return mSendQueued;
    }
    KActionMenuTransport *sendQueueViaMenu() const
    {
        return mSendActionMenu;
    }

    KMail::MessageActions *messageActions() const
    {
        return mMsgActions;
    }

    /**
      Returns a list of all KMMainWidgets. Warning, the list itself can be 0.
      @return the list of all main widgets, or 0 if it is not yet initialized
    */
    static const PtrList *mainWidgetList();

    QWidget *vacationScriptIndicator() const;
    void updateVacationScriptStatus();

    MailCommon::FolderTreeView *folderTreeView() const
    {
        return mFolderTreeWidget->folderTreeView();
    }

    /** Returns the XML GUI client. */
    KXMLGUIClient *guiClient() const
    {
        return mGUIClient;
    }

    KMail::TagActionManager *tagActionManager() const;

    KMail::FolderShortcutActionManager *folderShortcutActionManager() const;
    void savePaneSelection();

    void updatePaneTagComboBox();

    void clearViewer();

    void addRecentFile(const QUrl &mUrl);
    void updateQuickSearchLineText();

    void populateMessageListStatusFilterCombo();
public Q_SLOTS:
    // Moving messages around
    /**
     * This will ask for a destination folder and move the currently selected
     * messages (in MessageListView) into it.
     */
    void slotMoveSelectedMessageToFolder();

    // Copying messages around

    /**
     * This will ask for a destination folder and copy the currently selected
     * messages (in MessageListView) into it.
     */
    void slotCopySelectedMessagesToFolder();

    /**
     * Implements the "move to trash" action
     */
    void slotTrashSelectedMessages();

    void slotCheckMail();

    void slotCheckMailOnStartup();

    /**
      Select the given folder
      If the folder is 0 the intro is shown
    */
    void folderSelected(const Akonadi::Collection &col);

    /**
      Open a separate viewer window containing the specified message.
    */
    void slotMessageActivated(const Akonadi::Item &);

    /**
      Opens mail in the internal viewer.
    */
    void slotMessageSelected(const Akonadi::Item &);

    void slotItemsFetchedForActivation(KMCommand *command);
    void slotMessageStatusChangeRequest(const Akonadi::Item &, const Akonadi::MessageStatus &, const Akonadi::MessageStatus &);

    void slotReplaceMsgByUnencryptedVersion();

    /**
      Update message menu
    */
    void updateMessageMenu();

    /**
      Start a timer to update message actions
    */
    void startUpdateMessageActionsTimer();

    /** Update message actions */
    void updateMessageActions(bool fast = false);
    void updateMessageActionsDelayed();

    /** Clear and create actions for marked filters */
    void clearFilterActions();
    void initializeFilterActions();

    /** Trigger the dialog for editing out-of-office scripts.  */
    void slotEditVacation(const QString &serverName = QString());

    /** Adds if not existing/removes if existing the tag identified by @p aLabel
        in all selected messages */
    void slotUpdateMessageTagList(const Akonadi::Tag &tag);
    void slotSelectMoreMessageTagList();

    /**
     * Convenience function to get the action collection in a list.
     *
     * @return a list of action collections. The list only has one item, and
     *         that is the action collection of this main widget as returned
     *         by actionCollection().
     */
    QList<KActionCollection *> actionCollections() const;

    QAction *akonadiStandardAction(Akonadi::StandardActionManager::Type type);
    QAction *akonadiStandardAction(Akonadi::StandardMailActionManager::Type type);
    Akonadi::StandardMailActionManager *standardMailActionManager() const
    {
        return mAkonadiStandardActionManager;
    }

    void refreshMessageListSelection();

    void slotStartCheckMail();
    void slotEndCheckMail();

    void slotRemoveDuplicates();

    void slotSelectCollectionFolder(const Akonadi::Collection &col);

    void restoreCollectionFolderViewConfig();
Q_SIGNALS:
    void messagesTransfered(bool);
    void captionChangeRequest(const QString &caption);
    void recreateGui();

protected:
    void setupActions();
    void createWidgets();
    void deleteWidgets();
    void layoutSplitters();
    void newFromTemplate(const Akonadi::Item &);
    void moveSelectedMessagesToFolder(const Akonadi::Collection &dest);
    void copySelectedMessagesToFolder(const Akonadi::Collection &dest);

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

    KActionCollection *actionCollection() const
    {
        return mActionCollection;
    }

    /**
      @return the correct config dialog depending on whether the parent of
      the mainWidget is a KPart or a KMMainWindow.
      When dealing with geometries, use this pointer
    */
    KSharedConfig::Ptr config();

protected Q_SLOTS:
    void updateFileMenu();
    void slotHelp();
    void slotFilter();
    void slotManageSieveScripts();
    void slotCompose();
    void slotPostToML();
    void slotExpireFolder();
    void slotExpireAll();
    void slotArchiveFolder();
    void slotRemoveFolder();
    void slotEmptyFolder();
    void slotClearCurrentFolder();
    void slotAddFavoriteFolder();
    void slotShowSelectedFolderInPane();
    void slotOverrideHtmlLoadExt();
    void slotMessageQueuedOrDrafted();
    void slotUseTemplate();
    void slotDeleteMsg(bool confirmDelete = true);    // completely delete message
    void slotTrashThread();
    void slotDeleteThread(bool confirmDelete = true);    // completely delete thread
    void slotUndo();
    void slotReadOn();
    void slotSaveMsg();
    void slotOpenMsg();
    void slotSaveAttachments();
    void slotJumpToFolder();
    void slotResendMsg();
    void slotCheckVacation();
    void slotDebugSieve();
    void slotApplyFilters();
    void slotApplyFiltersOnFolder();
    void slotExpandThread();
    void slotExpandAllThreads();
    void slotCollapseThread();
    void slotCollapseAllThreads();
    void slotSetThreadStatusUnread();
    void slotSetThreadStatusRead();
    void slotSetThreadStatusImportant();
    void slotSetThreadStatusToAct();
    void slotSetThreadStatusWatched();
    void slotSetThreadStatusIgnored();
    void slotSendQueued();
    void slotSendQueuedVia(MailTransport::Transport *transport);
    void slotOnlineStatus();
    void slotUpdateOnlineStatus(GlobalSettings::EnumNetworkState::type);
    void slotMessagePopup(const Akonadi::Item &, const QUrl &, const QUrl &imageUrl, const QPoint &);
    void slotContactSearchJobForMessagePopupDone(KJob *job);
    void slotMarkAll();
    void slotFocusQuickSearch();
    bool slotSearch();
    void slotIntro();
    void slotShowStartupFolder();

    /** Message navigation */
    void slotSelectNextMessage();
    void slotExtendSelectionToNextMessage();
    void slotSelectNextUnreadMessage();
    void slotSelectPreviousMessage();
    void slotExtendSelectionToPreviousMessage();
    void slotSelectPreviousUnreadMessage();
    void slotFocusOnNextMessage();
    void slotFocusOnPrevMessage();
    void slotSelectFirstMessage();
    void slotSelectLastMessage();
    void slotSelectFocusedMessage();

    void slotNextUnreadFolder();
    void slotPrevUnreadFolder();

    /** etc. */
    void slotDisplayCurrentMessage();

    void slotShowNewFromTemplate();
    void slotDelayedShowNewFromTemplate(KJob *);
    void slotNewFromTemplate(QAction *);

    /** Update the undo action */
    void slotUpdateUndo();

    /** Update html and threaded messages preferences in Folder menu. */
    void updateFolderMenu();

    /** Settings menu */

    /** XML-GUI stuff */
    void slotEditNotifications();

    /** Slot to reply to a message */
    void slotCustomReplyToMsg(const QString &tmpl);
    void slotCustomReplyAllToMsg(const QString &tmpl);
    void slotForwardInlineMsg();
    void slotForwardAttachedMsg();
    void slotRedirectMsg();
    void slotCustomForwardMsg(const QString &tmpl);
    void slotSubjectFilter();
    void slotFromFilter();
    void slotToFilter();

    void slotConfigChanged();

    /** Show a splash screen for the longer-lasting operation */
    void slotShowBusySplash();

    /**
      Show a message screen explaining that we are currently offline, when
      an online folder is selected.
    */
    void showOfflinePage();
    void showResourceOfflinePage();
    void updateVacationScriptStatus(bool active , const QString &serverName = QString());

    void slotItemAdded(const Akonadi::Item &, const Akonadi::Collection &col);
    void slotItemRemoved(const Akonadi::Item &);
    void slotItemMoved(const Akonadi::Item &item, const Akonadi::Collection &from, const Akonadi::Collection &to);
    void slotCollectionStatisticsChanged(const Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);

    void slotAkonadiStandardActionUpdated();
    void slotCollectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &);
    void slotCreateNewTab(bool);
    void slotUpdateActionsAfterMailChecking();
    void slotCreateAddressBookContact();
    void slotOpenRecentMsg(const QUrl &url);

private:
    void checkAkonadiServerManagerState();
    void updateHtmlMenuEntry();

    void updateMoveAction(const Akonadi::CollectionStatistics &statistic);
    void updateMoveAction(bool hasUnreadMails, bool hasMails);

    void updateAllToTrashAction(int statistics);

    /** Get Q_DECL_OVERRIDE character encoding. */
    QString overrideEncoding() const;

    void moveMessageSelected(MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion = true);

    void copyMessageSelected(const QList<Akonadi::Item> &selectMsg, const Akonadi::Collection &dest);

    /**
     * Move the messages referenced by the specified set to trash.
     * The set parameter must not be null and the ownership is passed
     * to this function.
     */
    void trashMessageSelected(MessageList::Core::MessageItemSetReference ref);
    /**
     * Set the status of the messages referenced by the specified set, eventually toggling it.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    void setMessageSetStatus(const QList<Akonadi::Item> &select,
                             const Akonadi::MessageStatus &status,
                             bool toggle
                            );
    /**
     * Toggles a tag for the messages referenced by the specified set.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    void toggleMessageSetTag(const QList<Akonadi::Item> &select, const Akonadi::Tag &tag);
    /**
     * This applies setMessageSetStatus() on the current thread.
     */
    void setCurrentThreadStatus(const Akonadi::MessageStatus &status, bool toggle);

    void applyFilters(const QList< Akonadi::Item > &selectedMessages);

    /**
     * Internal helper that creates the folder selection dialog used for the
     * move and copy to folder actions on demand. Only folders where items can
     * be added are listed.
     */
    MailCommon::FolderSelectionDialog *moveOrCopyToDialog();

    /**
     * Internal helper that creates the folder selection dialog used for
     * jumping to folders, or adding them as favourites. All folders are listed.
     */
    MailCommon::FolderSelectionDialog *selectFromAllFoldersDialog();

    /**
     * Internal helper that applies the current settings so the
     * favorite folder view.
     */
    void refreshFavoriteFoldersViewProperties();

    void openFilterDialog(const QByteArray &field, const QString &value);

    void showMessagePopup(const Akonadi::Item &msg , const QUrl &aUrl, const QUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound);

private Q_SLOTS:
    void slotMoveMessageToTrash();
    /**
     * Called when a "move to trash" operation is completed
     */
    void slotTrashMessagesCompleted(KMMoveCommand *command);

    /**
     * Called when a "move" operation is completed
     */
    void slotMoveMessagesCompleted(KMMoveCommand *command);

    /**
     * Called when a "copy" operation is completed
     */
    void slotCopyMessagesCompleted(KMCommand *command);

    void slotRequestFullSearchFromQuickSearch();
    void slotFolderChanged(const Akonadi::Collection &);
    void slotCollectionFetched(int collectionId);

    void itemsReceived(const Akonadi::Item::List &list);
    void itemsFetchDone(KJob *job);

    void slotServerSideSubscription();
    void slotFetchItemsForFolderDone(KJob *job);
    void slotServerStateChanged(Akonadi::ServerManager::State state);
    void slotArchiveMails();
    void slotChangeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage format);

    void slotCollectionRemoved(const Akonadi::Collection &col);
private:
    // Message actions
    QAction *mDeleteAction, *mTrashThreadAction,
            *mDeleteThreadAction, *mSaveAsAction, *mUseAction,
            *mSendAgainAction, *mApplyAllFiltersAction,
            *mSaveAttachmentsAction, *mOpenAction,
            *mMoveMsgToFolderAction, *mCollectionProperties, *mSendQueued;
    QAction *mArchiveAction;
    KActionMenuTransport *mSendActionMenu;
    // Filter actions
    KActionMenu *mFilterMenu;
    QAction *mExpireConfigAction;
    QAction *mApplyFiltersOnFolder;
    // Custom template actions menu
    KActionMenu *mTemplateMenu;

    KActionMenu *mThreadStatusMenu, *mApplyFilterActionsMenu;
    QAction *mCopyActionMenu;
    QAction *mMoveActionMenu;
    QAction *mMarkThreadAsReadAction;
    QAction *mMarkThreadAsUnreadAction;
    KToggleAction *mToggleThreadImportantAction;
    KToggleAction *mToggleThreadToActAction;

    KToggleAction *mWatchThreadAction, *mIgnoreThreadAction;

    MailCommon::FavoriteCollectionWidget *mFavoriteCollectionsView;
    Akonadi::FavoriteCollectionsModel *mFavoritesModel;
    QWidget      *mSearchAndTree;
    KMReaderWin  *mMsgView;
    QSplitter    *mSplitter1, *mSplitter2, *mFolderViewSplitter;
    Akonadi::Collection mTemplateFolder;
    bool          mLongFolderList;
    bool          mStartupDone;
    bool          mWasEverShown;
    bool mHtmlGlobalSetting, mHtmlLoadExtGlobalSetting, mFolderHtmlLoadExtPreference,
         mReaderWindowActive, mReaderWindowBelow;
    bool mEnableFavoriteFolderView;
    bool mEnableFolderQuickSearch;

    QPointer<KMail::SearchWindow> mSearchWin;

    QAction *mExpireFolderAction,
            *mFolderMailingListPropertiesAction,
            *mShowFolderShortcutDialogAction,
            *mArchiveFolderAction, *mMessageNewList;
    KToggleAction *mPreferHtmlLoadExtAction;

    QTimer *menutimer;
    QTimer *mShowBusySplashTimer;

    KSieveUi::VacationManager *mVacationManager;
#if !defined(NDEBUG)
    QPointer<KSieveUi::SieveDebugDialog> mSieveDebugDialog;
#endif
    KActionCollection *mActionCollection;
    QAction *mToolbarActionSeparator;
    QVBoxLayout *mTopLayout;
    bool mDestructed;
    QList<QAction *> mFilterMenuActions;
    QList<QAction *> mFilterTBarActions;
    QList<KMMetaFilterActionCommand *> mFilterCommands;

    KMail::TagActionManager *mTagActionManager;
    KMail::FolderShortcutActionManager *mFolderShortcutActionManager;
    KSharedConfig::Ptr mConfig;
    KXMLGUIClient *mGUIClient;

    KMail::MessageActions *mMsgActions;
    Akonadi::StandardMailActionManager *mAkonadiStandardActionManager;
    CollectionPane *mMessagePane;
    QSharedPointer<MailCommon::FolderCollection> mCurrentFolder;

    MailCommon::FolderTreeWidget *mFolderTreeWidget;

    KMail::VacationScriptIndicatorWidget *mVacationScriptIndicator;
    bool mVacationIndicatorActive;
    bool mGoToFirstUnreadMessageInSelectedFolder;
    MessageList::Core::PreSelectionMode mPreSelectionMode;

    QTimer mCheckMailTimer;

    QPointer<MailCommon::FolderSelectionDialog> mMoveOrCopyToDialog;
    QPointer<MailCommon::FolderSelectionDialog> mSelectFromAllFoldersDialog;
    QAction *mServerSideSubscription;
    KRecentFilesAction *mOpenRecentAction;
    QPointer<KSieveUi::ManageSieveScriptsDialog> mManageSieveDialog;
    QAction *mQuickSearchAction;
    DisplayMessageFormatActionMenu *mDisplayMessageFormatMenu;
    MessageViewer::Viewer::DisplayFormatMessage mFolderDisplayFormatPreference;
    QAction *mSearchMessages;
    KMLaunchExternalComponent *mLaunchExternalComponent;
    ManageShowCollectionProperties *mManageShowCollectionProperties;
    QAction *mShowIntroductionAction;
};

#endif

