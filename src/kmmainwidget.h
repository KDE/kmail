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

#ifndef KMAIL_KMMAINWIDGET
#define KMAIL_KMMAINWIDGET

#include "kmail_export.h"
#include "kmreaderwin.h" //for inline actions
#include "kmkernel.h" // for access to config

#include "mailcommon/foldertreewidget.h"

#include <kxmlguiclient.h>
#include "messageactions.h"
#include <kactioncollection.h>
#include <mailcommon/foldersettings.h>
#include "messageviewer/config-messageviewer.h"
#include <QPointer>
#include <QTimer>
#include <Akonadi/KMime/StandardMailActionManager>
#include <AkonadiCore/tag.h>
#include <MessageList/View>

namespace MailTransport {
class Transport;
}
namespace Akonadi {
class Tag;
}

namespace KMime {
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
class KRecentFilesAction;
class ManageShowCollectionProperties;
class KActionMenuTransport;
class KActionMenuAccount;
class ZoomLabelWidget;

namespace KIO {
class Job;
}

namespace KMail {
class SearchWindow;
class VacationScriptIndicatorWidget;
class TagActionManager;
class FolderShortcutActionManager;
}

namespace KSieveUi {
class SieveImapPasswordProvider;
class ManageSieveScriptsDialog;
class VacationManager;
}
#ifdef USE_DKIM_CHECKER
namespace MessageViewer {
class DKIMWidgetInfo;
}
#endif
namespace MailCommon {
class FolderSelectionDialog;
class FavoriteCollectionWidget;
class MailFilter;
}
class QStatusBar;
class KMAIL_EXPORT KMMainWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QList<KMMainWidget *> PtrList;

    KMMainWidget(QWidget *parent, KXMLGUIClient *aGUIClient, KActionCollection *actionCollection, const KSharedConfig::Ptr &config = KMKernel::self()->config());
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
    KMReaderWin *messageView() const;
    /** Access to the header list pane. */
    CollectionPane *messageListPane() const;

    Akonadi::Collection currentCollection() const;
    QSharedPointer<MailCommon::FolderSettings> currentFolder() const;

    static void cleanup();
    QAction *action(const QString &name);

    QAction *sendQueuedAction() const;

    KActionMenuTransport *sendQueueViaMenu() const;

    /**
      Returns a list of all KMMainWidgets. Warning, the list itself can be 0.
      @return the list of all main widgets, or 0 if it is not yet initialized
    */
    static const PtrList *mainWidgetList();

    QWidget *vacationScriptIndicator() const;
#ifdef USE_DKIM_CHECKER
    QWidget *dkimWidgetInfo() const;
#endif
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

    Q_REQUIRED_RESULT Akonadi::Item::List currentSelection() const;

    Q_REQUIRED_RESULT QString fullCollectionPath() const;

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
    Q_REQUIRED_RESULT QList<KActionCollection *> actionCollections() const;
    void refreshMessageListSelection();
    Akonadi::StandardMailActionManager *standardMailActionManager() const;
    QAction *akonadiStandardAction(Akonadi::StandardActionManager::Type type);
    QAction *akonadiStandardAction(Akonadi::StandardMailActionManager::Type type);
    QWidget *zoomLabelIndicator() const;

    void clearPluginActions();
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
    void messagesTransfered(bool);
    void captionChangeRequest(const QString &caption);
    void recreateGui();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void assignLoadExternalReference();
    KMail::MessageActions *messageActions() const;

    KActionMenu *filterMenu() const;

    KActionMenu *mailingListActionMenu() const;

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

    /** Trigger the dialog for editing out-of-office scripts.  */
    void slotEditVacation(const QString &serverName);

    void slotStartCheckMail();
    void slotEndCheckMail();
    void restoreCollectionFolderViewConfig();
    /** Update message actions */
    void updateMessageActions(bool fast = false);
    void updateMessageActionsDelayed();
    /**
      Update message menu
    */
    void updateMessageMenu();

