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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __KMMAINWIDGET
#define __KMMAINWIDGET

#include <kurl.h>
#include <kxmlguiclient.h>
#include <qguardedptr.h>
#include <qlistview.h>
#include <qvbox.h>
#include <qvaluevector.h>

#include "kmreaderwin.h" //for inline actions
#include "kmkernel.h" // for access to config
#include "messageactions.h"
#include <kaction.h>

class QVBoxLayout;
class QSplitter;
class QSignalMapper;

class KActionMenu;
class KActionCollection;
class KConfig;
class KRadioAction;
class KToggleAction;
class KMenuBar;
class KStatusBarLabel;

class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMFolderTreeItem;
class KMCommand;
class KMMetaFilterActionCommand;
class FolderShortcutCommand;
class KMMessage;
class KMFolder;
class KMAccount;
class KMSystemTray;
class KMHeaders;

template <typename T> class QValueList;
template <typename T, typename S> class QMap;
template <typename T> class QGuardedPtr;

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
}

typedef QMap<int,KMFolder*> KMMenuToFolder;


class KDE_EXPORT KMMainWidget : public QWidget
{
  Q_OBJECT

public:
  KMMainWidget(QWidget *parent, const char *name,
               KXMLGUIClient *aGUIClient,
               KActionCollection *actionCollection,
         KConfig*config = KMKernel::config() );
  virtual ~KMMainWidget();
  void destruct();

  /** Read configuration options before widgets are created. */
  virtual void readPreConfig(void);

  /** Read configuration for current folder. */
  virtual void readFolderConfig(void);

  /** Write configuration for current folder. */
  virtual void writeFolderConfig(void);

  /** Read configuration options after widgets are created. */
  virtual void readConfig(void);

  /** Write configuration options. */
  virtual void writeConfig(void);

  /** Easy access to main components of the window. */
  KMReaderWin* messageView(void) const { return mMsgView; }
  KMFolderTree* folderTree(void) const  { return mFolderTree; }
  KMail::FavoriteFolderView *favoriteFolderView() const { return mFavoriteFolderView; }

  static void cleanup();

  KAction *action( const char *name ) { return mActionCollection->action( name ); }
  KActionMenu *customReplyAction() const { return mCustomReplyActionMenu; }
  KActionMenu *customReplyAllAction() const { return mCustomReplyAllActionMenu; }
  KActionMenu *forwardMenu() const { return mForwardActionMenu; }
  KAction *forwardInlineAction() const { return mForwardInlineAction; }
  KAction *forwardAttachedAction() const { return mForwardAttachedAction; }
  KAction *forwardDigestAction() const { return mForwardDigestAction; }
  KAction *redirectAction() const { return mRedirectAction; }
  KActionMenu *customForwardAction() const { return mCustomForwardActionMenu; }
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

  KActionMenu *statusMenu()  const{ return mMsgActions->messageStatusMenu(); }
  KActionMenu *threadStatusMenu() const { return mThreadStatusMenu; }
  KActionMenu *moveActionMenu() const{ return mMoveActionMenu; }
  KActionMenu *mopyActionMenu() const { return mCopyActionMenu; }
  KActionMenu *applyFilterActionsMenu() const { return mApplyFilterActionsMenu; }

  KToggleAction *watchThreadAction() const { return mWatchThreadAction; }
  KToggleAction *ignoreThreadAction() const { return mIgnoreThreadAction; }

  KMHeaders *headers() const { return mHeaders; }
  void toggleSystemTray();

  void updateListFilterAction();

  /** Returns a list of all KMMainWidgets. Warning, the list itself can be 0.
   * @return the list of all main widgets, or 0 if it is not yet initialized */
  static const QValueList<KMMainWidget*>* mainWidgetList() { return s_mainWidgetList; }

  KMSystemTray *systray() const;

  /** Checks a shortcut against the actioncollection and returns whether it
   * is already used and therefor not valid or not. */
  bool shortcutIsValid( const KShortcut& ) const;


  void modifyFolder( KMFolderTreeItem* folderItem );

  /**
   * Enable or disable the global accelerators. This is useful for keyboard
   * navigation inside child widgets like combo boxes.
   */
  void setAccelsEnabled( bool enabled = true );

  /**
   * Sets up action list for forward menu.
  */
  void setupForwardingActionsList();

  KStatusBarLabel* vacationScriptIndicator() const { return mVacationScriptIndicator; }
  void updateVactionScriptStatus() { updateVactionScriptStatus( mVacationIndicatorActive ); }

public slots:
  void slotMoveMsgToFolder( KMFolder *dest);
  void slotTrashMsg();   // move to trash

  virtual void show();
  virtual void hide();
  /** sven: moved here as public */
  void slotCheckMail();

  /**
   * Select the given folder
   * If the folder is 0 the intro is shown
   */
  void folderSelected( KMFolder*, bool forceJumpToUnread = false );

