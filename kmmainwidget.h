 /* -*- mode: C++; c-file-style: "gnu" -*-
 * kmail main window
 * Copyright 2002 Don Sanders <sanders@kde.org>
 * Based on the work of Stefan Taferner <taferner@kde.org>
 *
 * License GPL
 */
#ifndef __KMMAINWIDGET
#define __KMMAINWIDGET

#include <kurl.h>
#include <kxmlguiclient.h>
#include <qlistview.h>
#include <qvbox.h>

#include "kmreaderwin.h" //for inline actions
#include "kmkernel.h" // for access to config
#include <kaction.h>

class QVBoxLayout;
class QSplitter;
class QTextCodec;

class KActionMenu;
class KActionCollection;
class KConfig;
class KSelectAction;
class KRadioAction;
class KToggleAction;
class KMenuBar;

class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMFolderTreeItem;
class KMHeaders;
class KMCommand;
class KMMetaFilterActionCommand;
class FolderShortcutCommand;
class KMMessage;
class KMFolder;
class KMAccount;
class KMFldSearch;
class KMSystemTray;

template <typename T> class QValueList;
template <typename T, typename S> class QMap;
template <typename T> class QGuardedPtr;

namespace KIO {
  class Job;
}

namespace KMail {
  class Vacation;
  class AttachmentStrategy;
  class HeaderStrategy;
  class HeaderStyle;
  class FolderJob;
  class HeaderListQuickSearch;
}

typedef QMap<int,KMFolder*> KMMenuToFolder;


class KMMainWidget : public QWidget
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

  static void cleanup();

  KAction *action( const char *name ) { return mActionCollection->action( name ); }
  KAction *replyAction() const { return mReplyAction; }
  KAction *replyAuthorAction() const { return mReplyAuthorAction; }
  KAction *replyAllAction() const { return mReplyAllAction; }
  KAction *replyListAction() const { return mReplyListAction; }
  KActionMenu * replyMenu() const { return mReplyActionMenu; }
  KActionMenu *forwardMenu() const { return mForwardActionMenu; }
  KAction *forwardAction() const { return mForwardAction; }
  KAction *forwardAttachedAction() const { return mForwardAttachedAction; }
  KAction *redirectAction() const { return mRedirectAction; }
  KAction *noQuoteReplyAction() const { return mNoQuoteReplyAction; }
  KActionMenu *filterMenu() const { return mFilterMenu; }
  KAction *printAction() const { return mPrintAction; }
  KAction *trashAction() const { return mTrashAction; }
  KAction *deleteAction() const { return mDeleteAction; }
  KAction *saveAsAction() const { return mSaveAsAction; }
  KAction *editAction() const { return mEditAction; }
  KAction *sendAgainAction() const { return mSendAgainAction; }
  KAction *applyAllFiltersAction() const { return mApplyAllFiltersAction; }
  KAction *findInMessageAction() const { return mFindInMessageAction; }
  KAction *saveAttachmentsAction() const { return mSaveAttachmentsAction; }
  KAction *openAction() const { return mOpenAction; }
  KAction *viewSourceAction() { return mViewSourceAction; }

  KActionMenu *statusMenu()  const{ return mStatusMenu; }
  KActionMenu *threadStatusMenu() const { return mThreadStatusMenu; }
  KActionMenu *moveActionMenu() const{ return mMoveActionMenu; }
  KActionMenu *mopyActionMenu() const { return mCopyActionMenu; }
  KActionMenu *applyFilterActionsMenu() const { return mApplyFilterActionsMenu; }

  KToggleAction *watchThreadAction() const { return mWatchThreadAction; }
  KToggleAction *ignoreThreadAction() const { return mIgnoreThreadAction; }



  KMHeaders *headers() const { return mHeaders; }
  void toggleSystemTray();

  void updateListFilterAction();

  /// @return a list of all KMMainWidgets. Warning, the list itself can be 0.
  static QPtrList<KMMainWidget>* mainWidgetList() { return s_mainWidgetList; }

  KMSystemTray *systray() const;

  /** Checks a shortcut against the actioncollection and returns whether it
   * is already used and therefor not valid or not. */
  bool shortcutIsValid( const KShortcut& ) const;


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

  /** Launch subscription-dialog */
  void slotSubscriptionDialog();

  /** The columns of the foldertree changed */
  void slotFolderTreeColumnsChanged();

  /** Clear and create actions for marked filters */
  void clearFilterActions();
  void initializeFilterActions();

  /** Create actions for the folder shortcuts. */
  void initializeFolderShortcutActions();

  /** Add, remove or adjust the folder's shortcut. */
  void slotShortcutChanged( KMFolder *folder );

signals:
  void messagesTransfered( bool );
  void captionChangeRequest( const QString & caption );

