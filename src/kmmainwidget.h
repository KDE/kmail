/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2002 Don Sanders <sanders@kde.org>

  Based on the work of Stefan Taferner <taferner@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "config-kmail.h"
#include "kmail_export.h"
#include "kmkernel.h" // for access to config
#include "kmreaderwin.h" //for inline actions

#include <MailCommon/FolderTreeWidget>

#include "messageactions.h"
#include <Akonadi/StandardMailActionManager>
#include <Akonadi/Tag>
#include <KActionCollection>
#include <KXMLGUIClient>
#include <MailCommon/FolderSettings>
#include <MessageList/View>
#include <QPointer>
#include <QTimer>

#if KMAIL_WITH_KUSERFEEDBACK
namespace KUserFeedback
{
class NotificationPopup;
}
#endif

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
class KMTrashMsgCommand;
class KRecentFilesMenu;
class ManageShowCollectionProperties;
class KActionMenuTransport;
class KActionMenuAccount;
class ZoomLabelWidget;
class HistoryClosedReaderMenu;

namespace KIO
{
class Job;
}

namespace KMail
{
class SearchWindowDialog;
class VacationScriptIndicatorWidget;
class TagActionManager;
class FolderShortcutActionManager;
}

namespace KSieveCore
{
class SieveImapPasswordProvider;
}

namespace KSieveUi
{
class ManageSieveScriptsDialog;
class VacationManager;
}
namespace MailCommon
{
class FolderSelectionDialog;
class FavoriteCollectionWidget;
class MailFilter;
}
class QStatusBar;
class KMailPluginCheckBeforeDeletingManagerInterface;
class CollectionSwitcherTreeViewManager;
class KMAIL_EXPORT KMMainWidget : public QWidget
{
    Q_OBJECT

public:
    KMMainWidget(QWidget *parent,
                 KXMLGUIClient *aGUIClient,
                 KActionCollection *actionCollection,
                 const KSharedConfig::Ptr &config = KMKernel::self()->config());
    ~KMMainWidget() override;
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
    [[nodiscard]] KMReaderWin *messageView() const;
    /** Access to the header list pane. */
    CollectionPane *messageListPane() const;

    [[nodiscard]] Akonadi::Collection currentCollection() const;
    [[nodiscard]] QSharedPointer<MailCommon::FolderSettings> currentFolder() const;

    static void cleanup();
    QAction *action(const QString &name);

    QAction *sendQueuedAction() const;

    KActionMenuTransport *sendQueueViaMenu() const;

    /**
      Returns a list of all KMMainWidgets. Warning, the list itself can be 0.
      @return the list of all main widgets, or 0 if it is not yet initialized
    */
    static const KMMainWidget *mainWidgetList();

    QWidget *vacationScriptIndicator() const;
    [[nodiscard]] QWidget *dkimWidgetInfo() const;
    MailCommon::FolderTreeView *folderTreeView() const;

    /** Returns the XML GUI client. */
    KXMLGUIClient *guiClient() const;

    KMail::TagActionManager *tagActionManager() const;

    KMail::FolderShortcutActionManager *folderShortcutActionManager() const;
    void savePaneSelection();

    void updatePaneTagComboBox();

    void addRecentFile(const QUrl &mUrl);
    void updateQuickSearchLineText();

    void populateMessageListStatusFilterCombo();
    void initializePluginActions();

    [[nodiscard]] Akonadi::Item::List currentSelection() const;

    [[nodiscard]] QString fullCollectionPath() const;

    void initializeFilterActions(bool clearFilter);
    /** Clear and create actions for marked filters */
    void clearFilterActions();
    /**
     * Convenience function to get the action collection in a list.
     *
     * @return a list of action collections. The list only has one item, and
     *         that is the action collection of this main widget as returned
     *         by actionCollection().
     */
    [[nodiscard]] QList<KActionCollection *> actionCollections() const;
    void refreshMessageListSelection();
    Akonadi::StandardMailActionManager *standardMailActionManager() const;
    QAction *akonadiStandardAction(Akonadi::StandardActionManager::Type type);
    QAction *akonadiStandardAction(Akonadi::StandardMailActionManager::Type type);
    [[nodiscard]] QWidget *zoomLabelIndicator() const;

