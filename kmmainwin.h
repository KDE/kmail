/* kmail main window
 * Maintained by Stefan Taferner <taferner@kde.org>
 * This code is under the GPL
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include "kmtopwidget.h"
#include "qvaluelist.h"

class KMFolder;
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

#define KMMainWinInherited KMTopLevelWidget

class KMMainWin : public KMTopLevelWidget
{
  Q_OBJECT

public:
  KMMainWin(QWidget *parent = 0, char *name = 0);
  virtual ~KMMainWin();

  /** Read configuration options before widgets are created. */
  virtual void readPreConfig();

  /** Read configuration options after widgets are created. */
  virtual void readConfig();

  /** Write configuration options. */
  virtual void writeConfig();

  /** Insert a text field to the status bar and return ID of this field. */
  virtual int statusBarAddItem(const char* text);

  /** Change contents of a text field. */
  virtual void statusBarChangeItem(int id, const char* text);

  /** Easy access to main components of the window. */
  KMReaderWin* messageView() const { return mMsgView; }
  KToolBar* toolBar() const     { return mToolBar; }
  KStatusBar* statusBar() const   { return mStatusBar; }
  KMFolderTree* folderTree() const  { return mFolderTree; }

  /** Returns folder popup menu. */
  QPopupMenu* folderMenu();

public slots:
  virtual void show();
  virtual void hide();
  void slotCheckMail(); // sven moved here as public

  /** Output given message in the statusbar message field. */
  void statusMsg(const QString& text);

protected:
  void setupMenuBar();
  void setupToolBar();
  void setupStatusBar();
  void createWidgets();
  void activatePanners();

protected slots:
  void slotCheckOneAccount(int);
  void slotNewMail(KMAccount*);
  void getAccountMenu(); 
  void slotMenuActivated();
  void slotClose();
  void slotHelp();
  void slotNewMailReader();
  void slotSettings();
  void slotFilter();
  void slotAddrBook();
  void slotUnimplemented();
  void slotViewChange();
  void slotCreateFolder();
  void slotCreateDirectory();
  //void slotCheckMail(); sven - moved to public slots
  void slotCompose();
  void slotRename();
  void slotRemoveFolder();
  void slotEmptyFolder();
  void slotCompactFolder();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotForwardMsg();
  void slotEditMsg();
  void slotDeleteMsg();
  void slotSaveMsg();
  void slotPrintMsg();
  void slotMoveMsg();
  void slotCopyMsg();
  void slotResendMsg();
  void slotApplyFilters();
  void slotSetMsgStatus(int);
  void slotShowMsgSrc();
  void slotSetHeaderStyle(int);
  void slotSendQueued();
  void slotMsgPopup(const char* url, const QPoint&);
  void slotUrlClicked(const char* url, int button);
  void slotCopyText();
  void slotMarkAll();

  /** etc. */
  void folderSelected(KMFolder*);
  void slotMsgSelected(KMMessage*);
  void slotMsgActivated(KMMessage*);
  void quit();
  //void pannerHasChanged();
  //void resizeEvent(QResizeEvent*);
  //void initIntegrated();
  //void initSeparated();

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

protected:
  KMenuBar     *mMenuBar;
  KToolBar     *mToolBar;
  KStatusBar   *mStatusBar;
  KMFolderTree *mFolderTree;
  KMReaderWin  *mMsgView;
  QSplitter    *mHorizPanner, *mVertPanner;
  KMHeaders    *mHeaders;
  KMFolder     *mFolder;
  QPopupMenu   *mViewMenu, *mBodyPartsMenu;
  bool		mIntegrated;
  bool          mSendOnCheck;
  int		mMessageStatusId;
  QValueList<int> *mHorizPannerSep, *mVertPannerSep;
  QString       mUrlCurrent;
  QPopupMenu	*actMenu;
  QPopupMenu	*fileMenu;
  QPopupMenu	*mFolderMenu;
  bool		mLongFolderList;
  bool		mStartupDone;
};

#endif

