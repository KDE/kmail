/* kmfldsearch
 * (c) 1999 Stefan Taferner
 * This code is under GPL
 */
#ifndef kmfldsearch_h
#define kmfldsearch_h

#include <qdialog.h>
#include <qguardedptr.h>

class QLineEdit;
class QPushButton;
class QComboBox;
class QGridLayout;
class KMFldSearchRule;
class QListView;
class KMFolder;
class KMMessage;
class QLabel;
class KMMainWin;
class KMFolderMgr;
class QListViewItem;
class KMFolderTreeItem;

#define KMFldSearchInherited QDialog
class KMFldSearch: public QDialog
{
  Q_OBJECT
public:
  KMFldSearch(KMMainWin* parent, const char* name=NULL,
	       QString currentfolder = "", bool modal=FALSE, WFlags f=0);
  virtual ~KMFldSearch();

protected slots:
  virtual void slotClose();
  virtual void slotSearch();
  virtual void slotStop();
  virtual void slotShowMsg(QListViewItem *);
  virtual void slotFolderActivated(int nr);
  virtual void slotFolderComplete(KMFolderTreeItem *fti, bool success);

protected:
  void enableGUI();

  /** Create combo-box with list of folders */
  virtual QComboBox* createFolderCombo(const QString curFolder=0);

  /** Test if message matches. */
  virtual bool searchInMessage(KMMessage*, const QCString&);

  /** Search for matches in given folder. Adds matches to listbox mLbxMatches. */
  virtual void searchInFolder(QGuardedPtr<KMFolder>, int);

  /** Search for matches in all folders. Calls searchInFolder() for every
      folder. */
  virtual void searchInAllFolders(void);

  /** Update status line widget. */
  virtual void updStatus(void);

  /** GUI cleanup after search */
  virtual void searchDone();

  /** Reimplemented to react to Escape. */
  virtual void keyPressEvent(QKeyEvent*);

  /** Reimplemented to stop searching when the window is closed */
  virtual void closeEvent(QCloseEvent*);

protected:
  QGridLayout* mGrid;
  QComboBox *mCbxFolders;
  QPushButton *mBtnSearch, *mBtnStop, *mBtnClose;
  KMFldSearchRule **mRules;
  QListView* mLbxMatches;
  QLabel* mLblStatus;
  int mNumRules, mNumMatches;
  int mCount;
  QString mSearchFolder;
  bool mSearching, mStopped, mCloseRequested;
  KMMainWin* mMainWin;
  QWidget* mLastFocus;
  QStringList mFolderNames;
  QValueList<QGuardedPtr<KMFolder> > mFolders;
};



//-----------------------------------------------------------------------------
class KMFldSearchRule
{
public:
  KMFldSearchRule(QWidget* parent, QGridLayout* grid, int gridRow, int gridCol);
  virtual ~KMFldSearchRule();

  /** Test if message matches rules. */
  virtual bool matches(const KMMessage*, const QCString&) const;

  /** Prepare for search run. */
  virtual void prepare(void);

  /** Enable or disable all the push buttons */
  virtual void setEnabled(bool);

  /** The header field to search in (or whole message) */
  virtual bool isHeaderField() const;

  /** Update the functions according to the searching capabilites in the
   * selected folder */
  virtual void updateFunctions(QComboBox *cbx,
    const QValueList<QGuardedPtr<KMFolder> > &folders);

  /* Fill in the header fiels where to search */
  virtual void insertFieldItems(bool all);

  enum Func { Contains=0, NotContains, Equal, NotEqual, 
              MatchesRegExp, NotMatchesRegExp };

protected:
  QComboBox *mCbxField, *mCbxFunc;
  QLineEdit *mEdtValue;
  QString mField, mValue;
  QCString mHeaderField;
  int mFieldLength;
  bool mNonLatin;
  int mFieldIdx, mFunc;
};

#endif /*kmfldsearch_h*/