    void clearPluginActions();

    void replyMessageTo(const Akonadi::Item &item, bool replyToAll);
public Q_SLOTS:

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

    /** Adds if not existing/removes if existing the tag identified by @p aLabel
        in all selected messages */
    void slotUpdateMessageTagList(const Akonadi::Tag &tag);

    void slotSelectCollectionFolder(const Akonadi::Collection &col);

    void slotUpdateConfig();
Q_SIGNALS:
    void captionChangeRequest(const QString &caption);
    void recreateGui();

protected:
    void showEvent(QShowEvent *event) override;

private:
    KMAIL_NO_EXPORT void assignLoadExternalReference();
    KMAIL_NO_EXPORT KMail::MessageActions *messageActions() const;

    KMAIL_NO_EXPORT KActionMenu *filterMenu() const;

    KMAIL_NO_EXPORT KActionMenu *mailingListActionMenu() const;

    // Moving messages around
    /**
     * This will ask for a destination folder and move the currently selected
     * messages (in MessageListView) into it.
     */
    KMAIL_NO_EXPORT void slotMoveSelectedMessageToFolder();

    // Copying messages around

    /**
     * This will ask for a destination folder and copy the currently selected
     * messages (in MessageListView) into it.
     */
    KMAIL_NO_EXPORT void slotCopySelectedMessagesToFolder();
    /**
     * Implements the "move to trash" action
     */
    KMAIL_NO_EXPORT void slotTrashSelectedMessages();

    KMAIL_NO_EXPORT void slotCheckMail();
    KMAIL_NO_EXPORT void slotCheckMailOnStartup();

    /** Trigger the dialog for editing out-of-office scripts.  */
    KMAIL_NO_EXPORT void slotEditVacation(const QString &serverName);

    KMAIL_NO_EXPORT void slotStartCheckMail();
    KMAIL_NO_EXPORT void slotEndCheckMail();
    KMAIL_NO_EXPORT void restoreCollectionFolderViewConfig();
    /** Update message actions */
    KMAIL_NO_EXPORT void updateMessageActions(bool fast = false);
    KMAIL_NO_EXPORT void updateMessageActionsDelayed();
    /**
      Update message menu
    */
    KMAIL_NO_EXPORT void updateMessageMenu();

    KMAIL_NO_EXPORT void slotRemoveDuplicates();

    /**
      Start a timer to update message actions
    */
    KMAIL_NO_EXPORT void startUpdateMessageActionsTimer();
    KMAIL_NO_EXPORT void slotSelectMoreMessageTagList();
    KMAIL_NO_EXPORT void setupActions();
    KMAIL_NO_EXPORT void createWidgets();
    KMAIL_NO_EXPORT void deleteWidgets();
    KMAIL_NO_EXPORT void layoutSplitters();
    KMAIL_NO_EXPORT void newFromTemplate(const Akonadi::Item &);
    KMAIL_NO_EXPORT void moveSelectedMessagesToFolder(const Akonadi::Collection &dest);
    KMAIL_NO_EXPORT void copySelectedMessagesToFolder(const Akonadi::Collection &dest);
    KMAIL_NO_EXPORT KActionCollection *actionCollection() const;
    /**
      @return the correct config dialog depending on whether the parent of
      the mainWidget is a KPart or a KMMainWindow.
      When dealing with geometries, use this pointer
    */
    [[nodiscard]] KMAIL_NO_EXPORT KSharedConfig::Ptr config();

    KMAIL_NO_EXPORT void checkAkonadiServerManagerState();
    KMAIL_NO_EXPORT void updateHtmlMenuEntry();

    KMAIL_NO_EXPORT void updateMoveAction(const Akonadi::CollectionStatistics &statistic);
    KMAIL_NO_EXPORT void updateMoveAction(bool hasUnreadMails);

    KMAIL_NO_EXPORT void updateAllToTrashAction(qint64 statistics);

    /** Get override character encoding. */
    [[nodiscard]] KMAIL_NO_EXPORT QString overrideEncoding() const;

    KMAIL_NO_EXPORT void moveMessageSelected(MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion = true);

    KMAIL_NO_EXPORT void copyMessageSelected(const Akonadi::Item::List &selectMsg, const Akonadi::Collection &dest);

