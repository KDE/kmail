/* kmail main window
 * Maintained by Stefan Taferner <taferner@kde.org>
 * This code is under the GPL
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include "kmtopwidget.h"
#include <qvaluelist.h>
#include <qmap.h>
#include <kurl.h>

class ConfigureDialog;
class KMFolder;
class KMFolderDir;
class KMFolderTree;
class KMHeaders;
class KMReaderWin;
class QSplitter;
class KMenuBar;
class KToolBar;
class KStatusBar;
class KMMessage;
class KMFolder;
class KMAccount;
class KMLittleProgressDlg;
class KMFldSearch;

#define KMMainWinInherited KMTopLevelWidget
typedef QMap<int,KMFolder*> KMMenuToFolder;

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
  KToolBar* toolBar(void) const     { return mToolBar; }
  KStatusBar* statusBar(void) const   { return mStatusBar; }
  KMFolderTree* folderTree(void) const  { return mFolderTree; }

  // Returns a popupmenu containing a hierarchy of folder names
  // starting at the given (aFolderDir) folder directory
  // Each item in the popupmenu is connected to a slot, if
  // move is TRUE this slot will cause all selected messages to
  // be moved into the given folder, otherwise messages will be
  // copied.
  // Am empty KMMenuToFolder must be passed in.
  virtual QPopupMenu* folderToPopupMenu(KMFolderDir* aFolderDir, 
					bool move,
					QObject *receiver,
					KMMenuToFolder *aMenuToFolder);

  static void cleanup();

public slots:
  virtual void show();
  virtual void hide();
  void slotCheckMail(); // sven moved here as public
  void slotAtmMsg(KMMessage *msg); //sven: called from reader

  /** Output given message in the statusbar message field. */
  void statusMsg(const QString& text);
  void folderSelected(KMFolder*);
  void slotMsgSelected(KMMessage*);
  
  /** Change the current folder, select a message in the current folder */
  void slotSelectFolder(KMFolder*);
  void slotSelectMessage(KMMessage*);

protected:
  void setupMenuBar();
  void setupToolBar();
  void setupStatusBar();
  void createWidgets();
  void activatePanners();
  void showMsg(KMReaderWin *win, KMMessage *msg);

protected slots:
  void slotCheckOneAccount(int);
  void slotMailChecked(bool newMail);
  void getAccountMenu(); 
  void slotMenuActivated();
  void slotClose();
  void slotHelp();
  void slotNewMailReader();
  void slotSettings();
  void slotFilter();
  void slotAddrBook();
  void slotUseKab();
  void slotUnimplemented();
  void slotViewChange();
  void slotAddFolder();
  void slotCompose();
  void slotModifyFolder();
  void slotRemoveFolder();
  void slotEmptyFolder();
  void slotCompactFolder();
  void slotOverrideHtml();
  void slotOverrideThread();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotForwardMsg();
  void slotRedirectMsg();
  void slotBounceMsg();
  void slotEditMsg();
  void slotDeleteMsg();
  void slotUndo();
  void slotSaveMsg();
  void slotPrintMsg();
  void slotMoveMsg();
  void slotMoveMsgToFolder( KMFolder *dest);
  void slotCopyMsg();
  void slotResendMsg();
  void slotApplyFilters();
  void slotSetMsgStatus(int);
  void slotShowMsgSrc();
  void slotSetHeaderStyle(int);
  void slotSendQueued();
  void slotMsgPopup(const KURL &url, const QPoint&);
  void slotUrlClicked(const KURL &url, int button);
  void slotCopyText();
  void slotMarkAll();
  void slotMemInfo();
  void slotSearch();
  void slotSearchClosed();
  void slotFind();

  /** etc. */
  void slotMsgActivated(KMMessage*);
  void quit();

  /** Operations on mailto: URLs. */
  void slotMailtoCompose();
  void slotMailtoReply();
  void slotMailtoForward();
  void slotMailtoAddAddrBook();

  /** Open URL in mUrlCurrent using Kfm. */
  void slotUrlOpen();

  /** Copy URL in mUrlCurrent to clipboard. Removes "mailto:" at 
      beginning of URL before copying. */
  void slotUrlCopy();

  // Move selected messages to folder with corresponding to given menuid
  virtual void moveSelectedToFolder( int menuId );
  // Copy selected messages to folder with corresponding to given menuid
  virtual void copySelectedToFolder( int menuId );
  // Update the "Move to" and "Copy to" popoutmenus in the Messages menu.
  virtual void updateMessageMenu();
  // Update html and threaded messages preferences in Folder menu.
  virtual void updateFolderMenu();
  
protected:
  KMenuBar     *mMenuBar;
  KToolBar     *mToolBar;
  KStatusBar   *mStatusBar;
  KMFolderTree *mFolderTree;
  KMReaderWin  *mMsgView;
  QSplitter    *mHorizPanner, *mVertPanner;
  KMHeaders    *mHeaders;
  KMFolder     *mFolder;
  QPopupMenu   *mFolderMenu, *mViewMenu, *mBodyPartsMenu;
  bool		mIntegrated;
  bool          mSendOnCheck;
  bool          mBeepOnNew, mBoxOnNew, mExecOnNew;
  QString       mNewMailCmd;
  int		mMessageStatusId;
  QValueList<int> *mHorizPannerSep, *mVertPannerSep;
  KURL          mUrlCurrent;
  QPopupMenu	*actMenu;
  QPopupMenu	*fileMenu;
  bool		mLongFolderList;
  bool		mStartupDone;
  KMMenuToFolder mMenuToFolder;
  int copyId, moveId, htmlId, threadId;
  bool mHtmlPref, mThreadPref, mFolderHtmlPref, mFolderThreadPref;
  QPopupMenu *messageMenu;
  KMLittleProgressDlg *littleProgress;
  KMFldSearch *searchWin;
  bool mUseKab;
  int mUseKabId;
  ConfigureDialog *mConfigureDialog;
};

#endif

