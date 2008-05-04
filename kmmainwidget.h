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

#include <QList>
#include <QVBoxLayout>
#include <q3listview.h>
#include <QMenu>
#include <QHash>
#include <QPointer>

class QVBoxLayout;
class QSplitter;
class QSignalMapper;
class QToolBar;

class KActionMenu;
class KConfig;
class KToggleAction;
class K3ListViewSearchLine;

class KMFolder;
class KMFolderTree;
class KMFolderTreeItem;
class KMMetaFilterActionCommand;
class FolderShortcutCommand;
class KMMessage;
class KMFolder;
class KMSystemTray;
class KMHeaders;
class KMMessageTagDescription;
typedef QPair<KMMessageTagDescription*,KAction*> MessageTagPtrPair;
class CustomTemplatesMenu;

template <typename T> class QList;
template <typename T, typename S> class QMap;
template <typename T> class QPointer;

namespace KIO {
  class Job;
}

namespace KMail {
  class Vacation;
  class SieveDebugDialog;
  class FolderJob;
  class HeaderListQuickSearch;
  class SearchWindow;
  class ImapAccountBase;
  class FavoriteFolderView;
  class StatusBarLabel;
}

typedef QMap<QAction*,KMFolder*> KMMenuToFolder;

class KMAIL_EXPORT KMMainWidget : public QWidget
{
  Q_OBJECT

  public:
    typedef QList<KMMainWidget*> PtrList;

    KMMainWidget(QWidget *parent, KXMLGUIClient *aGUIClient,
                 KActionCollection *actionCollection,
                 KConfig *config = KMKernel::config() );
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
    KMFolderTree* folderTree() const  { return mFolderTree; }
    KMail::FavoriteFolderView *favoriteFolderView() const { return mFavoriteFolderView; }

    static void cleanup();

    QAction *action( const char *name ) { return mActionCollection->action( name ); }
    KActionMenu *forwardMenu() const { return mForwardActionMenu; }
    KAction *forwardAction() const { return mForwardAction; }
    KAction *forwardAttachedAction() const { return mForwardAttachedAction; }
    KAction *redirectAction() const { return mRedirectAction; }
    KActionMenu *filterMenu() const { return mFilterMenu; }
    KAction *printAction() const { return mPrintAction; }
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
    KActionMenu *moveActionMenu() const{ return mMoveActionMenu; }
    KActionMenu *mopyActionMenu() const { return mCopyActionMenu; }
    KActionMenu *applyFilterActionsMenu() const { return mApplyFilterActionsMenu; }

    KToggleAction *watchThreadAction() const { return mWatchThreadAction; }
    KToggleAction *ignoreThreadAction() const { return mIgnoreThreadAction; }

    KMHeaders *headers() const { return mHeaders; }
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

    void modifyFolder( KMFolderTreeItem *folderItem );

    QLabel* vacationScriptIndicator() const;
    void updateVactionScriptStatus() { updateVactionScriptStatus( mVacationIndicatorActive ); }

  public slots:
    void slotMoveMsgToFolder( KMFolder *dest);
    void slotTrashMsg();   // move to trash

    void slotCheckMail();

    /**
      Select the given folder
      If the folder is 0 the intro is shown
    */
    void folderSelected( KMFolder*, bool forceJumpToUnread = false );

    /**
      Reselect current folder
    */
    void folderSelected();

    /**
      Select the folder and jump to the next unread msg
    */
    void folderSelectedUnread( KMFolder* );

    void slotMsgSelected(KMMessage*);
    void slotMsgChanged();

    /**
      Change the current folder, select a message in the current folder
    */
    void slotSelectFolder(KMFolder*);
    void slotSelectMessage(KMMessage*);

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

    /** Launch subscription-dialog (server side) */
    void slotSubscriptionDialog();

    /** Launch dialog for local (client side) subscription configuration */
    void slotLocalSubscriptionDialog();

    /** The columns of the foldertree changed */
    void slotFolderTreeColumnsChanged();

    /** Clear and create actions for marked filters */
    void clearFilterActions();
    void initializeFilterActions();

    /** Create IMAP-account-related actions if applicable */
    void initializeIMAPActions() { initializeIMAPActions( true ); }