    /**
     * Move the messages referenced by the specified set to trash.
     * The set parameter must not be null and the ownership is passed
     * to this function.
     */
    KMAIL_NO_EXPORT void trashMessageSelected(MessageList::Core::MessageItemSetReference ref);
    /**
     * Set the status of the messages referenced by the specified set, eventually toggling it.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    KMAIL_NO_EXPORT void setMessageSetStatus(const Akonadi::Item::List &select, Akonadi::MessageStatus status, bool toggle);
    /**
     * Toggles a tag for the messages referenced by the specified set.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    KMAIL_NO_EXPORT void toggleMessageSetTag(const Akonadi::Item::List &select, const Akonadi::Tag &tag);
    /**
     * This applies setMessageSetStatus() on the current thread.
     */
    KMAIL_NO_EXPORT void setCurrentThreadStatus(Akonadi::MessageStatus status, bool toggle);

    KMAIL_NO_EXPORT void applyFilters(const Akonadi::Item::List &selectedMessages);
    KMAIL_NO_EXPORT void applyFilters(const Akonadi::Collection::List &selectedCols);
    KMAIL_NO_EXPORT void applyFilter(const Akonadi::Collection::List &selectedCols, const QString &filter);

    /**
     * Internal helper that creates the folder selection dialog used for the
     * move and copy to folder actions on demand. Only folders where items can
     * be added are listed.
     */
    KMAIL_NO_EXPORT MailCommon::FolderSelectionDialog *moveOrCopyToDialog();

    /**
     * Internal helper that creates the folder selection dialog used for
     * jumping to folders, or adding them as favourites. All folders are listed.
     */
    KMAIL_NO_EXPORT MailCommon::FolderSelectionDialog *selectFromAllFoldersDialog();

    /**
     * Internal helper that applies the current settings so the
     * favorite folder view.
     */
    KMAIL_NO_EXPORT void refreshFavoriteFoldersViewProperties();

    KMAIL_NO_EXPORT void openFilterDialog(const QByteArray &field, const QString &value);

    KMAIL_NO_EXPORT void showMessagePopup(const Akonadi::Item &msg,
                                          const QUrl &aUrl,
                                          const QUrl &imageUrl,
                                          const QPoint &aPoint,
                                          bool contactAlreadyExists,
                                          bool uniqueContactFound,
                                          const WebEngineViewer::WebHitTestResult &result);

    KMAIL_NO_EXPORT void setZoomChanged(qreal zoomFactor);

private Q_SLOTS:
    KMAIL_NO_EXPORT void updateFileMenu();
    KMAIL_NO_EXPORT void slotFilter();
    KMAIL_NO_EXPORT void slotManageSieveScripts();
    KMAIL_NO_EXPORT void slotCompose();
    KMAIL_NO_EXPORT void slotPostToML();
    KMAIL_NO_EXPORT void slotExpireFolder();
    KMAIL_NO_EXPORT void slotExpireAll();
    KMAIL_NO_EXPORT void slotArchiveFolder();
    KMAIL_NO_EXPORT void slotRemoveFolder();
    KMAIL_NO_EXPORT void slotEmptyFolder();
    KMAIL_NO_EXPORT void slotClearCurrentFolder();
    KMAIL_NO_EXPORT void slotAddFavoriteFolder();
    KMAIL_NO_EXPORT void slotShowSelectedFolderInPane();
    KMAIL_NO_EXPORT void slotOverrideHtmlLoadExt();
    KMAIL_NO_EXPORT void slotUseTemplate();
    KMAIL_NO_EXPORT void slotTrashThread();
    KMAIL_NO_EXPORT void slotDeleteThread(bool confirmDelete); // completely delete thread
    KMAIL_NO_EXPORT void slotUndo();
    KMAIL_NO_EXPORT void slotReadOn();
    KMAIL_NO_EXPORT void slotSaveMsg();
    KMAIL_NO_EXPORT void slotOpenMsg();
    KMAIL_NO_EXPORT void slotSaveAttachments();
    KMAIL_NO_EXPORT void slotDeleteAttachments();
    KMAIL_NO_EXPORT void slotJumpToFolder();
    KMAIL_NO_EXPORT void slotCheckVacation();
    KMAIL_NO_EXPORT void slotDebugSieve();
    KMAIL_NO_EXPORT void slotApplyFilters();
    KMAIL_NO_EXPORT void slotApplyFiltersOnFolder(bool recursive);
    KMAIL_NO_EXPORT void slotApplyFilterOnFolder(bool recursive);
    KMAIL_NO_EXPORT void slotExpandThread();
    KMAIL_NO_EXPORT void slotExpandAllThreads();
    KMAIL_NO_EXPORT void slotCollapseThread();
    KMAIL_NO_EXPORT void slotCollapseAllThreads();
    KMAIL_NO_EXPORT void slotSetThreadStatusUnread();
    KMAIL_NO_EXPORT void slotSetThreadStatusRead();
    KMAIL_NO_EXPORT void slotSetThreadStatusImportant();
    KMAIL_NO_EXPORT void slotSetThreadStatusToAct();
    KMAIL_NO_EXPORT void slotSetThreadStatusWatched();
    KMAIL_NO_EXPORT void slotSetThreadStatusIgnored();
    KMAIL_NO_EXPORT void slotSendQueued();
    KMAIL_NO_EXPORT void slotSendQueuedVia(MailTransport::Transport *transport);
    KMAIL_NO_EXPORT void slotOnlineStatus();
    KMAIL_NO_EXPORT void slotUpdateOnlineStatus(KMailSettings::EnumNetworkState::type);
    KMAIL_NO_EXPORT void slotMessagePopup(const Akonadi::Item &aMsg, const WebEngineViewer::WebHitTestResult &result, QPoint aPoint);
    KMAIL_NO_EXPORT void slotContactSearchJobForMessagePopupDone(KJob *job);
    KMAIL_NO_EXPORT void slotSelectAllMessages();
    KMAIL_NO_EXPORT void slotFocusQuickSearch();

