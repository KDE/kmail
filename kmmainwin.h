/* kmail main window
 * Maintained by Stefan Taferner <taferner@kde.org>
 * This code is under the GPL
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include "kmtopwidget.h"
#include "cryptplugwrapperlist.h"
#include <kurl.h>
#include <kdockwidget.h>

class ConfigureDialog;
class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMFolderTreeItem;
class KMMimePartTree;
class KMHeaders;
class KMReaderWin;
class QSplitter;
class QTextCodec;
class KMenuBar;
class KToolBar;
class KStatusBar;
class KMMessage;
class KMFolder;
class KMAccount;
class KMLittleProgressDlg;
class KMFldSearch;
class KToggleAction;
class KActionMenu;
class KSelectAction;
class KRadioAction;
template <typename T> class QValueList;
template <typename T, typename S> class QMap;

namespace KIO
{
  class Job;
}

#define KMMainWinInherited KMTopLevelWidget
typedef QMap<int,KMFolder*> KMMenuToFolder;

typedef enum
{
    KMWidgetLayoutLongFolderListMimeRightMiddle =0x0122,
    KMWidgetLayoutLongFolderListMimeRightBottom =0x0124,
    KMWidgetLayoutShortFolderListMimeLeftMiddle =0x0212,
    KMWidgetLayoutShortFolderListMimeRightMiddle=0x0222,
    KMWidgetLayoutShortFolderListMimeLeftBottom =0x0214,
    KMWidgetLayoutShortFolderListMimeRightBottom=0x0224,
    KMWidgetLayoutShortFolderListMimeAllMiddle  =0x0232,
    KMWidgetLayoutShortFolderListMimeAllBottom  =0x0234,
    KMWidgetLayoutMaxWidthForAllWidgets         =0x1000,
    KMWidgetLayoutMaxHeightForAllWidgets        =0x2000
} KMWidgetLayout;


class KMMainWin : public KMTopLevelWidget
{
  Q_OBJECT

public:
  KMMainWin(QWidget *parent = 0, char *name = 0);
  virtual ~KMMainWin();

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
  KStatusBar* statusBar(void) const   { return mStatusBar; }
  KMFolderTree* folderTree(void) const  { return mFolderTree; }

  /** Returns a popupmenu containing a hierarchy of folder names
      starting at the given (@p aFolderDir) folder directory
      Each item in the popupmenu is connected to a slot, if
      move is TRUE this slot will cause all selected messages to
      be moved into the given folder, otherwise messages will be
      copied.
      Am empty @ref KMMenuToFolder must be passed in. */
  virtual QPopupMenu* folderToPopupMenu(KMFolderTreeItem* fti,
					bool move,
					QObject *receiver,
					KMMenuToFolder *aMenuToFolder,
					QPopupMenu *menu);

  static void cleanup();
  KAction *replyAction, *noQuoteReplyAction, *replyAllAction, *replyListAction,
    *forwardAction, *forwardAttachedAction, *redirectAction,
    *deleteAction, *saveAsAction, *bounceAction, *editAction,
    *viewSourceAction, *printAction, *sendAgainAction;
  KToggleAction *toggleFixFontAction;
  KActionMenu *filterMenu, *statusMenu, *threadStatusMenu,
    *moveActionMenu, *copyActionMenu;
  CryptPlugWrapperList mCryptPlugList;

  void folderSelected(KMFolder*, bool jumpToUnread);

  /** Jump to any message in any folder.  The message serial number of the
      argument message is used to locate the original message, which
      is then returned. */
  KMMessage *jumpToMessage(KMMessage *aMsg);

public slots:
  virtual void show();
  virtual void hide();
  /** sven: moved here as public */
  void slotCheckMail();
  /** sven: called from reader */
  void slotAtmMsg(KMMessage *msg);

  /** Output given message in the statusbar message field. */
  void statusMsg(const QString&);
  void htmlStatusMsg(const QString&);
  void folderSelected(KMFolder*);
  void folderSelectedUnread( KMFolder* );

  void slotMsgSelected(KMMessage*);

  /** Change the current folder, select a message in the current folder */
  void slotSelectFolder(KMFolder*);
  void slotSelectMessage(KMMessage*);

  /** Update message menu */
  void updateMessageMenu();
  /** Start a timer to update message actions */
  void startUpdateMessageActionsTimer();
  /** Update message actions */
  void updateMessageActions();

protected:
  void setupMenuBar();
  void setupStatusBar();
  void createWidgets();
  void activatePanners();
  void showMsg(KMReaderWin *win, KMMessage *msg);
  virtual bool queryClose();

