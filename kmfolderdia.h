/* Dialog for creating/modifying a folder or a directory name
 */
#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include <qcstring.h>

class QPushButton;
class QLineEdit;
class QListBox;
class KMFolder;

#define KMFolderDialogInherited KDialogBase
class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog(const QCString& path, const QCString& name,
		 QWidget *parent, const QString& caption);

  QCString folderName() const;

protected:
  QLineEdit *mNameEdit;
  QCString mPath;
};

#endif /*__KMFOLDERDIA*/

