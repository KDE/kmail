/* Dialog for handling the properties of a mail folder
 */
#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include <qlist.h>

class KMAcctFolder;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QListBox;
class QComboBox ;
class KMFolder;

class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
		 QWidget *parent, const QString& caption);

protected slots:
  virtual void slotOk( void );
  virtual void slotHoldsML( bool );

protected:
  QComboBox *fileInFolder;
  QLineEdit *nameEdit;
  KMAcctFolder* folder;
  KMFolder *mFolder;
  KMFolderDir* mFolderDir;
  QValueList<QGuardedPtr<KMFolder> > mFolders;

  QCheckBox *holdsMailingList, *markAnyMessage;
  QLineEdit *mailingListPostAddress;
  QComboBox *identity;
//   QLineEdit *mailingListAdminAddress;

};

#endif /*__KMFOLDERDIA*/

