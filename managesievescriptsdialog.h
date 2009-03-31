#ifndef __KMAIL__MANAGESIEVESCRIPTSDIALOG_H__
#define __KMAIL__MANAGESIEVESCRIPTSDIALOG_H__

#include <kdialog.h>
#include <kurl.h>
#include <QMap>

class Q3ListView;
class Q3ListViewItem;
class Q3CheckListItem;

namespace KMail {

class SieveJob;
class SieveEditor;

class ManageSieveScriptsDialog : public KDialog {
  Q_OBJECT
public:
  explicit ManageSieveScriptsDialog( QWidget * parent=0, const char * name=0 );
  ~ManageSieveScriptsDialog();

private slots:
  void slotRefresh();
  void slotItem( KMail::SieveJob *, const QString &, bool );
  void slotResult( KMail::SieveJob *, bool, const QString &, bool );
  void slotContextMenuRequested( Q3ListViewItem *, const QPoint & );
  void slotDoubleClicked( Q3ListViewItem * );
  void slotSelectionChanged( Q3ListViewItem * );
  void slotNewScript();
  void slotEditScript();
  void slotDeleteScript();
  void slotDeactivateScript();
  void slotGetResult( KMail::SieveJob *, bool, const QString &, bool );
  void slotPutResult( KMail::SieveJob *, bool );
  void slotSieveEditorOkClicked();
  void slotSieveEditorCancelClicked();

private:
  void killAllJobs();
  void changeActiveScript( Q3CheckListItem*, bool activate = true );

private:
  Q3ListView * mListView;
  SieveEditor * mSieveEditor;
  QMap<KMail::SieveJob*,Q3CheckListItem*> mJobs;
  QMap<Q3CheckListItem*,KUrl> mUrls;
  QMap<Q3CheckListItem*,Q3CheckListItem*> mSelectedItems;
  Q3CheckListItem * mContextMenuItem;
  KUrl mCurrentURL;
  bool mWasActive : 1;
};

}

#endif /* __KMAIL__MANAGESIEVESCRIPTSDIALOG_H__ */