  /** Reselect current folder */
  void folderSelected();

  /** Select the folder and jump to the next unread msg */
  void folderSelectedUnread( KMFolder* );

  void slotMsgSelected(KMMessage*);
  void slotMsgChanged();

  /** Change the current folder, select a message in the current folder */
  void slotSelectFolder(KMFolder*);
  void slotSelectMessage(KMMessage*);

  void slotReplaceMsgByUnencryptedVersion();

  /** Update message menu */
  void updateMessageMenu();
  /** Start a timer to update message actions */
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

  void updateCustomTemplateMenus();
  void slotEditVacation();

signals:
  void messagesTransfered( bool );
  void captionChangeRequest( const QString & caption );

protected:
  void setupActions();
  void createWidgets();
  void activatePanners();
  void showMsg( KMReaderWin *win, KMMessage *msg );
  void updateFileMenu();
  void newFromTemplate( KMMessage *msg );

  KActionCollection * actionCollection() const { return mActionCollection; }

  /** @return the correct config dialog depending on whether the parent of the mainWidget
   *          is a KPart or a KMMainWindow. When dealing with geometries, use this pointer
   */
  KConfig * config();

protected slots:
  void slotCheckOneAccount(int);
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
  void slotViewChange();
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
  void slotSetThreadStatusTodo();
  void slotSetThreadStatusFlag();
  void slotSetThreadStatusWatched();
  void slotSetThreadStatusIgnored();
  void slotToggleUnread();
  void slotToggleTotalColumn();
  void slotToggleSizeColumn();
  void slotSendQueued();
  void slotSendQueuedVia( int item );
  void slotOnlineStatus();
  void slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type );
  void slotMsgPopup(KMMessage &msg, const KURL &aUrl, const QPoint&);
  void slotMarkAll();
  void slotMemInfo();
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
  void slotNextImportantMessage();
  void slotNextUnreadFolder();
  void slotPrevMessage();
  void slotPrevUnreadMessage();
  void slotPrevImportantMessage();
  void slotPrevUnreadFolder();

  /** etc. */
  void slotDisplayCurrentMessage();
  void slotMsgActivated( KMMessage* );

  void slotShowNewFromTemplate();
  void slotNewFromTemplate( int );

  /** Update the undo action */
  void slotUpdateUndo();

  /** Move selected messages to folder with corresponding to given menuid */
  virtual void moveSelectedToFolder( int menuId );
  /** Copy selected messages to folder with corresponding to given menuid */
  virtual void copySelectedToFolder( int menuId );
  /** Update html and threaded messages preferences in Folder menu. */
  void updateFolderMenu();
  /** Enable or disable the "mark all as read" action. Needs to happen more
   * often the the other updates and is therefor in its own method. */
  void updateMarkAsReadAction();

  /** XML-GUI stuff */
  void slotEditNotifications();
  void slotEditKeys();

  /** changes the caption and displays the foldername */
  void slotChangeCaption(QListViewItem*);
  void removeDuplicates();

  void slotCustomReplyToMsg( int tid );
  void slotCustomReplyAllToMsg( int tid );
  void slotForwardInlineMsg();
  void slotForwardAttachedMsg();
  void slotForwardDigestMsg();
  void slotRedirectMsg();
  void slotCustomForwardMsg( int tid );
  void slotNoQuoteReplyToMsg();
  void slotSubjectFilter();
  void slotMailingListFilter();
  void slotFromFilter();
  void slotToFilter();
  void slotPrintMsg();
  void slotCreateTodo();

  void slotConfigChanged();
  /** Remove the shortcut actions associated with a folder. */
  void slotFolderRemoved( KMFolder *folder );

  /** Show a splash screen for the longer-lasting operation */
  void slotShowBusySplash();
  /** Show a message screen explaining that we are currently offline, when
   * an online folder is selected. */
  void showOfflinePage();

private:
  /** Get override character encoding. */
  QString overrideEncoding() const;

  void initializeIMAPActions( bool setState );

  /** Helper which finds the associated account if there is a current
   * folder and it is an imap or disconnected imap one.
   */
  KMail::ImapAccountBase* findCurrentImapAccountBase();

  /** Helper which finds the associated IMAP path if there is a current
   * folder and it is an imap or disconnected imap one.
   */
  QString findCurrentImapPath();

  void setupFolderView();

private slots:
  void slotRequestFullSearchFromQuickSearch();
  void updateVactionScriptStatus( bool active );