    void slotRemoveDuplicates();
    /**
      Select the given folder
      If the folder is 0 the intro is shown
    */
    void folderSelected(const Akonadi::Collection &col);

    /**
      Start a timer to update message actions
    */
    void startUpdateMessageActionsTimer();
    void slotSelectMoreMessageTagList();
    void setupActions();
    void createWidgets();
    void deleteWidgets();
    void layoutSplitters();
    void newFromTemplate(const Akonadi::Item &);
    void moveSelectedMessagesToFolder(const Akonadi::Collection &dest);
    void copySelectedMessagesToFolder(const Akonadi::Collection &dest);
    KActionCollection *actionCollection() const;
    /**
      @return the correct config dialog depending on whether the parent of
      the mainWidget is a KPart or a KMMainWindow.
      When dealing with geometries, use this pointer
    */
    KSharedConfig::Ptr config();

    void checkAkonadiServerManagerState();
    void updateHtmlMenuEntry();

    void updateMoveAction(const Akonadi::CollectionStatistics &statistic);
    void updateMoveAction(bool hasUnreadMails);

    void updateAllToTrashAction(qint64 statistics);

    /** Get override character encoding. */
    QString overrideEncoding() const;

    void moveMessageSelected(MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion = true);

    void copyMessageSelected(const Akonadi::Item::List &selectMsg, const Akonadi::Collection &dest);

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
    void setMessageSetStatus(const Akonadi::Item::List &select, const Akonadi::MessageStatus &status, bool toggle);
    /**
     * Toggles a tag for the messages referenced by the specified set.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    void toggleMessageSetTag(const Akonadi::Item::List &select, const Akonadi::Tag &tag);
    /**
     * This applies setMessageSetStatus() on the current thread.
     */
    void setCurrentThreadStatus(const Akonadi::MessageStatus &status, bool toggle);

    void applyFilters(const Akonadi::Item::List &selectedMessages);
    void applyFilters(const Akonadi::Collection::List &selectedCols);
    void applyFilter(const Akonadi::Collection::List &selectedCols, const QString &filter);

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

    void showMessagePopup(const Akonadi::Item &msg, const QUrl &aUrl, const QUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound, const WebEngineViewer::WebHitTestResult &result);

    void setZoomChanged(qreal zoomFactor);

private Q_SLOTS:
    void updateFileMenu();
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
    void slotUseTemplate();
    void slotTrashThread();
    void slotDeleteThread(bool confirmDelete);    // completely delete thread
    void slotUndo();
    void slotReadOn();
    void slotSaveMsg();
    void slotOpenMsg();
    void slotSaveAttachments();
    void slotJumpToFolder();
    void slotCheckVacation();
    void slotDebugSieve();
    void slotApplyFilters();
    void slotApplyFiltersOnFolder(bool recursive);
    void slotApplyFilterOnFolder(bool recursive);
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
    void slotUpdateOnlineStatus(KMailSettings::EnumNetworkState::type);
    void slotMessagePopup(const Akonadi::Item &aMsg, const WebEngineViewer::WebHitTestResult &result, const QPoint &aPoint);
    void slotContactSearchJobForMessagePopupDone(KJob *job);
    void slotSelectAllMessages();
    void slotFocusQuickSearch();

    void slotIntro();
    void slotShowStartupFolder();
    void slotCopyDecryptedTo(QAction *action);

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
    void slotForwardAttachedMessage();
    void slotRedirectMessage();
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
    void updateVacationScriptStatus(bool active, const QString &serverName = QString());

    void slotItemAdded(const Akonadi::Item &, const Akonadi::Collection &col);
    void slotItemRemoved(const Akonadi::Item &);
    void slotItemMoved(const Akonadi::Item &item, const Akonadi::Collection &from, const Akonadi::Collection &to);
    void slotCollectionStatisticsChanged(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);

