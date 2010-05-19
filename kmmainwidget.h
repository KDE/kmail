/* -*- mode: C++; c-file-style: "gnu" -*-
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

#include <kurl.h>
#include <kxmlguiclient.h>
#include "messageactions.h"
#include <kaction.h>
#include <kactioncollection.h>
#include <kvbox.h>
#include <foldercollection.h>

#include <QList>
#include <QVBoxLayout>
#include <QMenu>
#include <QLabel>
#include <QHash>
#include <QPointer>
#include <akonadi/standardactionmanager.h>
#include <messagelist/core/view.h>
#include "foldertreewidget.h"

namespace MessageList {
  class Pane;
}
namespace Akonadi {
  class EntityListView;
}

namespace KMime {
  class Message;
}

class QVBoxLayout;
class QSplitter;

class KActionMenu;
class KToggleAction;
class FolderTreeView;
class KMMetaFilterActionCommand;
class KMSystemTray;
class CustomTemplatesMenu;


template <typename T, typename S> class QMap;

namespace KIO {
  class Job;
}

namespace KMail {
  class Vacation;
  class SieveDebugDialog;
  class SearchWindow;
  class StatusBarLabel;
  class TagActionManager;
  class FolderShortcutActionManager;
}

class FolderTreeWidget;

class KMAIL_EXPORT KMMainWidget : public QWidget
{
  Q_OBJECT

  public:
    typedef QList<KMMainWidget*> PtrList;

    enum PropsPage
    {
      PropsGeneral,
      PropsShortcut,
      PropsMailingList,
      PropsExpire
    };

    KMMainWidget(QWidget *parent, KXMLGUIClient *aGUIClient,
                 KActionCollection *actionCollection,
                 KSharedConfig::Ptr config = KMKernel::config() );
    virtual ~KMMainWidget();
    void destruct();

    /** Read configuration options before widgets are created. */
    virtual void readPreConfig();

    /** Read configuration for current folder. */
    virtual void readFolderConfig();

    /** Write configuration for current folder. */
    virtual void writeFolderConfig();

    /** Read configuration options after widgets are created. */
    virtual void readConfig();

    /** Write configuration options. */
    virtual void writeConfig();

    /** Easy access to main components of the window. */
    KMReaderWin* messageView() const { return mMsgView; }
    /** Access to the header list pane. */
    MessageList::Pane* messageListPane() const { return mMessagePane; }

    QSharedPointer<FolderCollection> currentFolder() const;

    static void cleanup();

    QAction *action( const char *name ) { return mActionCollection->action( name ); }
    KActionMenu *filterMenu() const { return mFilterMenu; }
    KActionMenu *mailingListActionMenu() const { return mMsgActions->mailingListActionMenu(); }
    KAction *trashAction() const { return mTrashAction; }
    KAction *deleteAction() const { return mDeleteAction; }
    KAction *trashThreadAction() const { return mTrashThreadAction; }
    KAction *deleteThreadAction() const { return mDeleteThreadAction; }
    KAction *saveAsAction() const { return mSaveAsAction; }
    KAction *editAction() const { return mMsgActions->editAction(); }
    KAction *useAction() const { return mUseAction; }
    KAction *sendAgainAction() const { return mSendAgainAction; }
    KAction *applyAllFiltersAction() const { return mApplyAllFiltersAction; }
    KAction *findInMessageAction() const { return mFindInMessageAction; }
    KAction *saveAttachmentsAction() const { return mSaveAttachmentsAction; }
    KAction *openAction() const { return mOpenAction; }
    KAction *viewSourceAction() const { return mViewSourceAction; }
    KMail::MessageActions *messageActions() const { return mMsgActions; }

    KActionMenu *threadStatusMenu() const { return mThreadStatusMenu; }
    KAction *moveActionMenu() const{ return mMoveActionMenu; }
    KAction *mopyActionMenu() const { return mCopyActionMenu; }
    KActionMenu *applyFilterActionsMenu() const { return mApplyFilterActionsMenu; }

    KToggleAction *watchThreadAction() const { return mWatchThreadAction; }
    KToggleAction *ignoreThreadAction() const { return mIgnoreThreadAction; }


    void toggleSystemTray();

    void updateListFilterAction();

    /**
      Returns a list of all KMMainWidgets. Warning, the list itself can be 0.
      @return the list of all main widgets, or 0 if it is not yet initialized
    */
    static const PtrList *mainWidgetList();

    KMSystemTray *systray() const;

    /**
      Return the list of all action, in order to check shortcuts conflicts against them.
    */
    QList<QAction*> actionList();

    QLabel* vacationScriptIndicator() const;
    void updateVacationScriptStatus() { updateVacationScriptStatus( mVacationIndicatorActive ); }
    void selectCollectionFolder( const Akonadi::Collection & col );

    FolderTreeView *folderTreeView() const {
      return mFolderTreeWidget->folderTreeView();
    }

    /** Returns the XML GUI client. */
    KXMLGUIClient* guiClient() const { return mGUIClient; }

    KMail::TagActionManager *tagActionManager() const {
      return mTagActionManager;
    }

    KMail::FolderShortcutActionManager *folderShortcutActionManager() const {
      return mFolderShortcutActionManager;
    }

  public slots:
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

    /**
      Select the given folder
      If the folder is 0 the intro is shown
    */
    void folderSelected( const Akonadi::Collection & col );

    /**
      Open a separate viewer window containing the specified message.
    */
    void slotMessageActivated( const Akonadi::Item & );

    /**
      Opens mail in the internal viewer.
    */
    void slotMessageSelected( const Akonadi::Item & );

    void slotItemsFetchedForActivation( const Akonadi::Item::List &list );
    void slotMessageStatusChangeRequest(  const Akonadi::Item &, const KPIM::MessageStatus &, const KPIM::MessageStatus & );


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
    void updateMessageActions();

    /** Clear and create actions for marked filters */
    void clearFilterActions();
    void initializeFilterActions();


    /** Trigger the dialog for editing out-of-office scripts.  */
    void slotEditVacation();

    /** Adds if not existing/removes if existing the tag identified by @p aLabel
        in all selected messages */
    void slotUpdateMessageTagList( const QString &aLabel );

    /**
     * Convenience function to get the action collection in a list.
     *
     * @return a list of action collections. The list only has one item, and
     *         that is the action collection of this main widget as returned
     *         by actionCollection().
     */
    QList<KActionCollection*> actionCollections() const;


    KAction *akonadiStandardAction( Akonadi::StandardActionManager::Type type );
  signals:
    void messagesTransfered( bool );
    void captionChangeRequest( const QString &caption );

  protected:
    void setupActions();
    void createWidgets();
    void deleteWidgets();
    void layoutSplitters();
    void updateFileMenu();
    void newFromTemplate( const Akonadi::Item& );
    void moveSelectedMessagesToFolder( const Akonadi::Collection & dest );
    void copySelectedMessagesToFolder( const Akonadi::Collection& dest );


    virtual void showEvent( QShowEvent *event );

    KActionCollection *actionCollection() const { return mActionCollection; }

    /**
      @return the correct config dialog depending on whether the parent of
      the mainWidget is a KPart or a KMMainWindow.
      When dealing with geometries, use this pointer
    */
    KSharedConfig::Ptr config();

  protected slots:
    void slotCheckOneAccount( QAction* );
    void slotMailChecked( bool newMail, bool sendOnCheck,
                          const QMap<QString, int> & newInFolder );
    void getAccountMenu();
    void getTransportMenu();
    void slotHelp();
    void slotFilter();
    void slotPopFilter();
    void slotManageSieveScripts();
    void slotAddrBook();
    void slotImport();
    void slotCompose();
    void slotPostToML();
    void slotFolderMailingListProperties();
    void slotShowFolderShortcutDialog();
    void slotExpireFolder();
    void slotExpireAll();
    void slotMarkAllAsRead();
    void slotArchiveFolder();
    void slotRemoveFolder();
    void slotDelayedRemoveFolder( KJob* );
    void slotEmptyFolder();
    void slotAddFavoriteFolder();
    void slotShowSelectedForderInPane();
