/* kmfldsearch
 * (c) 1999 Stefan Taferner, (c) 2001 Aaron J. Seigo
 * This code is under GPL
 */
#ifndef kmfldsearch_h
#define kmfldsearch_h

#include <qvaluelist.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qguardedptr.h>

#include <kdialogbase.h>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListView;
class QListViewItem;
class QPushButton;
class QRadioButton;

class KMFldSearchRule;
class KMFolder;
class KMFolderComboBox;
class KMFolderImap;
class KMFolderMgr;
class KMMainWin;
class KMMessage;
class KStatusBar;

typedef QPtrList<KMFldSearchRule> KMFldSearchRuleList;
typedef QPtrListIterator<KMFldSearchRule> KMFldSearchRuleIt;

class KMFldSearch: public KDialogBase
{
  Q_OBJECT

public:
  KMFldSearch(KMMainWin* parent, const char* name=NULL,
              KMFolder *curFolder=NULL, bool modal=FALSE);
  virtual ~KMFldSearch();

  void activateFolder(KMFolder* curFolder);

protected slots:
  virtual void slotClose();
  virtual void slotSearch();
  virtual void slotStop();
  virtual bool slotShowMsg(QListViewItem *);
  virtual void slotFolderActivated(int nr);
  virtual void slotFolderComplete(KMFolderImap *folder, bool success);

protected:
  void enableGUI();

  /** Test if message matches. */
  virtual bool searchInMessage(KMMessage*, const QCString&);

  /** Search for matches in given folder. Adds matches to listbox mLbxMatches. */
  virtual void searchInFolder(QGuardedPtr<KMFolder>, bool recursive = true,
    bool fetchHeaders = true);

  /** Search for matches in all folders. Calls searchInFolder() for every
      folder. */
  virtual void searchInAllFolders(void);

  /** Update status line widget. */
  virtual void updStatus(void);

  /** GUI cleanup after search */
  virtual void searchDone();

  /** Return the KMMessage corresponding to the selected listviewitem */
  KMMessage* getSelectedMsg();

  /** Reimplemented to react to Escape. */
  virtual void keyPressEvent(QKeyEvent*);

  /** Reimplemented to stop searching when the window is closed */
  virtual void closeEvent(QCloseEvent*);

protected:
  bool mSearching;
  bool mStopped;
  bool mCloseRequested;
  int mFetchingInProgress;
  int mNumMatches;
  int mCount;
  QString mSearchFolder;
  KMFldSearchRuleList mRules;

  // GC'd by Qt
  QGridLayout* mGrid;
  QRadioButton *mChkbxAllFolders;
  QRadioButton *mChkbxSpecificFolders;
  KMFolderComboBox *mCbxFolders;
  QPushButton *mBtnSearch;
  QPushButton *mBtnStop;
  QCheckBox *mChkSubFolders;
  QListView* mLbxMatches;	
  KStatusBar* mStatusBar;
  QWidget* mLastFocus; // to remember the position of the focus

  // not owned by us
  KMMainWin* mMainWin;
  
  static const int MSGID_COLUMN;
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

  /** Make this rule gain the focus **/
  virtual void setFocus();

  /** The header field to search in (or whole message) */
  virtual bool isHeaderField() const;

  /** Update the functions according to the searching capabilites in
   * the selected folder. Disables editing if the folder doesn't
   * support arbitrary headers */
  virtual void updateFunctions(KMFolder* folder);

  /** Fill in the header fields where to search. If @p imap == @p
      true, shows only the IMAP-searchable fields */
  virtual void insertFieldItems(bool imap);

  enum Func { Contains=0, NotContains, Equal, NotEqual,
              MatchesRegExp, NotMatchesRegExp };

protected:
  QComboBox *mCbxField, *mCbxFunc;
  QLineEdit *mEdtValue;
  QString mField, mValue;
  QCString mHeaderField;
  int mFieldLength;
  bool mNonLatin;
  int mFieldIdx, mFunc, mRow;
};

#endif /*kmfldsearch_h*/
