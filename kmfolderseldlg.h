/* KMail Folder Selection Dialog
 * Pops up a small window with a list of folders and Ok/Cancel buttons.
 * Author: Stefan Taferner <taferner@kde.org>
 */
#ifndef kmfolderseldlg_h
#define kmfolderseldlg_h

#include <qdialog.h>

class QListBox;

#define KMFolderSelDlgInherited KMFolderSelDlg
class KMFolderSelDlg: public QDialog
{
  Q_OBJECT

public:
  KMFolderSelDlg(const char* caption);
  virtual ~KMFolderSelDlg();

  /** Returns selected folder */
  KMFolder* folder(void) const { return mFolder; }

protected slots:
  void slotOkPressed();
  void slotCancelPressed();

protected:
  KMFolder mFolder;
  QList* mListBox;
};

#endif /*kmfolderseldlg_h*/