#if 0
  void slotRefreshFolder();
#endif
    void slotOverrideHtml();
    void slotOverrideHtmlLoadExt();
    void slotMessageQueuedOrDrafted();
    void slotUseTemplate();
    void slotDeleteMsg( bool confirmDelete = true );  // completely delete message
    void slotTrashThread();
    void slotDeleteThread( bool confirmDelete = true );  // completely delete thread
    void slotUndo();
    void slotReadOn();
    void slotSaveMsg();
    void slotOpenMsg();
    void slotSaveAttachments();
    void slotJumpToFolder();
    void slotResendMsg();
    void slotCheckVacation();
    void slotDebugSieve();
    void slotStartCertManager();
    void slotStartWatchGnuPG();
    void slotApplyFilters();
    int slotFilterMsg( const Akonadi::Item &msg );
    void slotExpandThread();
    void slotExpandAllThreads();
    void slotCollapseThread();
    void slotCollapseAllThreads();
    void slotShowMsgSrc();
    void slotSetThreadStatusNew();
    void slotSetThreadStatusUnread();
    void slotSetThreadStatusRead();
    void slotSetThreadStatusImportant();
    void slotSetThreadStatusToAct();
    void slotSetThreadStatusWatched();
    void slotSetThreadStatusIgnored();
    void slotSendQueued();
    void slotSendQueuedVia( QAction* item );
    void slotOnlineStatus();
    void slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type );
    void slotMessagePopup(const Akonadi::Item& ,const KUrl&,const QPoint& );
    void slotDelayedMessagePopup( KJob *job );
    void slotMarkAll();
    void slotFocusQuickSearch();
    void slotSearch();
    void slotSearchClosed();
    void slotFind();
    void slotIntro();
    void slotShowStartupFolder();
    /** Show tip-of-the-day, forced */
    void slotShowTip();
    void slotAntiSpamWizard();
    void slotAntiVirusWizard();
    void slotFilterLogViewer();
    void slotAccountWizard();
    /** Message navigation */
    void slotSelectNextMessage();
    void slotExtendSelectionToNextMessage();
    void slotSelectNextUnreadMessage();
    void slotSelectPreviousMessage();
    void slotExtendSelectionToPreviousMessage();
    void slotSelectPreviousUnreadMessage();
    void slotFocusOnNextMessage();
    void slotFocusOnPrevMessage();
    void slotSelectFocusedMessage();

    void slotNextUnreadFolder();
    void slotPrevUnreadFolder();

    /** etc. */
    void slotDisplayCurrentMessage();

    void slotShowNewFromTemplate();
    void slotDelayedShowNewFromTemplate( KJob* );
    void slotNewFromTemplate( QAction* );

    /** Update the undo action */
    void slotUpdateUndo();

    /** Update html and threaded messages preferences in Folder menu. */
    void updateFolderMenu();
    /**
      Enable or disable the "mark all as read" action. Needs to happen more
      often than the other updates and is therefor in its own method.
    */
    void updateMarkAsReadAction();

    /** Settings menu */

    /** XML-GUI stuff */
    void slotEditNotifications();
    void slotEditKeys();

    void removeDuplicates();

    /** Slot to reply to a message */
    void slotCustomReplyToMsg( const QString &tmpl );
    void slotCustomReplyAllToMsg( const QString &tmpl );
    void slotForwardInlineMsg();
    void slotForwardAttachedMsg();
    void slotRedirectMsg();
    void slotCustomForwardMsg( const QString &tmpl );
    void slotNoQuoteReplyToMsg();
    void slotSubjectFilter();
    void slotMailingListFilter();
    void slotFromFilter();
    void slotToFilter();
    void slotCreateTodo();

    void slotConfigChanged();

    /** Show a splash screen for the longer-lasting operation */
    void slotShowBusySplash();

    /**
      Show a message screen explaining that we are currently offline, when
      an online folder is selected.
    */
    void showOfflinePage();

    void updateVacationScriptStatus( bool active );


    void slotShowExpiryProperties();
    void slotItemAdded( const Akonadi::Item &, const Akonadi::Collection& col);
    void slotItemRemoved( const Akonadi::Item & );
  private:
    /** Get override character encoding. */
    QString overrideEncoding() const;

    /** Update the custom template menus. */
    void updateCustomTemplateMenus();


    void moveMessageSelected( MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion = true );

    void copyMessageSelected( const QList<Akonadi::Item> &selectMsg, const Akonadi::Collection &dest );


    /**
     * Move the messages referenced by the specified set to trash.
     * The set parameter must not be null and the ownership is passed
     * to this function.
     */
    void trashMessageSelected( MessageList::Core::MessageItemSetReference ref );
    /**
     * Set the status of the messages referenced by the specified set, eventually toggling it.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    void setMessageSetStatus( const QList<Akonadi::Item> &select,
        const KPIM::MessageStatus &status,
        bool toggle
      );
    /**
     * Toggles a tag for the messages referenced by the specified set.
     * The set parameter must not be null and the ownership is passed to this function.
     */
    void toggleMessageSetTag( const QList<Akonadi::Item> &select, const QString &taglabel );
    /**
     * This applies setMessageSetStatus() on the current thread.
     */
    void setCurrentThreadStatus( const KPIM::MessageStatus &status, bool toggle );
  private slots:
    /**
     * Called when a "move to trash" operation is completed
     */
    void slotTrashMessagesCompleted( KMMoveCommand *command );

    /**
     * Called when a "move" operation is completed
     */
    void slotMoveMessagesCompleted( KMMoveCommand *command );

    /**
     * Called when a "copy" operation is completed
     */
    void slotCopyMessagesCompleted( KMCommand *command );

    void slotRequestFullSearchFromQuickSearch();
    void slotFolderChanged( const Akonadi::Collection& );

    void itemsReceived(const Akonadi::Item::List &list );
    void itemsFetchDone( KJob *job );
    void itemsFetchJobForFilterDone( KJob *job );
    void slotItemsFetchedForFilter( const Akonadi::Item::List & );
