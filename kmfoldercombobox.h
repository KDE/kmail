/* kmail folder-list combo-box */

#ifndef __KMFOLDERCOMBOBOX
#define __KMFOLDERCOMBOBOX

#include <qcombobox.h>
#include <qguardedptr.h>

#include "kmfolder.h"

class KMFolderComboBox : public QComboBox
{
  Q_OBJECT
  
public:
  KMFolderComboBox( QWidget *parent = 0, char *name = 0 );
  KMFolderComboBox( bool rw, QWidget *parent = 0, char *name = 0 );
  
  void setFolder( KMFolder *aFolder );
  KMFolder *getFolder();
  
public slots:
  /** Refresh list of folders in the combobox. */
  void refreshFolders();
  
private slots:
  void slotActivated(int index);
  
private:
  QGuardedPtr<KMFolder> mFolder;
};

#endif /* __KMFOLDERCOMBOBOX */