    /** Create actions for the folder shortcuts. */
    void initializeFolderShortcutActions();

    /** Add, remove or adjust the folder's shortcut. */
    void slotShortcutChanged( KMFolder *folder );

    /** Clear and create actions for message tag toggling */
    void clearMessageTagActions();

    void initializeMessageTagActions();

    /** Trigger the dialog for editing out-of-office scripts.  */
    void slotEditVacation();

    /** Adds if not existing/removes if existing the tag identified by @p aLabel
        in all selected messages */
    void slotUpdateMessageTagList( const QString &aLabel );


    /** If @p aCount is 0, disables all tag related actions in menus.
        If @p aCount is 1, Checks/unchecks according to the selected message's tag list.
        If @p aCount is >1, changes labels of the actions to "Toggle <tag>"
       @param aCount Number of selected messages
    */
    void updateMessageTagActions( const int aCount );

    /**
     * Convenience function to get the action collection in a list.
     *
     * @return a list of action collections. The list only has one item, and
     *         that is the action collection of this main widget as returned
     *         by actionCollection().
     */
    QList<KActionCollection*> actionCollections() const;

  signals:
    void messagesTransfered( bool );
    void captionChangeRequest( const QString &caption );

  protected:
    void setupActions();
    void createWidgets();
    void deleteWidgets();
    void layoutSplitters();
    void showMsg( KMReaderWin *win, KMMessage *msg );
    void updateFileMenu();
    void newFromTemplate( KMMessage *msg );

    // helper functions for keeping reference to mFolder
    void openFolder();
    void closeFolder();

    virtual void showEvent( QShowEvent *event );

    KActionCollection *actionCollection() const { return mActionCollection; }

