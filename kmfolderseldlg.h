/* KMail Folder Selection Dialog
 * Pops up a small window with a list of folders and Ok/Cancel buttons.
 * Author: Stefan Taferner <taferner@kde.org>
 */
#ifndef kmfolderseldlg_h
#define kmfolderseldlg_h

#include <kdialogbase.h>
#include <qvaluelist.h>
#include <qguardedptr.h>

class QListBox;
class KMFolder;
class KMMainWidget;

class KMFolderSelDlg: public KDialogBase
{
  Q_OBJECT

public:
  /** Constructor. @p parent @em must be a @ref KMMainWin, because we
      need it's foldertree. */
  KMFolderSelDlg(KMMainWidget * parent, const QString& caption);
  virtual ~KMFolderSelDlg();

  /** Returns selected folder */
  virtual KMFolder* folder(void);

protected slots:
  void slotSelect(int);
  virtual void slotCancel();

protected:
  QListBox* mListBox;
  QValueList<QGuardedPtr<KMFolder> > mFolder;

  static QString oldSelection;
};

#endif /*kmfolderseldlg_h*/