    KMAIL_NO_EXPORT void slotIntro();
    KMAIL_NO_EXPORT void slotShowStartupFolder();
    KMAIL_NO_EXPORT void slotCopyDecryptedTo(QAction *action);

    /** Message navigation */
    KMAIL_NO_EXPORT void slotSelectNextMessage();
    KMAIL_NO_EXPORT void slotExtendSelectionToNextMessage();
    KMAIL_NO_EXPORT void slotSelectNextUnreadMessage();
    KMAIL_NO_EXPORT void slotSelectPreviousMessage();
    KMAIL_NO_EXPORT void slotExtendSelectionToPreviousMessage();
    KMAIL_NO_EXPORT void slotSelectPreviousUnreadMessage();
    KMAIL_NO_EXPORT void slotFocusOnNextMessage();
    KMAIL_NO_EXPORT void slotFocusOnPrevMessage();
    KMAIL_NO_EXPORT void slotSelectFirstMessage();
    KMAIL_NO_EXPORT void slotSelectLastMessage();
    KMAIL_NO_EXPORT void slotSelectFocusedMessage();

    KMAIL_NO_EXPORT void slotNextUnreadFolder();
    KMAIL_NO_EXPORT void slotPrevUnreadFolder();

    /** etc. */
    KMAIL_NO_EXPORT void slotDisplayCurrentMessage();

    KMAIL_NO_EXPORT void slotShowNewFromTemplate();
    KMAIL_NO_EXPORT void slotDelayedShowNewFromTemplate(KJob *);
    KMAIL_NO_EXPORT void slotNewFromTemplate(QAction *);

    /** Update the undo action */
    KMAIL_NO_EXPORT void slotUpdateUndo();

    /** Update html and threaded messages preferences in Folder menu. */
    KMAIL_NO_EXPORT void updateFolderMenu();

    /** Settings menu */

    /** XML-GUI stuff */
    KMAIL_NO_EXPORT void slotEditNotifications();

    /** Slot to reply to a message */
    KMAIL_NO_EXPORT void slotCustomReplyToMsg(const QString &tmpl);
    KMAIL_NO_EXPORT void slotCustomReplyAllToMsg(const QString &tmpl);
    KMAIL_NO_EXPORT void slotForwardInlineMsg();
    KMAIL_NO_EXPORT void slotForwardAttachedMessage();
    KMAIL_NO_EXPORT void slotRedirectMessage();
    KMAIL_NO_EXPORT void slotNewMessageToRecipients();
    KMAIL_NO_EXPORT void slotCustomForwardMsg(const QString &tmpl);
    KMAIL_NO_EXPORT void slotSubjectFilter();
    KMAIL_NO_EXPORT void slotFromFilter();
    KMAIL_NO_EXPORT void slotToFilter();