private:
    // Message actions
    KAction *mTrashAction, *mDeleteAction, *mTrashThreadAction,
      *mDeleteThreadAction, *mSaveAsAction, *mUseAction,
      *mSendAgainAction, *mApplyAllFiltersAction, *mFindInMessageAction,
      *mSaveAttachmentsAction, *mOpenAction, *mViewSourceAction,
      *mMoveMsgToFolderAction, *mCollectionProperties;
    // Filter actions
    KActionMenu *mFilterMenu;
    KAction *mSubjectFilterAction, *mFromFilterAction, *mToFilterAction,
        *mListFilterAction;

    KAction *mNextMessageAction, *mPreviousMessageAction;
    KAction *mExpireConfigAction;
    KAction *mAddFavoriteFolder;
    // Custom template actions menu
    KActionMenu *mTemplateMenu;
    CustomTemplatesMenu *mCustomTemplateMenus;

    KActionMenu *mThreadStatusMenu, *mApplyFilterActionsMenu;
    KAction *mCopyActionMenu;
    KAction *mMoveActionMenu;
    KAction *mMarkThreadAsNewAction;
    KAction *mMarkThreadAsReadAction;
    KAction *mMarkThreadAsUnreadAction;
    KToggleAction *mToggleThreadImportantAction;
    KToggleAction *mToggleThreadToActAction;
    KToggleAction *mToggleThreadFlagAction;
    KToggleAction* mSizeColumnToggle;

    KToggleAction *mWatchThreadAction, *mIgnoreThreadAction;

    Akonadi::EntityListView *mFavoriteCollectionsView;
    Akonadi::FavoriteCollectionsModel *mFavoritesModel;
    QWidget      *mSearchAndTree;
    KMReaderWin  *mMsgView;
    QSplitter    *mSplitter1, *mSplitter2, *mFolderViewSplitter;
    Akonadi::Collection mTemplateFolder;
    QMenu        *mViewMenu, *mBodyPartsMenu;
    KAction      *mlistFilterAction;
    bool          mIntegrated;
    bool          mBeepOnNew;
    bool          mConfirmEmpty;
    int           mMessageStatusId;
    KUrl          mUrlCurrent;
    QMenu        *mActMenu;
    QMenu        *mSendMenu;
    QMenu        *mFileMenu;
    bool          mLongFolderList;
    bool          mStartupDone;
    bool          mWasEverShown;
    int copyId, moveId, htmlId, threadId;
    bool mHtmlPref, mHtmlLoadExtPref,
      mFolderHtmlPref, mFolderHtmlLoadExtPref,
      mReaderWindowActive, mReaderWindowBelow;
    bool mEnableFavoriteFolderView;
    bool mEnableFolderQuickSearch;

    //  QPopupMenu *mMessageMenu;
    KMail::SearchWindow *mSearchWin;

    KAction *mRemoveFolderAction,
      *mExpireFolderAction,
      *mEmptyFolderAction, *mMarkAllAsReadAction, *mFolderMailingListPropertiesAction,
      *mShowFolderShortcutDialogAction,
      *mRemoveDuplicatesAction, *mArchiveFolderAction,
      *mPostToMailinglistAction;
    KToggleAction *mPreferHtmlAction, *mPreferHtmlLoadExtAction;
    KToggleAction *mFolderAction, *mHeaderAction, *mMimeAction;

    QTimer *menutimer;
    QTimer *mShowBusySplashTimer;

    QPointer<KMail::Vacation> mVacation;
