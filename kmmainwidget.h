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

class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMFolderTreeItem;
class KMMimePartTree;
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
class KSelectAction;
class KRadioAction;
class KProgressDialog;
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
  KAction *replyAction() { return mMsgView->replyAction(); }
  KAction *replyAllAction() { return mMsgView->replyAllAction(); }
  KAction *replyListAction() { return mMsgView->replyListAction(); }
  KActionMenu *forwardMenu() { return mForwardActionMenu; }
  KAction *forwardAction() { return mForwardAction; }
  KAction *forwardAttachedAction() { return mForwardAttachedAction; }
  KAction *redirectAction() { return mMsgView->redirectAction(); }
  KAction *bounceAction() { return mMsgView->bounceAction(); }
  KAction *noQuoteReplyAction() { return mMsgView->noQuoteReplyAction(); }
  KActionMenu *filterMenu() { return mMsgView->filterMenu(); }
  KToggleAction *toggleFixFontAction() { return mMsgView->toggleFixFontAction(); }
  KAction *viewSourceAction() { return mMsgView->viewSourceAction(); }
  KAction *printAction() { return mMsgView->printAction(); }

  //FIXME: wtf? member variables in the public interface:
  KAction *trashAction, *deleteAction, *saveAsAction, *editAction,
    *sendAgainAction, *mForwardAction, *mForwardAttachedAction,
    *applyFiltersAction;
  KActionMenu *statusMenu, *threadStatusMenu,
    *moveActionMenu, *copyActionMenu, *mForwardActionMenu,
    *applyFilterActionsMenu;

  /** we need to access those KToggleActions from the foldertree-popup */
  KToggleAction* unreadColumnToggle;
  KToggleAction* totalColumnToggle;

  void folderSelected(KMFolder*, bool jumpToUnread);
  KMHeaders *headers() { return mHeaders; }

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

protected:
  void setupActions();
  void createWidgets();
  void activatePanners();
  void showMsg(KMReaderWin *win, KMMessage *msg);
  virtual bool queryClose();

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
  void slotCompactAll();
  void slotOverrideHtml();
  void slotOverrideThread();
  void slotMessageQueuedOrDrafted();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotEditMsg();
  //void slotTrashMsg();   // move to trash
  void slotDeleteMsg();  // completely delete message
  void slotUndo();
  void slotReadOn();
  void slotSaveMsg();
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
  void slotToggleUnreadColumn();
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


  // FIXME: ACTIVATE this when KDockWidgets are working nicely (khz, 19.04.2002)
  /*
  void updateSettingsMenu();
  */
  // (khz, 19.04.2002)


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

protected:
  KActionCollection * actionCollection() { return mActionCollection; }

  KRadioAction * actionForHeaderStyle( const KMail::HeaderStyle *,
				       const KMail::HeaderStrategy * );
  KRadioAction * actionForAttachmentStrategy( const KMail::AttachmentStrategy * );

protected:
  KMFolderTree *mFolderTree;
  KMMimePartTree* mMimePartTree;
  KMReaderWin  *mMsgView;
  QSplitter    *mPanner1, *mPanner2, *mPanner3;
  KMHeaders    *mHeaders;
  KMFolder     *mFolder;
  QTextCodec   *mCodec;
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
  QValueList<int> mPanner1Sep, mPanner2Sep, mPanner3Sep;
  KMMessage     *mMsgCurrent;
  KURL          mUrlCurrent;
  QPopupMenu	*actMenu;
  QPopupMenu	*fileMenu;

  int mWindowLayout;
  int mShowMIMETreeMode;

  bool		mStartupDone;
  KMMenuToFolder mMenuToFolder;
  int copyId, moveId, htmlId, threadId;
  bool mHtmlPref, mThreadPref, mFolderHtmlPref, mFolderThreadPref;
  QPopupMenu *messageMenu;
  KMFldSearch *searchWin;

  KAction *modifyFolderAction, *removeFolderAction, *expireFolderAction,
      *compactFolderAction, *emptyFolderAction, *markAllAsReadAction;
  KToggleAction *preferHtmlAction, *threadMessagesAction;
  KToggleAction *folderAction, *headerAction, *mimeAction;

  QTimer *menutimer;

  // ProgressDialog for transfering messages
  KProgressDialog* mProgressDialog;
  int mCountJobs, mCountMsgs;

  QPtrList<KMMessage> mSelectedMsgs;
  QGuardedPtr<KMail::Vacation> mVacation;
  KActionCollection *mActionCollection;
  QVBoxLayout *mTopLayout;
  bool mDestructed;
  QPtrList<KAction> mFilterActions;
  QPtrList<KMMetaFilterActionCommand> mFilterCommands;

signals:
  void messagesTransfered(bool);
  void captionChangeRequest( const QString & caption );
};

#endif