    KMAIL_NO_EXPORT void slotConfigChanged();

    /** Show a splash screen for the longer-lasting operation */
    KMAIL_NO_EXPORT void slotShowBusySplash();

    /**
      Show a message screen explaining that we are currently offline, when
      an online folder is selected.
    */
    KMAIL_NO_EXPORT void showOfflinePage();
    KMAIL_NO_EXPORT void showResourceOfflinePage();
    KMAIL_NO_EXPORT void updateVacationScriptStatus(bool active, const QString &serverName = QString());

    KMAIL_NO_EXPORT void slotItemAdded(const Akonadi::Item &, const Akonadi::Collection &col);
    KMAIL_NO_EXPORT void slotItemRemoved(const Akonadi::Item &);
    KMAIL_NO_EXPORT void slotItemMoved(const Akonadi::Item &item, const Akonadi::Collection &from, const Akonadi::Collection &to);
    KMAIL_NO_EXPORT void slotCollectionStatisticsChanged(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);

    KMAIL_NO_EXPORT void slotAkonadiStandardActionUpdated();
    KMAIL_NO_EXPORT void slotCollectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &);
    KMAIL_NO_EXPORT void slotCreateNewTab(bool);
    KMAIL_NO_EXPORT void slotUpdateActionsAfterMailChecking();
    KMAIL_NO_EXPORT void slotCreateAddressBookContact();
    KMAIL_NO_EXPORT void slotOpenRecentMessage(const QUrl &url);

    KMAIL_NO_EXPORT void slotMoveMessageToTrash();
    /**
     * Called when a "move to trash" operation is completed
     */
    KMAIL_NO_EXPORT void slotTrashMessagesCompleted(KMTrashMsgCommand *command);

    /**
     * Called when a "move" operation is completed
     */
    KMAIL_NO_EXPORT void slotMoveMessagesCompleted(KMMoveCommand *command);

    /**
     * Called when a "copy" operation is completed
     */
    KMAIL_NO_EXPORT void slotCopyMessagesCompleted(KMCommand *command);

    KMAIL_NO_EXPORT void slotRequestFullSearchFromQuickSearch();
    KMAIL_NO_EXPORT void slotFolderChanged(const Akonadi::Collection &);
    KMAIL_NO_EXPORT void slotCollectionFetched(int collectionId);

    KMAIL_NO_EXPORT void itemsReceived(const Akonadi::Item::List &list);
    KMAIL_NO_EXPORT void itemsFetchDone(KJob *job);

    KMAIL_NO_EXPORT void slotServerStateChanged(Akonadi::ServerManager::State state);
    KMAIL_NO_EXPORT void slotArchiveMails();
    KMAIL_NO_EXPORT void slotChangeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage format);

    KMAIL_NO_EXPORT void slotCollectionRemoved(const Akonadi::Collection &col);
    KMAIL_NO_EXPORT void slotCcFilter();
    KMAIL_NO_EXPORT void slotDeleteMessages();

    KMAIL_NO_EXPORT void slotMarkAllMessageAsReadInCurrentFolderAndSubfolder();
    KMAIL_NO_EXPORT void slotRemoveDuplicateRecursive();
    KMAIL_NO_EXPORT void slotRedirectCurrentMessage();
    KMAIL_NO_EXPORT void slotEditCurrentVacation();
    KMAIL_NO_EXPORT void slotReplyMessageTo(const KMime::Message::Ptr &message, bool replyToAll);

