/* kmail main window
 * Maintained by Stefan Taferner <taferner@kde.org>
 * This code is under the GPL
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include <ktopwidget.h>

class KMFolder;
class KMFolderTree;
class KMHeaders;
class KMReaderWin;
class KNewPanner;
class KMenuBar;
class KToolBar;
class KStatusBar;
class KMMessage;
class KMFolder;

#define KMMainWinInherited KTopLevelWidget
class KMMainWin : public KTopLevelWidget
{
  Q_OBJECT

public:
  KMMainWin(QWidget *parent = 0, char *name = 0);
  virtual ~KMMainWin();

  /** Read configuration options. */
  virtual void readConfig();

  /** Write configuration options. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Insert a text field to the status bar and return ID of this field. */
  virtual int statusBarAddItem(const char* text);

  /** Change contents of a text field. */
  virtual void statusBarChangeItem(int id, const char* text);

  /** Easy access to main components of the window. */
  KMReaderWin* messageView(void) const { return mMsgView; }
  KToolBar* toolBar(void) const     { return mToolBar; }
  KStatusBar* statusBar(void) const   { return mStatusBar; }
  KMFolderTree* folderTree(void) const  { return mFolderTree; }

public slots:
  /** Output given message in the statusbar message field. */
  void statusMsg(const char* text);

protected:
  virtual void closeEvent(QCloseEvent *);

  void setupMenuBar();
  void setupToolBar();
  void setupStatusBar();

protected slots:
  void slotClose();
  void slotHelp();
  void slotNewMailReader();
  void slotSettings();
  void slotFilter();
  void slotUnimplemented();
  void slotViewChange();
  void slotAddFolder();
  void slotCheckMail();
  void slotCompose();
  void slotModifyFolder();
  void slotRemoveFolder();
  void slotEmptyFolder();
  void slotCompactFolder();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotForwardMsg();
  void slotDeleteMsg();
  void slotSaveMsg();
  void slotPrintMsg();
  void slotMoveMsg();
  void slotShowMsgSrc();
  void slotSetHeaderStyle(int);
  void slotSendQueued();
  void slotMsgPopup(const QPoint&);
  void slotUrlClicked(const char* url, int button);

  void folderSelected(KMFolder*);
  void slotMsgSelected(KMMessage*);
  void slotMsgActivated(KMMessage*);
  //void pannerHasChanged();
  //void resizeEvent(QResizeEvent*);
  //void initIntegrated();
  //void initSeparated();

protected:
  KMenuBar     *mMenuBar;
  KToolBar     *mToolBar;
  KStatusBar   *mStatusBar;
  KMFolderTree *mFolderTree;
  KMReaderWin  *mMsgView;
  KNewPanner   *mHorizPanner, *mVertPanner;
  KMHeaders    *mHeaders;
  KMFolder     *mFolder;
  QPopupMenu   *mViewMenu, *mBodyPartsMenu;
  bool		mIntegrated;
  int		mMessageStatusId;
  int		mHorizPannerSep, mVertPannerSep;
};

#endif