protected slots:
  void displayStatusMsg(const QString&);
  void slotCheckOneAccount(int);
  void slotMailChecked(bool newMail, bool sendOnCheck);
  void getAccountMenu();
  void slotQuit();
  void slotHelp();
  void slotNewMailReader();
  void slotSettings();
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
  void slotReplyToMsg();
  void slotNoQuoteReplyToMsg();
  void slotReplyAllToMsg();
  void slotReplyListToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotBounceMsg();
  void slotMessageQueuedOrDrafted();
  void slotEditMsg();
  void slotEditMsg(KMMessage*);
  void slotDeleteMsg();
  void slotUndo();
  void slotReadOn();
  void slotSaveMsg();
  void slotPrintMsg();
  void slotMoveMsg();
  void slotMoveMsgToFolder( KMFolder *dest);
  void slotCopyMsgToFolder( KMFolder *dest);
  void slotCopyMsg();
  void slotResendMsg();
  void slotApplyFilters();
  void slotSubjectFilter();
  void slotMailingListFilter();
  void slotFromFilter();
  void slotToFilter();
  void slotExpandThread();
  void slotExpandAllThreads();
  void slotCollapseThread();
  void slotCollapseAllThreads();
  void slotSetMsgStatusNew();
  void slotSetMsgStatusUnread();
  void slotSetMsgStatusRead();
  void slotSetMsgStatusReplied();
  void slotSetMsgStatusQueued();
  void slotSetMsgStatusSent();
  void slotSetMsgStatusFlag();
  void slotSetThreadStatusNew();
  void slotSetThreadStatusUnread();
  void slotSetThreadStatusRead();
  void slotSetThreadStatusReplied();
  void slotSetThreadStatusQueued();
  void slotSetThreadStatusSent();
  void slotSetThreadStatusFlag();
  void slotShowMsgSrc();
  void slotToggleFixedFont();
  void slotBriefHeaders();
  void slotFancyHeaders();
  void slotStandardHeaders();
  void slotLongHeaders();
  void slotAllHeaders();
  void slotIconicAttachments();
  void slotSmartAttachments();
  void slotInlineAttachments();
  void slotCycleHeaderStyles();
  void slotCycleAttachmentStyles();
  void slotSetEncoding();
  void slotSendQueued();
  void slotMsgPopup(KMMessage &msg, const KURL &aUrl, const QPoint&);
  void slotUrlClicked(const KURL &url, int button);
  void slotCopyText();
  void slotMarkAll();
  void slotSelectText();
  void slotMemInfo();
  void slotSearch();
  void slotSearchClosed();
  void slotFind();
  void slotUpdateImapMessage(KMMessage *msg);


  // FIXME: ACTIVATE this when KDockWidgets are working nicely (khz, 19.04.2002)
  /*
  void slotToggleFolderBar();
  void slotToggleHeaderBar();
  void slotToggleMimeBar();
  */
  // (khz, 19.04.2002)


  /** Message navigation */
  void slotNextMessage();
  void slotNextUnreadMessage();
  void slotPrevMessage();
  void slotPrevUnreadMessage();

  /** etc. */
  void slotMsgActivated(KMMessage*);

  /** Operations on mailto: URLs. */
  void slotMailtoCompose();
  void slotMailtoReply();
  void slotMailtoForward();
  void slotMailtoAddAddrBook();

  /** Open URL in mUrlCurrent using Kfm. */
  void slotUrlOpen();

  /** Save the page to a file */
  void slotUrlSave();
  void slotUrlSaveResult(KIO::Job*);

  /** Copy URL in mUrlCurrent to clipboard. Removes "mailto:" at
      beginning of URL before copying. */
  void slotUrlCopy();

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
  void slotToggleToolBar();
  void slotToggleStatusBar();
  void slotEditToolbars();
  void slotUpdateToolbars();
  void slotEditKeys();

protected:
  KRadioAction * actionForHeaderStyle(int);
  KRadioAction * actionForAttachmentStyle(int);

protected:
  KStatusBar   *mStatusBar;
  QString      mLastStatusMsg;
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
  bool          mBeepOnNew, mBoxOnNew, mExecOnNew;
  bool          mConfirmEmpty;
  QString       mNewMailCmd;
  int		mMessageStatusId;
  QValueList<int> *mPanner1Sep, *mPanner2Sep, *mPanner3Sep;
  KMMessage     *mMsgCurrent;
  KURL          mUrlCurrent;
  QPopupMenu	*actMenu;
  QPopupMenu	*fileMenu;
  KMWidgetLayout mWidgetLayout;

  int mWindowLayout;
  bool mShowMIME;

  bool		mStartupDone;
  KMMenuToFolder mMenuToFolder;
  int copyId, moveId, htmlId, threadId;
  bool mHtmlPref, mThreadPref, mFolderHtmlPref, mFolderThreadPref;
  QPopupMenu *messageMenu;
  KMLittleProgressDlg *littleProgress;
  KMFldSearch *searchWin;
  ConfigureDialog *mConfigureDialog;
  bool mbNewMBVisible;

  KAction *modifyFolderAction, *removeFolderAction, *expireFolderAction,
    *compactFolderAction, *emptyFolderAction, *markAllAsReadAction;
  KToggleAction *preferHtmlAction, *threadMessagesAction;
  KToggleAction *toolbarAction, *statusbarAction, *folderAction, *headerAction, *mimeAction;

  QTimer *menutimer;

  KDockWidget* mMsgDock;
  KDockWidget* mHeaderDock;
  KDockWidget* mFolderDock;
  KDockWidget* mMimeDock;
};

#endif

