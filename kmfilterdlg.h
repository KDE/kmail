/* Filter Dialog
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilterdlg_h
#define kmfilterdlg_h

#include "kmfilteraction.h"
#include "kmfilter.h"
#include <qcombobox.h>

class QBoxLayout;
class QListBox;
class QWidget;
class QPushButton;
class QLineEdit;
class QGridLayout;
class KMFaComboBox;
class KMFolderDir;

#define KMFilterDlgInherited KMGFilterDlg
class KMFilterDlg: public KMGFilterDlg
{
  Q_OBJECT

public:
  KMFilterDlg(QWidget* parent=NULL, const char* name=NULL);
  virtual ~KMFilterDlg();

  /** Remove all widgets from the filter-grid. */
  virtual void clear(void);

  /** Show details of given filter. */
  virtual void showFilter(KMFilter* filter);

  /** Translates the field for the filter rule back to english */
  virtual QString ruleFieldToEnglish(const QString & i18nVal);

  /** Apply changes in the edit fields to the current filter. */
  virtual void applyFilterChanges(void);

  /** Methods for filter options, @see KMGFilterDlg in kmfilteraction.h */
  virtual QPushButton* createDetailsButton(void);

  /* Creates a combo box of all folder labels with folders indented
     with white space according to their position in the folder
     hierarchy */
  virtual QComboBox* createFolderCombo( QStringList *,
					QValueList<QGuardedPtr<KMFolder> >*,
					QGuardedPtr<KMFolder> );

  /* Creates a combo box of strings */
  virtual QComboBox* createCombo( QStringList *str,
				  QString curItem );

  virtual QLineEdit* createEdit(const QString txt=0);

protected slots:
  void updateCurFilterName(const QString &text);
  void slotBtnUp();
  void slotBtnDown();
  void slotBtnNew();
  void slotBtnDelete();
  void slotBtnOk();
  void slotBtnCancel();
  void slotBtnHelp();
  void slotFilterSelected(int);

  void slotActionTypeSelected(KMFaComboBox*, int);

protected:
  // Fill listbox with filter list
  virtual void reloadFilterList(void);

  // Returns FALSE if grid was not created with setGrid() or given ptr is NULL
  virtual bool testOpts(const QWidget* ptr) const;

  // Returns index of filter rule field
  virtual int indexOfRuleField(const QString fieldName) const;

  // Initialize static lists. Automatically called.
  static void initLists(void);

  // disable/enabled relevant widgets
  void enableControls();

  // Update the position of the filter action field widgets
  void resizeEvent(QResizeEvent *qre);

  // Undo any changes that have been applied
  virtual void closeEvent( QCloseEvent * );
    
  KMFilter* mFilter;
  QGridLayout *mGrid;
  int mGridRow, mCbxHeight, mCbxWidth;
  int mActLineHeight;
  int mCurFilterIdx;
  QListBox *mFilterList;
  QWidget *mFilterArea;
  QPushButton *mBtnUp, *mBtnDown, *mBtnNew, *mBtnDelete, *mBtnOk, *mBtnCancel;
  QPushButton *mFaBtnDetails[FILTER_MAX_ACTIONS];
  QWidget     *mFaField[FILTER_MAX_ACTIONS];
  KMFaComboBox*mFaType[FILTER_MAX_ACTIONS];
  QComboBox   *mRuleFieldA, *mRuleFieldB;
  QLineEdit   *mRuleValueA, *mRuleValueB;
  QComboBox   *mRuleFuncA, *mRuleFuncB, *mRuleOp;

private:
  // ugly workaround against filter-up-down-move bug
  bool updown_move_semaphore;
};


//-----------------------------------------------------------------------------
#define KMFaComboBoxInherited QComboBox
class KMFaComboBox: public QComboBox
{
  Q_OBJECT
public:
  KMFaComboBox(QWidget* parent, const char* name=0);

signals:
  void selectType(KMFaComboBox* self, int index);

protected slots:
  void slotSelected(int);
};

#endif /*kmfilterdlg_h*/