private:
    KMAIL_NO_EXPORT void slotSetFocusToViewer();
    KMAIL_NO_EXPORT void deleteSelectedMessages(bool confirmDelete); // completely delete message
    [[nodiscard]] bool showSearchDialog();
    KMAIL_NO_EXPORT void clearCurrentFolder();
    KMAIL_NO_EXPORT void setCurrentCollection(const Akonadi::Collection &col);
    KMAIL_NO_EXPORT void showMessageActivities(const QString &str);
    KMAIL_NO_EXPORT void slotPageIsScrolledToBottom(bool isAtBottom);
    KMAIL_NO_EXPORT void setupUnifiedMailboxChecker();
    QAction *filterToAction(MailCommon::MailFilter *filter);
    [[nodiscard]] Akonadi::Collection::List applyFilterOnCollection(bool recursive);
    KMAIL_NO_EXPORT void setShowStatusBarMessage(const QString &msg);
    KMAIL_NO_EXPORT void slotRestartAccount();
    KMAIL_NO_EXPORT void slotAccountSettings();
    KMAIL_NO_EXPORT void updateDisplayFormatMessage();
    KMAIL_NO_EXPORT void slotHistorySwitchFolder(const Akonadi::Collection &collection);
    KMAIL_NO_EXPORT void redoSwitchFolder();
    KMAIL_NO_EXPORT void undoSwitchFolder();
    KMAIL_NO_EXPORT void updateMoveAllToTrash();
    KMAIL_NO_EXPORT void slotClearFolder();
    KMAIL_NO_EXPORT void slotClearCacheDone();
    KMAIL_NO_EXPORT void slotClearFolderAndSubFolders();
    KMAIL_NO_EXPORT void slotRestoreClosedMessage(Akonadi::Item::Id id);
    KMAIL_NO_EXPORT void slotHistoryClosedReaderChanged();

    // Message actions
    QAction *mDeleteAction = nullptr;
    QAction *mTrashThreadAction = nullptr;
    QAction *mDeleteThreadAction = nullptr;
    QAction *mSaveAsAction = nullptr;
    QAction *mApplyAllFiltersAction = nullptr;
    QAction *mSaveAttachmentsAction = nullptr;
    QAction *mDeleteAttachmentsAction = nullptr;
    QAction *mOpenAction = nullptr;
    QAction *mMoveMsgToFolderAction = nullptr;
    QAction *mCollectionProperties = nullptr;
    QAction *mSendQueued = nullptr;
    QAction *mArchiveAction = nullptr;
    QAction *mSelectAllMessages = nullptr;
    KActionMenuTransport *mSendActionMenu = nullptr;
    QAction *mRestartAccountSettings = nullptr;
    // Filter actions
    KActionMenu *mFilterMenu = nullptr;
    QAction *mExpireConfigAction = nullptr;
    KActionMenu *mApplyFilterFolderActionsMenu = nullptr;
    KActionMenu *mApplyFilterFolderRecursiveActionsMenu = nullptr;
    QAction *mApplyAllFiltersFolderAction = nullptr;
    QAction *mApplyAllFiltersFolderRecursiveAction = nullptr;
    // Custom template actions menu
    KActionMenu *mTemplateMenu = nullptr;

    KActionMenu *mThreadStatusMenu = nullptr;
    KActionMenu *mApplyFilterActionsMenu = nullptr;
    QAction *mCopyActionMenu = nullptr;
    QAction *mMoveActionMenu = nullptr;
    QAction *mCopyDecryptedActionMenu = nullptr;
    QAction *mMarkThreadAsReadAction = nullptr;
    QAction *mMarkThreadAsUnreadAction = nullptr;
    KToggleAction *mToggleThreadImportantAction = nullptr;
    KToggleAction *mToggleThreadToActAction = nullptr;

    KToggleAction *mWatchThreadAction = nullptr;
    KToggleAction *mIgnoreThreadAction = nullptr;

    MailCommon::FavoriteCollectionWidget *mFavoriteCollectionsView = nullptr;
    Akonadi::FavoriteCollectionsModel *mFavoritesModel = nullptr;
    KMReaderWin *mMsgView = nullptr;
    QSplitter *mSplitter1 = nullptr;
    QSplitter *mSplitter2 = nullptr;
    QSplitter *mFolderViewSplitter = nullptr;
    Akonadi::Collection mTemplateFolder;
    bool mLongFolderList = false;
    bool mStartupDone = false;
    bool mWasEverShown = false;
    bool mHtmlGlobalSetting = false;
    bool mHtmlLoadExtGlobalSetting = false;
    bool mFolderHtmlLoadExtPreference = false;
    bool mReaderWindowActive = true;
    bool mReaderWindowBelow = true;
    bool mEnableFavoriteFolderView = false;
    bool mEnableFolderQuickSearch = false;

    QPointer<KMail::SearchWindowDialog> mSearchWinDlg;

    QAction *mExpireFolderAction = nullptr;
    QAction *mFolderMailingListPropertiesAction = nullptr;
    QAction *mShowFolderShortcutDialogAction = nullptr;
    QAction *mArchiveFolderAction = nullptr;
    QAction *mMessageNewList = nullptr;
    KToggleAction *mPreferHtmlLoadExtAction = nullptr;

    QTimer *menutimer = nullptr;
    QTimer *mShowBusySplashTimer = nullptr;

    KSieveUi::VacationManager *mVacationManager = nullptr;
    KActionCollection *mActionCollection = nullptr;
    QAction *const mToolbarActionSeparator;
    QVBoxLayout *mTopLayout = nullptr;
    bool mDestructCalled = false;
    QList<QAction *> mFilterMenuActions;
    QList<QAction *> mFilterFolderMenuActions;
    QList<QAction *> mFilterFolderMenuRecursiveActions;
    QList<QAction *> mFilterTBarActions;
    QList<KMMetaFilterActionCommand *> mFilterCommands;

    KMail::TagActionManager *mTagActionManager = nullptr;
    KMail::FolderShortcutActionManager *mFolderShortcutActionManager = nullptr;
    KSharedConfig::Ptr mConfig;
    KXMLGUIClient *mGUIClient = nullptr;

    KMail::MessageActions *mMsgActions = nullptr;
    Akonadi::StandardMailActionManager *mAkonadiStandardActionManager = nullptr;
    CollectionPane *mMessagePane = nullptr;
    QSharedPointer<MailCommon::FolderSettings> mCurrentFolderSettings;

    MailCommon::FolderTreeWidget *mFolderTreeWidget = nullptr;

    KMail::VacationScriptIndicatorWidget *mVacationScriptIndicator = nullptr;
    bool mVacationIndicatorActive = false;
    bool mGoToFirstUnreadMessageInSelectedFolder = false;
    MessageList::Core::PreSelectionMode mPreSelectionMode;

    QTimer mCheckMailTimer;

    KSieveCore::SieveImapPasswordProvider *const mSievePasswordProvider;
    QPointer<MailCommon::FolderSelectionDialog> mMoveOrCopyToDialog;
    QPointer<MailCommon::FolderSelectionDialog> mSelectFromAllFoldersDialog;
    QAction *mAccountSettings = nullptr;
    KRecentFilesMenu *mOpenRecentMenu = nullptr;
    QPointer<KSieveUi::ManageSieveScriptsDialog> mManageSieveDialog;
    QAction *mQuickSearchAction = nullptr;
    DisplayMessageFormatActionMenu *mDisplayMessageFormatMenu = nullptr;
    MessageViewer::Viewer::DisplayFormatMessage mFolderDisplayFormatPreference = MessageViewer::Viewer::UseGlobalSetting;
#ifndef Q_OS_WIN
    QAction *mSearchMessages = nullptr;
#endif
    KMLaunchExternalComponent *const mLaunchExternalComponent;
    ManageShowCollectionProperties *const mManageShowCollectionProperties;
    QAction *mShowIntroductionAction = nullptr;
    QAction *mMarkAllMessageAsReadAndInAllSubFolder = nullptr;
    KActionMenuAccount *mAccountActionMenu = nullptr;
    QAction *mRemoveDuplicateRecursiveAction = nullptr;
    Akonadi::Collection mCurrentCollection;
    QStatusBar *mCurrentStatusBar = nullptr;
    ZoomLabelWidget *mZoomLabelIndicator = nullptr;
#if KMAIL_WITH_KUSERFEEDBACK
    KUserFeedback::NotificationPopup *mUserFeedBackNotificationPopup = nullptr;
#endif
    KMailPluginCheckBeforeDeletingManagerInterface *mPluginCheckBeforeDeletingManagerInterface = nullptr;
    CollectionSwitcherTreeViewManager *const mCollectionSwitcherTreeViewManager;
    QAction *mClearFolderCacheAction = nullptr;
    HistoryClosedReaderMenu *mRestoreClosedMessageMenu = nullptr;
};