protected:
  void setupActions();
  void createWidgets();
  void activatePanners();
  void showMsg(KMReaderWin *win, KMMessage *msg);
  void updateFileMenu();

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
  void slotHelp();
  void slotNewMailReader();
  void slotFilter();
  void slotPopFilter();
  void slotAddrBook();
  void slotImport();
  void slotViewChange();
  void slotAddFolder();
  void slotCompose();
  void slotPostToML();
  void slotModifyFolder();
  void slotExpireFolder();
  void slotExpireAll();
  void slotInvalidateIMAPFolders();
  void slotMarkAllAsRead();
  void slotRemoveFolder();
  void slotEmptyFolder();
  void slotCompactFolder();
  void slotRefreshFolder();
  void slotCompactAll();
  void slotOverrideHtml();
  void slotOverrideHtmlLoadExt();
  void slotOverrideThread();
  void slotToggleSubjectThreading();
  void slotMessageQueuedOrDrafted();
  void slotEditMsg();
  //void slotTrashMsg();   // move to trash
  void slotDeleteMsg( bool confirmDelete = true );  // completely delete message
  void slotUndo();
  void slotReadOn();
  void slotSaveMsg();
  void slotOpenMsg();
  void slotSaveAttachments();
  void slotMoveMsg();
  //void slotMoveMsgToFolder( KMFolder *dest);
  void slotCopyMsgToFolder( KMFolder *dest);
  void slotCopyMsg();
  void slotResendMsg();
  void slotEditVacation();
  void slotStartCertManager();
  void slotStartWatchGnuPG();
  void slotApplyFilters();
  void slotExpandThread();
  void slotExpandAllThreads();
  void slotCollapseThread();
  void slotCollapseAllThreads();
  void slotShowMsgSrc();
  void slotSetMsgStatusNew();
  void slotSetMsgStatusUnread();
  void slotSetMsgStatusRead();
  void slotSetMsgStatusReplied();
  void slotSetMsgStatusForwarded();
  void slotSetMsgStatusQueued();
  void slotSetMsgStatusTodo();
  void slotSetMsgStatusSent();
  void slotSetMsgStatusFlag();
  void slotSetMsgStatusSpam();
  void slotSetMsgStatusHam();
  void slotSetThreadStatusNew();
  void slotSetThreadStatusUnread();
  void slotSetThreadStatusRead();
  void slotSetThreadStatusReplied();
  void slotSetThreadStatusForwarded();
  void slotSetThreadStatusQueued();
  void slotSetThreadStatusTodo();
  void slotSetThreadStatusSent();
  void slotSetThreadStatusFlag();
  void slotSetThreadStatusSpam();
  void slotSetThreadStatusHam();
  void slotSetThreadStatusWatched();
  void slotSetThreadStatusIgnored();
  void slotToggleUnread();
  void slotToggleTotalColumn();
  void slotSetEncoding();
  void slotSendQueued();
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
  void slotMsgActivated(KMMessage*);

  /** Update the undo action */
  void slotUpdateUndo();

  /** Move selected messages to folder with corresponding to given menuid */
  virtual void moveSelectedToFolder( int menuId );
  /** Copy selected messages to folder with corresponding to given menuid */
  virtual void copySelectedToFolder( int menuId );
  /** Update html and threaded messages preferences in Folder menu. */
  virtual void updateFolderMenu();
  /** Enable or disable the "mark all as read" action. Needs to happen more
   * often the the other updates and is therefor in its own method. */
  void updateMarkAsReadAction();

  /** Settings menu */
  void slotToggleShowQuickSearch();

  /** XML-GUI stuff */
  void slotEditNotifications();
  void slotEditKeys();

  /** changes the caption and displays the foldername */
  void slotChangeCaption(QListViewItem*);
  void removeDuplicates();

  /** Slot to reply to a message */
  void slotReplyToMsg();
  void slotReplyAuthorToMsg();
  void slotReplyListToMsg();
  void slotReplyAllToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotNoQuoteReplyToMsg();
  void slotSubjectFilter();
  void slotMailingListFilter();
  void slotFromFilter();
  void slotToFilter();
  void slotPrintMsg();

  void slotConfigChanged();
  /** Remove the shortcut actions associated with a folder. */
  void slotFolderRemoved( KMFolder *folder );

