/* Dialog for handling the properties of a mail folder
 */
#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include <qlist.h>

class KMAcctFolder;
class QPushButton;
class QLineEdit;
class QListBox;
class QComboBox;
class KMFolder;

#define KMFolderDialogInherited KDialogBase

class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
		 QWidget *parent, const QString& caption);

protected slots:
  virtual void slotOk( void );

protected:
  QComboBox *fileInFolder;
  QLineEdit *nameEdit;
  KMAcctFolder* folder;
  KMFolderDir* mFolderDir;
  QList<KMFolder> mFolders;
};

#endif /*__KMFOLDERDIA*/

