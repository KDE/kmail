/* KMail Folder Selection Dialog
 * Pops up a small window with a list of folders and Ok/Cancel buttons.
 * Author: Stefan Taferner <taferner@kde.org>
 */
#ifndef kmfolderseldlg_h
#define kmfolderseldlg_h

#include <qdialog.h>

class QListBox;
class KMFolder;

#define KMFolderSelDlgInherited QDialog
class KMFolderSelDlg: public QDialog
{
  Q_OBJECT

public:
  KMFolderSelDlg(const char* caption);
  virtual ~KMFolderSelDlg();

  /** Returns selected folder */
  virtual KMFolder* folder(void);

protected slots:
  void slotSelect(int);
  void slotCancel();

protected:
  QListBox* mListBox;
};

#endif /*kmfolderseldlg_h*/