    /**
      @return the correct config dialog depending on whether the parent of
      the mainWidget is a KPart or a KMMainWindow.
      When dealing with geometries, use this pointer
    */
    KConfig *config();

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
    void slotModifyFolder();
    void slotFolderMailingListProperties();
    void slotFolderShortcutCommand();
    void slotExpireFolder();
    void slotExpireAll();
    void slotInvalidateIMAPFolders();
    void slotMarkAllAsRead();
    void slotRemoveFolder();
    void slotEmptyFolder();
    void slotCompactFolder();
    void slotRefreshFolder();
    void slotTroubleshootFolder();
    void slotTroubleshootMaildir();
    void slotCompactAll();
    void slotOverrideHtml();
    void slotOverrideHtmlLoadExt();
    void slotOverrideThread();
    void slotToggleSubjectThreading();
    void slotMessageQueuedOrDrafted();
    void slotUseTemplate();
    //void slotTrashMsg();   // move to trash
    void slotDeleteMsg( bool confirmDelete = true );  // completely delete message
    void slotTrashThread();
    void slotDeleteThread( bool confirmDelete = true );  // completely delete thread
    void slotUndo();
    void slotReadOn();
    void slotSaveMsg();
    void slotOpenMsg();
    void slotSaveAttachments();
    void slotJumpToFolder();
    void slotMoveMsg();
    //void slotMoveMsgToFolder( KMFolder *dest);
    void slotCopyMsgToFolder( KMFolder *dest);
    void slotCopyMsg();
    void slotResendMsg();
    void slotCheckVacation();
    void slotDebugSieve();
    void slotStartCertManager();
    void slotStartWatchGnuPG();
    void slotApplyFilters();
    void slotExpandThread();
    void slotExpandAllThreads();
    void slotCollapseThread();
    void slotCollapseAllThreads();
    void slotShowMsgSrc();
    void slotSetThreadStatusNew();
    void slotSetThreadStatusUnread();
    void slotSetThreadStatusRead();
    void slotSetThreadStatusImportant();
    void slotSetThreadStatusTodo();
    void slotSetThreadStatusWatched();
    void slotSetThreadStatusIgnored();
    void slotToggleUnread();
    void slotToggleTotalColumn();
    void slotToggleSizeColumn();
    void slotSendQueued();
    void slotSendQueuedVia( QAction* item );
    void slotOnlineStatus();
    void slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type );
    void slotMsgPopup(KMMessage &msg, const KUrl &aUrl, const QPoint&);
    void slotMarkAll();
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

    /** Message navigation */
    void slotNextMessage();
    void slotNextUnreadMessage();
    void slotNextUnreadFolder();
    void slotPrevMessage();
    void slotPrevUnreadMessage();
    void slotPrevUnreadFolder();

    /** etc. */
    void slotDisplayCurrentMessage();
    void slotMsgActivated( KMMessage* );

    void slotShowNewFromTemplate();
    void slotNewFromTemplate( QAction* );

    /** Update the undo action */
    void slotUpdateUndo();

    /** Move selected messages to folder with corresponding to given qaction */
    virtual void moveSelectedToFolder( QAction* );
    /** Copy selected messages to folder with corresponding to given qaction */
    virtual void copySelectedToFolder( QAction* );
    /** Update html and threaded messages preferences in Folder menu. */
    void updateFolderMenu();
    /**
      Enable or disable the "mark all as read" action. Needs to happen more
      often the the other updates and is therefor in its own method.
    */
    void updateMarkAsReadAction();

    /** Settings menu */

    /** XML-GUI stuff */
    void slotEditNotifications();
    void slotEditKeys();

    /** changes the caption and displays the foldername */
    void slotChangeCaption(Q3ListViewItem*);
    void removeDuplicates();

    /** Slot to reply to a message */
    void slotCustomReplyToMsg( const QString &tmpl );
    void slotCustomReplyAllToMsg( const QString &tmpl );
    void slotForwardMsg();
    void slotForwardAttachedMsg();
    void slotRedirectMsg();
    void slotCustomForwardMsg( const QString &tmpl );
    void slotNoQuoteReplyToMsg();
    void slotSubjectFilter();
    void slotMailingListFilter();
    void slotFromFilter();
    void slotToFilter();
    void slotPrintMsg();
    void slotCreateTodo();

    void slotConfigChanged();

    /**
      Remove the shortcut actions associated with a folder.
    */
    void slotFolderRemoved( KMFolder *folder );

    /**
     * Reopens the folder if it was closed because of a rename
     */
    void folderClosed( KMFolder *folder );

    /** Show a splash screen for the longer-lasting operation */
    void slotShowBusySplash();

    /**
      Show a message screen explaining that we are currently offline, when
      an online folder is selected.
    */
    void showOfflinePage();

    void updateVactionScriptStatus( bool active );

  private:
    /** Get override character encoding. */
    QString overrideEncoding() const;

    void initializeIMAPActions( bool setState );

    /**
      Helper which finds the associated account if there is a current
      folder and it is an imap or disconnected imap one.
    */
    KMail::ImapAccountBase* findCurrentImapAccountBase();

    /**
      Helper which finds the associated IMAP path if there is a current
      folder and it is an imap or disconnected imap one.
    */
    QString findCurrentImapPath();

    /** Update the custom template menus. */
    void updateCustomTemplateMenus();

  private slots:
    void slotRequestFullSearchFromQuickSearch();

  private:
    // Message actions
    KAction *mTrashAction, *mDeleteAction, *mTrashThreadAction,
      *mDeleteThreadAction, *mSaveAsAction, *mUseAction,
      *mSendAgainAction, *mApplyAllFiltersAction, *mFindInMessageAction,
      *mSaveAttachmentsAction, *mOpenAction, *mViewSourceAction,
      *mFavoritesCheckMailAction;
    // Composition actions
    KAction *mPrintAction,
      *mForwardAction, *mForwardAttachedAction,
      *mRedirectAction;
    KActionMenu *mForwardActionMenu;
    // Filter actions
    KActionMenu *mFilterMenu;
    KAction *mSubjectFilterAction, *mFromFilterAction, *mToFilterAction,
        *mListFilterAction;

    KAction *mNextMessageAction, *mPreviousMessageAction;

    // Custom template actions menu
    KActionMenu *mTemplateMenu;
    CustomTemplatesMenu *mCustomTemplateMenus;

    KActionMenu *mThreadStatusMenu,
      *mMoveActionMenu, *mCopyActionMenu, *mApplyFilterActionsMenu;
    KAction *mMarkThreadAsNewAction;
    KAction *mMarkThreadAsReadAction;
    KAction *mMarkThreadAsUnreadAction;
    KToggleAction *mToggleThreadImportantAction;
    KToggleAction *mToggleThreadTodoAction;
    KToggleAction *mToggleThreadFlagAction;
    KToggleAction* mSizeColumnToggle;

    KToggleAction *mWatchThreadAction, *mIgnoreThreadAction;

    /** we need to access those KToggleActions from the foldertree-popup */
    KToggleAction* mUnreadColumnToggle;
    KToggleAction* mUnreadTextToggle;
    KToggleAction* mTotalColumnToggle;

    KMail::HeaderListQuickSearch *mQuickSearchLine;
    KMail::FavoriteFolderView    *mFavoriteFolderView;
    QPointer<KMFolder> mFolder;
    QWidget      *mSearchAndTree;
    K3ListViewSearchLine *mFolderQuickSearch;
    KMFolderTree *mFolderTree;
    KMReaderWin  *mMsgView;
    QSplitter    *mSplitter1, *mSplitter2, *mFolderViewSplitter;
    KMHeaders    *mHeaders;
    KVBox        *mSearchAndHeaders;
    QWidget      *mSearchToolBar;
    KMFolder     *mTemplateFolder;
    QMenu        *mViewMenu, *mBodyPartsMenu;
    KAction      *mlistFilterAction;
    bool          mIntegrated;
    bool          mBeepOnNew;
    bool          mConfirmEmpty;
    QString       mStartupFolder;
    int           mMessageStatusId;
    KUrl          mUrlCurrent;
    QMenu        *mActMenu;
    QMenu        *mSendMenu;
    QMenu        *mFileMenu;
    bool          mLongFolderList;
    bool          mStartupDone;
    bool          mWasEverShown;
    KMMenuToFolder mMenuToFolder;
    int copyId, moveId, htmlId, threadId;
    bool mHtmlPref, mHtmlLoadExtPref, mThreadPref,
      mFolderHtmlPref, mFolderHtmlLoadExtPref, mFolderThreadPref,
      mFolderThreadSubjPref, mReaderWindowActive, mReaderWindowBelow;
    bool mEnableFavoriteFolderView;
    bool mEnableFolderQuickSearch;
    bool mEnableQuickSearch;

    //  QPopupMenu *mMessageMenu;
    KMail::SearchWindow *mSearchWin;

    KAction *mNewFolderAction, *mModifyFolderAction, *mRemoveFolderAction,
      *mExpireFolderAction, *mCompactFolderAction, *mRefreshFolderAction,
      *mEmptyFolderAction, *mMarkAllAsReadAction, *mFolderMailingListPropertiesAction,
      *mFolderShortCutCommandAction, *mTroubleshootFolderAction, *mRemoveDuplicatesAction,
      *mTroubleshootMaildirAction;
    KToggleAction *mPreferHtmlAction, *mPreferHtmlLoadExtAction,
      *mThreadMessagesAction, *mThreadBySubjectAction;
    KToggleAction *mFolderAction, *mHeaderAction, *mMimeAction;

    QTimer *menutimer;
    QTimer *mShowBusySplashTimer;

    QPointer<KMail::Vacation> mVacation;
#if !defined(NDEBUG)
    QPointer<KMail::SieveDebugDialog> mSieveDebugDialog;
#endif
    KActionCollection *mActionCollection;
    QAction *mToolbarActionSeparator;
    QAction *mMessageTagToolbarActionSeparator;
    QVBoxLayout *mTopLayout;
    bool mDestructed, mForceJumpToUnread, mShowingOfflineScreen;
    QList<QAction*> mFilterMenuActions;
    QList<QAction*> mFilterTBarActions;
    QList<KMMetaFilterActionCommand*> mFilterCommands;
    QHash<QString,FolderShortcutCommand*> mFolderShortcutCommands;
    QPointer<KMail::FolderJob> mJob;

    QList<MessageTagPtrPair> mMessageTagMenuActions;
    QList<QAction*> mMessageTagTBarActions;
    QSignalMapper *mMessageTagToggleMapper;

    KMSystemTray *mSystemTray;
    KConfig *mConfig;
    KXMLGUIClient *mGUIClient;

    KMail::MessageActions *mMsgActions;

    bool mOpenedImapFolder;

    KMail::StatusBarLabel *mVacationScriptIndicator;
    bool mVacationIndicatorActive;
};

#endif