private:
  // Message actions
  KAction *mTrashAction, *mDeleteAction, *mSaveAsAction, *mEditAction,
    *mSendAgainAction, *mApplyAllFiltersAction, *mFindInMessageAction,
    *mSaveAttachmentsAction, *mOpenAction, *mViewSourceAction;
  // Composition actions
  KAction *mPrintAction, *mReplyAction, *mReplyAllAction, *mReplyAuthorAction,
      *mReplyListAction,
      *mForwardAction, *mForwardAttachedAction, *mRedirectAction,
      *mNoQuoteReplyAction;
  KActionMenu *mReplyActionMenu;
  KActionMenu *mForwardActionMenu;
  // Filter actions
  KActionMenu *mFilterMenu;
  KAction *mSubjectFilterAction, *mFromFilterAction, *mToFilterAction,
      *mListFilterAction;

  KActionMenu *mStatusMenu, *mThreadStatusMenu,
    *mMoveActionMenu, *mCopyActionMenu, *mApplyFilterActionsMenu;
  KAction *mMarkThreadAsNewAction;
  KAction *mMarkThreadAsReadAction;
  KAction *mMarkThreadAsUnreadAction;
  KToggleAction *mToggleThreadRepliedAction;
  KToggleAction *mToggleThreadForwardedAction;
  KToggleAction *mToggleThreadQueuedAction;
  KToggleAction *mToggleThreadTodoAction;
  KToggleAction *mToggleThreadSentAction;
  KToggleAction *mToggleThreadFlagAction;
  KAction *mMarkThreadAsSpamAction;
  KAction *mMarkThreadAsHamAction;
  KToggleAction *mToggleRepliedAction;
  KToggleAction *mToggleForwardedAction;
  KToggleAction *mToggleQueuedAction;
  KToggleAction *mToggleTodoAction;
  KToggleAction *mToggleSentAction;
  KToggleAction *mToggleFlagAction;
  KAction *mMarkAsSpamAction;
  KAction *mMarkAsHamAction;

  KToggleAction *mWatchThreadAction, *mIgnoreThreadAction;

  /** we need to access those KToggleActions from the foldertree-popup */
  KRadioAction* mUnreadColumnToggle;
  KRadioAction* mUnreadTextToggle;
  KToggleAction* mTotalColumnToggle;

  KToggleAction *mToggleShowQuickSearchAction;
private:
  KMFolderTree *mFolderTree;
  KMReaderWin  *mMsgView;
  QSplitter    *mPanner1, *mPanner2;
  KMHeaders    *mHeaders;
  QVBox        *mSearchAndHeaders;
  KToolBar     *mSearchToolBar;
  KMail::HeaderListQuickSearch *mQuickSearchLine;
  KMFolder     *mFolder;
  const QTextCodec   *mCodec;
  QPopupMenu   *mViewMenu, *mBodyPartsMenu;
  KSelectAction *mEncoding;
  KAction       *mlistFilterAction;
  QCString	mEncodingStr;
  bool		mIntegrated;
  bool          mBeepOnNew;
  bool          mConfirmEmpty;
  QString       mStartupFolder;
  int		mMessageStatusId;
  QValueList<int> mPanner1Sep, mPanner2Sep;
  KURL          mUrlCurrent;
  QPopupMenu	*mActMenu;
  QPopupMenu	*mFileMenu;

  bool mLongFolderList;

  bool		mStartupDone;
  KMMenuToFolder mMenuToFolder;
  int copyId, moveId, htmlId, threadId;
  bool mHtmlPref, mHtmlLoadExtPref, mThreadPref,
       mFolderHtmlPref, mFolderHtmlLoadExtPref, mFolderThreadPref,
       mFolderThreadSubjPref, mReaderWindowActive, mReaderWindowBelow;

//  QPopupMenu *mMessageMenu;
  KMFldSearch *mSearchWin;

  KAction *mModifyFolderAction, *mRemoveFolderAction, *mExpireFolderAction,
      *mCompactFolderAction, *mRefreshFolderAction, *mEmptyFolderAction,
      *mMarkAllAsReadAction;
  KToggleAction *mPreferHtmlAction, *mPreferHtmlLoadExtAction, *mThreadMessagesAction;
  KToggleAction *mThreadBySubjectAction;
  KToggleAction *mFolderAction, *mHeaderAction, *mMimeAction;

  QTimer *menutimer;

  QGuardedPtr<KMail::Vacation> mVacation;
  KActionCollection *mActionCollection;
  KActionSeparator  *mToolbarActionSeparator;
  QVBoxLayout *mTopLayout;
  bool mDestructed, mForceJumpToUnread;
  QPtrList<KAction> mFilterMenuActions;
  QPtrList<KAction> mFilterTBarActions;
  QPtrList<KMMetaFilterActionCommand> mFilterCommands;
  QDict<FolderShortcutCommand> mFolderShortcutCommands;
  QGuardedPtr <KMail::FolderJob> mJob;

  KMSystemTray  *mSystemTray;
  KConfig *mConfig;
  KXMLGUIClient *mGUIClient;

  static QPtrList<KMMainWidget>* s_mainWidgetList;
};

#endif

