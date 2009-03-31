#ifndef __KMAIL__MANAGESIEVESCRIPTSDIALOG_H__
#define __KMAIL__MANAGESIEVESCRIPTSDIALOG_H__

#include <kdialogbase.h>
#include <kurl.h>
#include <qmap.h>

class QListView;
class QCheckListItem;

namespace KMail {

class SieveJob;
class SieveEditor;

class ManageSieveScriptsDialog : public KDialogBase {
  Q_OBJECT
public:
  ManageSieveScriptsDialog( QWidget * parent=0, const char * name=0 );
  ~ManageSieveScriptsDialog();

private slots:
  void slotRefresh();
  void slotItem( KMail::SieveJob *, const QString &, bool );
  void slotResult( KMail::SieveJob *, bool, const QString &, bool );
  void slotContextMenuRequested( QListViewItem *, const QPoint & );
  void slotDoubleClicked( QListViewItem * );
  void slotSelectionChanged( QListViewItem * );
  void slotNewScript();
  void slotEditScript();
  void slotDesactivateScript();
  void slotDeleteScript();
  void slotGetResult( KMail::SieveJob *, bool, const QString &, bool );
  void slotPutResult( KMail::SieveJob *, bool );
  void slotSieveEditorOkClicked();
  void slotSieveEditorCancelClicked();

private:
  void killAllJobs();
  void changeActiveScript( QCheckListItem *, bool activate = true );

private:
  QListView * mListView;
  SieveEditor * mSieveEditor;
  QMap<KMail::SieveJob*,QCheckListItem*> mJobs;
  QMap<QCheckListItem*,KURL> mUrls;
  QMap<QCheckListItem*,QCheckListItem*> mSelectedItems;
  QCheckListItem * mContextMenuItem;
  KURL mCurrentURL;
  bool mWasActive : 1;
};

}

#endif /* __KMAIL__MANAGESIEVESCRIPTSDIALOG_H__ */

