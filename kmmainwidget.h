/* kmail main window
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
#include <kaction.h>

class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMFolderTreeItem;
class KMHeaders;
class QVBoxLayout;
class QSplitter;
class QTextCodec;
class KMenuBar;
class KMCommand;
class KMMetaFilterActionCommand;
class KMMessage;
class KMFolder;
class KMAccount;
class KMFldSearch;
class KToggleAction;
class KActionMenu;
class KActionCollection;
class KSelectAction;
class KRadioAction;
class KProgressDialog;
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
	       KActionCollection *actionCollection );
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

  // Proxy the actions from the reader window,
  // but action( "some_name" ) some name could be used instead.
  KAction *action( const char *name ) { return mActionCollection->action( name ); }
  KAction *replyAction() { return mReplyAction; }
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

  // Forwarded to the reader window.
  KToggleAction *toggleFixFontAction() { return mMsgView->toggleFixFontAction(); }
  KAction *viewSourceAction() { return mMsgView->viewSourceAction(); }

  //FIXME: wtf? member variables in the public interface:
  KAction *trashAction, *deleteAction, *saveAsAction, *editAction,
    *sendAgainAction, *applyFiltersAction, *findInMessageAction, *saveAttachments;
  // Composition actions
  KAction *mPrintAction, *mReplyAction, *mReplyAllAction, *mReplyListAction,
      *mForwardAction, *mForwardAttachedAction, *mRedirectAction,
      *mBounceAction, *mNoQuoteReplyAction;
  KActionMenu *mForwardActionMenu;
  // Filter actions
  KActionMenu *mFilterMenu;
  KAction *mSubjectFilterAction, *mFromFilterAction, *mToFilterAction,
      *mListFilterAction;

  void updateListFilterAction();
  
  KActionMenu *statusMenu, *threadStatusMenu,
    *moveActionMenu, *copyActionMenu, *applyFilterActionsMenu;
  KAction *markThreadAsNewAction;
  KAction *markThreadAsReadAction;
  KAction *markThreadAsUnreadAction;
  KToggleAction *toggleThreadRepliedAction;
  KToggleAction *toggleThreadForwardedAction;
  KToggleAction *toggleThreadQueuedAction;
  KToggleAction *toggleThreadSentAction;
  KToggleAction *toggleThreadFlagAction;
  KToggleAction *toggleRepliedAction;
  KToggleAction *toggleForwardedAction;
  KToggleAction *toggleQueuedAction;
  KToggleAction *toggleSentAction;
  KToggleAction *toggleFlagAction;

  KToggleAction *watchThreadAction, *ignoreThreadAction;

  /** we need to access those KToggleActions from the foldertree-popup */
  KRadioAction* unreadColumnToggle;
  KRadioAction* unreadTextToggle;
  KToggleAction* totalColumnToggle;

  void folderSelected(KMFolder*, bool jumpToUnread);
  KMHeaders *headers() const { return mHeaders; }
  KMLittleProgressDlg* progressDialog() const;

  void toggleSystray(bool enabled, int mode);

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

protected:
  void setupActions();
  void setupStatusBar();
  void createWidgets();
  void activatePanners();
  void showMsg(KMReaderWin *win, KMMessage *msg);

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
  void slotSetThreadStatusNew();
  void slotSetThreadStatusUnread();
  void slotSetThreadStatusRead();
  void slotSetThreadStatusReplied();
  void slotSetThreadStatusForwarded();
  void slotSetThreadStatusQueued();
  void slotSetThreadStatusSent();
  void slotSetThreadStatusFlag();
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

protected:
  KActionCollection * actionCollection() { return mActionCollection; }

  KRadioAction * actionForHeaderStyle( const KMail::HeaderStyle *,
				       const KMail::HeaderStrategy * );
  KRadioAction * actionForAttachmentStrategy( const KMail::AttachmentStrategy * );

protected:
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
  QPopupMenu	*actMenu;
  QPopupMenu	*fileMenu;

  bool mLongFolderList;

  bool		mStartupDone;
  KMMenuToFolder mMenuToFolder;
  int copyId, moveId, htmlId, threadId;
  bool mHtmlPref, mThreadPref, mFolderHtmlPref, mFolderThreadPref, 
       mFolderThreadSubjPref, mReaderWindowActive, mReaderWindowBelow;
  
  QPopupMenu *messageMenu;
  KMFldSearch *searchWin;

  KAction *modifyFolderAction, *removeFolderAction, *expireFolderAction,
      *compactFolderAction, *refreshFolderAction, *emptyFolderAction,
      *markAllAsReadAction;
  KToggleAction *preferHtmlAction, *threadMessagesAction;
  KToggleAction *threadBySubjectAction;
  KToggleAction *folderAction, *headerAction, *mimeAction;

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
signals:
  void messagesTransfered(bool);
  void captionChangeRequest( const QString & caption );
};

#endif