private:
  // Message actions
  KAction *mTrashAction, *mDeleteAction, *mTrashThreadAction,
    *mDeleteThreadAction, *mSaveAsAction, *mUseAction,
    *mSendAgainAction, *mApplyAllFiltersAction, *mFindInMessageAction,
    *mSaveAttachmentsAction, *mOpenAction, *mViewSourceAction,
    *mFavoritesCheckMailAction;
  // Composition actions
  KAction *mPrintAction,
    *mForwardInlineAction, *mForwardAttachedAction, *mForwardDigestAction,
    *mRedirectAction;
  KActionMenu *mForwardActionMenu;
  // Filter actions
  KActionMenu *mFilterMenu;
  KAction *mSubjectFilterAction, *mFromFilterAction, *mToFilterAction,
      *mListFilterAction;
  KActionMenu *mTemplateMenu;

  // Custom template actions menu
  KActionMenu *mCustomReplyActionMenu,
              *mCustomReplyAllActionMenu,
              *mCustomForwardActionMenu;
  // Signal mappers for custom template actions
  QSignalMapper *mCustomReplyMapper,
                *mCustomReplyAllMapper,
                *mCustomForwardMapper;

  KActionMenu *mThreadStatusMenu,
    *mMoveActionMenu, *mCopyActionMenu, *mApplyFilterActionsMenu;
  KAction *mMarkThreadAsNewAction;
  KAction *mMarkThreadAsReadAction;
  KAction *mMarkThreadAsUnreadAction;
  KToggleAction *mToggleThreadTodoAction;
  KToggleAction *mToggleThreadFlagAction;

  KToggleAction *mWatchThreadAction, *mIgnoreThreadAction;

  /** we need to access those KToggleActions from the foldertree-popup */
  KRadioAction* mUnreadColumnToggle;
  KRadioAction* mUnreadTextToggle;
  KToggleAction* mTotalColumnToggle;
  KToggleAction* mSizeColumnToggle;

  QVBox        *mSearchAndTree;
  QHBox        *mFolderQuickSearch;
  KMFolderTree *mFolderTree;
  KMail::FavoriteFolderView *mFavoriteFolderView;
  QWidget      *mFolderView;
  QSplitter    *mFolderViewParent;
  KMReaderWin  *mMsgView;
  QSplitter    *mPanner1, *mPanner2;
  QSplitter    *mFolderViewSplitter;
  KMHeaders    *mHeaders;
  QVBox        *mSearchAndHeaders;
  KToolBar     *mSearchToolBar;
  KMail::HeaderListQuickSearch *mQuickSearchLine;
  QGuardedPtr<KMFolder> mFolder;
  KMFolder     *mTemplateFolder;
  QPopupMenu   *mViewMenu, *mBodyPartsMenu;
  KAction       *mlistFilterAction;
  bool		mIntegrated;
  bool          mBeepOnNew;
  bool          mConfirmEmpty;
  QString       mStartupFolder;
  int		mMessageStatusId;
  QValueList<int> mPanner1Sep, mPanner2Sep;
  KURL          mUrlCurrent;
  QPopupMenu	*mActMenu;
  QPopupMenu    *mSendMenu;
  QPopupMenu	*mFileMenu;

  bool mLongFolderList;

  bool		mStartupDone;
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

  KAction *mNewFolderAction, *mModifyFolderAction, *mRemoveFolderAction, *mExpireFolderAction,
      *mCompactFolderAction, *mRefreshFolderAction, *mEmptyFolderAction,
      *mMarkAllAsReadAction, *mFolderMailingListPropertiesAction,
      *mFolderShortCutCommandAction, *mTroubleshootFolderAction,
      *mRemoveDuplicatesAction;
  KToggleAction *mPreferHtmlAction, *mPreferHtmlLoadExtAction, *mThreadMessagesAction;
  KToggleAction *mThreadBySubjectAction;
  KToggleAction *mFolderAction, *mHeaderAction, *mMimeAction;

  QTimer *menutimer;
  QTimer *mShowBusySplashTimer;

  QGuardedPtr<KMail::Vacation> mVacation;
#if !defined(NDEBUG)
  QGuardedPtr<KMail::SieveDebugDialog> mSieveDebugDialog;
#endif
  KActionCollection *mActionCollection;
  KActionSeparator  *mToolbarActionSeparator;
  QVBoxLayout *mTopLayout;
  bool mDestructed, mForceJumpToUnread, mShowingOfflineScreen;
  QPtrList<KAction> mFilterMenuActions;
  QPtrList<KAction> mFilterTBarActions;
  QPtrList<KMMetaFilterActionCommand> mFilterCommands;
  QDict<FolderShortcutCommand> mFolderShortcutCommands;
  QGuardedPtr <KMail::FolderJob> mJob;

  QValueVector<QString> mCustomTemplates;
  QPtrList<KAction> mCustomTemplateActions;

  KMSystemTray  *mSystemTray;
  KConfig *mConfig;
  KXMLGUIClient *mGUIClient;

  KMail::MessageActions *mMsgActions;

  static QValueList<KMMainWidget*>* s_mainWidgetList;

  KStatusBarLabel *mVacationScriptIndicator;
  bool mVacationIndicatorActive;
};

#endif