#if !defined(NDEBUG)
    QPointer<KMail::SieveDebugDialog> mSieveDebugDialog;
#endif
    KActionCollection *mActionCollection;
    QAction *mToolbarActionSeparator;
    QVBoxLayout *mTopLayout;
    bool mDestructed, mForceJumpToUnread, mShowingOfflineScreen;
    QList<QAction*> mFilterMenuActions;
    QList<QAction*> mFilterTBarActions;
    QList<KMMetaFilterActionCommand*> mFilterCommands;

    KMail::TagActionManager *mTagActionManager;
    KMail::FolderShortcutActionManager *mFolderShortcutActionManager;
    KMSystemTray *mSystemTray;
    KSharedConfig::Ptr mConfig;
    KXMLGUIClient *mGUIClient;

    KMail::MessageActions *mMsgActions;
    Akonadi::StandardActionManager *mAkonadiStandardActionManager;
    MessageList::Pane *mMessagePane;
    QSharedPointer<FolderCollection> mCurrentFolder;

    FolderTreeWidget *mFolderTreeWidget;
    bool mOpenedImapFolder;

    KMail::StatusBarLabel *mVacationScriptIndicator;
    bool mVacationIndicatorActive;
    bool mGoToFirstUnreadMessageInSelectedFolder;
    MessageList::Core::PreSelectionMode mPreSelectionMode;

    KPIM::ProgressItem *mFilterProgressItem;
};

#endif

