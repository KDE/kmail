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
#include <qlistview.h>

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
class KProgressDialog;
class KMenuBar;

class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMFolderTreeItem;
class KMHeaders;
class KMCommand;
class KMMetaFilterActionCommand;
class KMMessage;
class KMFolder;
class KMAccount;
class KMFldSearch;
class KMLittleProgressDlg;
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
}

typedef QMap<int,KMFolder*> KMMenuToFolder;


class KMMainWidget : public QWidget
{
  Q_OBJECT

public:
  KMMainWidget(QWidget *parent, const char *name,
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
  KAction *replyAction() { return mReplyAction; }
  KAction *replyAuthorAction() { return mReplyAuthorAction; }
  KAction *replyAllAction() { return mReplyAllAction; }
  KAction *replyListAction() { return mReplyListAction; }
  KActionMenu *forwardMenu() { return mForwardActionMenu; }
  KAction *forwardAction() { return mForwardAction; }
  KAction *forwardAttachedAction() { return mForwardAttachedAction; }
  KAction *redirectAction() { return mRedirectAction; }
  KAction *bounceAction() { return mBounceAction; }
  KAction *noQuoteReplyAction() { return mNoQuoteReplyAction; }
  KActionMenu *filterMenu() { return mFilterMenu; }
  KAction *printAction() { return mPrintAction; }
  KAction *trashAction() { return mTrashAction; }
  KAction *deleteAction() { return mDeleteAction; }
  KAction *saveAsAction() { return mSaveAsAction; }
  KAction *editAction() { return mEditAction; }
  KAction *sendAgainAction() { return mSendAgainAction; }
  KAction *applyFiltersAction() { return mSendAgainAction; }
  KAction *findInMessageAction() { return mFindInMessageAction; }
  KAction *saveAttachmentsAction() { return mSaveAttachmentsAction; }

  KActionMenu *statusMenu() { return mStatusMenu; }
  KActionMenu *threadStatusMenu() { return mThreadStatusMenu; }
  KActionMenu *moveActionMenu() { return mMoveActionMenu; }
  KActionMenu *mopyActionMenu() { return mCopyActionMenu; }
  KActionMenu *applyFilterActionsMenu() { return mApplyFilterActionsMenu; }

  KToggleAction *watchThreadAction() { return mWatchThreadAction; }
  KToggleAction *ignoreThreadAction() { return mIgnoreThreadAction; }

  // Forwarded to the reader window.
  KToggleAction *toggleFixFontAction() { return mMsgView->toggleFixFontAction(); }
  KAction *viewSourceAction() { return mMsgView->viewSourceAction(); }

  void folderSelected(KMFolder*, bool jumpToUnread);
  KMHeaders *headers() const { return mHeaders; }
  KMLittleProgressDlg* progressDialog() const;

  void toggleSystray(bool enabled, int mode);

  void updateListFilterAction();

public slots:
  void slotMoveMsgToFolder( KMFolder *dest);
  void slotTrashMsg();   // move to trash

  virtual void show();
  virtual void hide();
  /** sven: moved here as public */
  void slotCheckMail();

  /** Output given message in the statusbar message field. */
  void folderSelected(KMFolder*);
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
  void statusMsg(const QString&);

  /** Launch subscription-dialog */
  void slotSubscriptionDialog();

  /** The columns of the foldertree changed */
  void slotFolderTreeColumnsChanged();

signals:
  void messagesTransfered(bool);
  void captionChangeRequest( const QString & caption );

protected:
  void setupActions();
  void setupStatusBar();
  void createWidgets();
  void activatePanners();
  void showMsg(KMReaderWin *win, KMMessage *msg);

  KActionCollection * actionCollection() { return mActionCollection; }

  KRadioAction * actionForHeaderStyle( const KMail::HeaderStyle *,
                                       const KMail::HeaderStrategy * );
  KRadioAction * actionForAttachmentStrategy( const KMail::AttachmentStrategy * );

  /** @return the correct config dialog depending on whether the parent of the mainWidget
   *          is a KPart or a KMMainWindow. When dealing with geometries, use this pointer
   */
  KConfig * config();

protected slots:
  void slotCheckOneAccount(int);
  void slotMailChecked(bool newMail, bool sendOnCheck);
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
  void slotOverrideThread();
  void slotToggleSubjectThreading();
  void slotMessageQueuedOrDrafted();
  void slotEditMsg();
  //void slotTrashMsg();   // move to trash
  void slotDeleteMsg();  // completely delete message
  void slotUndo();
  void slotReadOn();
  void slotSaveMsg();
  void slotSaveAttachments();
  void slotMoveMsg();
  //void slotMoveMsgToFolder( KMFolder *dest);
  void slotCopyMsgToFolder( KMFolder *dest);
  void slotCopyMsg();
  void slotResendMsg();
  void slotEditVacation();
  void slotApplyFilters();
  void slotExpandThread();
  void slotExpandAllThreads();
  void slotCollapseThread();
  void slotCollapseAllThreads();
  void slotSetMsgStatusNew();
  void slotSetMsgStatusUnread();
  void slotSetMsgStatusRead();
  void slotSetMsgStatusReplied();
  void slotSetMsgStatusForwarded();
  void slotSetMsgStatusQueued();
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
  void slotSetThreadStatusSent();
  void slotSetThreadStatusFlag();
  void slotSetThreadStatusSpam();
  void slotSetThreadStatusHam();
  void slotSetThreadStatusWatched();
  void slotSetThreadStatusIgnored();
  void slotToggleUnread();
  void slotToggleTotalColumn();
  void slotBriefHeaders();
  void slotFancyHeaders();
  void slotStandardHeaders();
  void slotLongHeaders();
  void slotAllHeaders();
  void slotIconicAttachments();
  void slotSmartAttachments();
  void slotInlineAttachments();
  void slotHideAttachments();
  void slotCycleHeaderStyles();
  void slotCycleAttachmentStrategy();
  void slotSetEncoding();
  void slotSendQueued();
  void slotMsgPopup(KMMessage &msg, const KURL &aUrl, const QPoint&);
  void slotMarkAll();
  void slotMemInfo();
  void slotSearch();
  void slotSearchClosed();
  void slotFind();
  void slotUpdateImapMessage(KMMessage *msg);
  void slotIntro();
  void slotShowStartupFolder();
  /** Show tip-of-the-day on startup */
  void slotShowTipOnStart();
  /** Show tip-of-the-day, forced */
  void slotShowTip();
  void slotAntiSpamWizard();

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


  /** XML-GUI stuff */
  void slotEditNotifications();
  void slotEditKeys();

  /** changes the caption and displays the foldername */
  void slotChangeCaption(QListViewItem*);
  void removeDuplicates();
  /** Create actions for marked filters */
  void initializeFilterActions();
  /** Plug filter actions into a popup menu */
  void plugFilterActions(QPopupMenu*);

  /** Slot to reply to a message */
  void slotReplyToMsg();
  void slotReplyAuthorToMsg();
  void slotReplyListToMsg();
  void slotReplyAllToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotBounceMsg();
  void slotNoQuoteReplyToMsg();
  void slotSubjectFilter();
  void slotMailingListFilter();
  void slotFromFilter();
  void slotToFilter();
  void slotPrintMsg();

  void slotConfigChanged();

private:
  // Message actions
  KAction *mTrashAction, *mDeleteAction, *mSaveAsAction, *mEditAction,
    *mSendAgainAction, *mApplyFiltersAction, *mFindInMessageAction,
    *mSaveAttachmentsAction;
  // Composition actions
  KAction *mPrintAction, *mReplyAction, *mReplyAllAction, *mReplyAuthorAction,
      *mReplyListAction,
      *mForwardAction, *mForwardAttachedAction, *mRedirectAction,
      *mBounceAction, *mNoQuoteReplyAction;
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
  KToggleAction *mToggleThreadSentAction;
  KToggleAction *mToggleThreadFlagAction;
  KAction *mMarkThreadAsSpamAction;
  KAction *mMarkThreadAsHamAction;
  KToggleAction *mToggleRepliedAction;
  KToggleAction *mToggleForwardedAction;
  KToggleAction *mToggleQueuedAction;
  KToggleAction *mToggleSentAction;
  KToggleAction *mToggleFlagAction;
  KAction *mMarkAsSpamAction;
  KAction *mMarkAsHamAction;

  KToggleAction *mWatchThreadAction, *mIgnoreThreadAction;

  /** we need to access those KToggleActions from the foldertree-popup */
  KRadioAction* mUnreadColumnToggle;
  KRadioAction* mUnreadTextToggle;
  KToggleAction* mTotalColumnToggle;

private:
  KMFolderTree *mFolderTree;
  KMReaderWin  *mMsgView;
  QSplitter    *mPanner1, *mPanner2;
  KMHeaders    *mHeaders;
  KMFolder     *mFolder;
  const QTextCodec   *mCodec;
  QPopupMenu   *mViewMenu, *mBodyPartsMenu;
  KSelectAction *mEncoding;
  KAction       *mlistFilterAction;
  QCString	mEncodingStr;
  bool		mIntegrated;
  bool          mSendOnCheck;
  bool          mBeepOnNew, mSystemTrayOnNew;
  int           mSystemTrayMode;
  bool          mConfirmEmpty;
  QString       mStartupFolder;
  int		mMessageStatusId;
  QValueList<int> mPanner1Sep, mPanner2Sep;
  KMMessage     *mMsgCurrent;
  KURL          mUrlCurrent;
  QPopupMenu	*mActMenu;
  QPopupMenu	*mFileMenu;

  bool mLongFolderList;

  bool		mStartupDone;
  KMMenuToFolder mMenuToFolder;
  int copyId, moveId, htmlId, threadId;
  bool mHtmlPref, mThreadPref, mFolderHtmlPref, mFolderThreadPref,
       mFolderThreadSubjPref, mReaderWindowActive, mReaderWindowBelow;

//  QPopupMenu *mMessageMenu;
  KMFldSearch *mSearchWin;

  KAction *mModifyFolderAction, *mRemoveFolderAction, *mExpireFolderAction,
      *mCompactFolderAction, *mRefreshFolderAction, *mEmptyFolderAction,
      *mMarkAllAsReadAction;
  KToggleAction *mPreferHtmlAction, *mThreadMessagesAction;
  KToggleAction *mThreadBySubjectAction;
  KToggleAction *mFolderAction, *mHeaderAction, *mMimeAction;

  QTimer *menutimer;

  // ProgressDialog for transferring messages
  KProgressDialog* mProgressDialog;
  int mCountJobs, mCountMsgs;

  KMLittleProgressDlg *mLittleProgress;

  QPtrList<KMMessage> mSelectedMsgs;
  QGuardedPtr<KMail::Vacation> mVacation;
  KActionCollection *mActionCollection;
  QVBoxLayout *mTopLayout;
  bool mDestructed;
  QPtrList<KAction> mFilterActions;
  QPtrList<KMMetaFilterActionCommand> mFilterCommands;
  QGuardedPtr <KMail::FolderJob> mJob;

  KMSystemTray  *mSystemTray;
  KConfig *mConfig;

};

#endif

