/* kmfldsearch
 * (c) 1999 Stefan Taferner
 * This code is under GPL
 */
#ifndef kmfldsearch_h
#define kmfldsearch_h

#include <qdialog.h>

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

#define KMFldSearchInherited QDialog
class KMFldSearch: public QDialog
{
  Q_OBJECT
public:
  KMFldSearch(KMMainWin* parent, const char* name=NULL,
	       bool modal=FALSE, WFlags f=0);
  virtual ~KMFldSearch();

protected slots:
  virtual void slotClose();
  virtual void slotSearch();
  virtual void slotClear();
  virtual void slotShowMsg(QListViewItem *);

protected:
  void enableGUI();

  /** Create combo-box with list of folders */
  virtual QComboBox* createFolderCombo(const QString curFolder=0);

  /** Test if message matches. */
  virtual bool searchInMessage(KMMessage*);

  /** Search for matches in given folder. Adds matches to listbox mLbxMatches. */
  virtual void searchInFolder(KMFolder*,int);

  /** Search for matches in all folders. Calls searchInFolder() for every
      folder. */
  virtual void searchInAllFolders(void);

  /** Update status line widget. */
  virtual void updStatus(void);

protected:
  QGridLayout* mGrid;
  QComboBox *mCbxFolders;
  QPushButton *mBtnSearch, *mBtnClear, *mBtnClose;
  KMFldSearchRule **mRules;
  QListView* mLbxMatches;
  QLabel* mLblStatus;
  int mNumRules, mNumMatches;
  int count;
  QString mSearchFolder;
  bool mSearching, mStopped;
  KMMainWin* mMainWin;
};



//-----------------------------------------------------------------------------
class KMFldSearchRule
{
public:
  KMFldSearchRule(QWidget* parent, QGridLayout* grid, int gridRow, int gridCol);
  virtual ~KMFldSearchRule();

  /** Clear widget contents */
  virtual void clear(void);

  /** Test if message matches rules. */
  virtual bool matches(const KMMessage*) const;

  /** Prepare for search run. */
  virtual void prepare(void);

  virtual void setEnabled(bool);

  enum Func { Contains=0, NotContains, Equal, NotEqual, LessEqual, 
	      GreaterEqual };

protected:
  QComboBox *mCbxField, *mCbxFunc;
  QLineEdit *mEdtValue;
  QString mField, mValue;
  int mFieldIdx, mFunc;
};

#endif /*kmfldsearch_h*/