    void slotAkonadiStandardActionUpdated();
    void slotCollectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &);
    void slotCreateNewTab(bool);
    void slotUpdateActionsAfterMailChecking();
    void slotCreateAddressBookContact();
    void slotOpenRecentMessage(const QUrl &url);

    void slotMoveMessageToTrash();
    /**
     * Called when a "move to trash" operation is completed
     */
    void slotTrashMessagesCompleted(KMTrashMsgCommand *command);

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
    void slotServerStateChanged(Akonadi::ServerManager::State state);
    void slotArchiveMails();
    void slotChangeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage format);

    void slotCollectionRemoved(const Akonadi::Collection &col);
    void slotCcFilter();
    void slotBandwidth(bool b);
    void slotDeleteMessages();

    void slotMarkAllMessageAsReadInCurrentFolderAndSubfolder();
    void slotRemoveDuplicateRecursive();
    void slotRedirectCurrentMessage();
    void slotEditCurrentVacation();
    void slotReplyMessageTo(const KMime::Message::Ptr &message, bool replyToAll);
private:
    void slotSetFocusToViewer();
    void deleteSelectedMessages(bool confirmDelete);    // completely delete message
    bool showSearchDialog();
    void clearCurrentFolder();
    void setCurrentCollection(const Akonadi::Collection &col);
    void showMessageActivities(const QString &str);
    void slotPageIsScrolledToBottom(bool isAtBottom);
    void printCurrentMessage(bool preview);
    void replyCurrentMessageCommand(MessageComposer::ReplyStrategy strategy);
    void setupUnifiedMailboxChecker();
    QAction *filterToAction(MailCommon::MailFilter *filter);
    Akonadi::Collection::List applyFilterOnCollection(bool recursive);
    void setShowStatusBarMessage(const QString &msg);
    void slotRestartAccount();
    void slotAccountSettings();

    // Message actions
    QAction *mDeleteAction = nullptr;
    QAction *mTrashThreadAction = nullptr;
    QAction *mDeleteThreadAction = nullptr;
    QAction *mSaveAsAction = nullptr;
    QAction *mApplyAllFiltersAction = nullptr;
    QAction *mSaveAttachmentsAction = nullptr;
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
    bool mReaderWindowActive = false;
    bool mReaderWindowBelow = false;
    bool mEnableFavoriteFolderView = false;
    bool mEnableFolderQuickSearch = false;

    QPointer<KMail::SearchWindow> mSearchWin;

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
    QAction *mToolbarActionSeparator = nullptr;
    QVBoxLayout *mTopLayout = nullptr;
    bool mDestructed = false;
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

    KSieveUi::SieveImapPasswordProvider *mSievePasswordProvider = nullptr;
    QPointer<MailCommon::FolderSelectionDialog> mMoveOrCopyToDialog;
    QPointer<MailCommon::FolderSelectionDialog> mSelectFromAllFoldersDialog;
    QAction *mServerSideSubscription = nullptr;
    QAction *mAccountSettings = nullptr;
    KRecentFilesAction *mOpenRecentAction = nullptr;
    QPointer<KSieveUi::ManageSieveScriptsDialog> mManageSieveDialog;
    QAction *mQuickSearchAction = nullptr;
    DisplayMessageFormatActionMenu *mDisplayMessageFormatMenu = nullptr;
    MessageViewer::Viewer::DisplayFormatMessage mFolderDisplayFormatPreference = MessageViewer::Viewer::UseGlobalSetting;
    QAction *mSearchMessages = nullptr;
    KMLaunchExternalComponent *mLaunchExternalComponent = nullptr;
    ManageShowCollectionProperties *mManageShowCollectionProperties = nullptr;
    QAction *mShowIntroductionAction = nullptr;
    KToggleAction *mUseLessBandwidth = nullptr;
    QAction *mMarkAllMessageAsReadAndInAllSubFolder = nullptr;
    KActionMenuAccount *mAccountActionMenu = nullptr;
    QAction *mRemoveDuplicateRecursiveAction = nullptr;
    Akonadi::Collection mCurrentCollection;
    QStatusBar *mCurrentStatusBar = nullptr;
    ZoomLabelWidget *mZoomLabelIndicator = nullptr;
};

#endif
