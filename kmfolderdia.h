#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include <qlist.h>
#include <knuminput.h>

class KMAcctFolder;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QListBox;
class QComboBox ;
class KMFolder;

/** Dialog for handling the properties of a mail folder
 */
class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
		 QWidget *parent, const QString& caption);

protected slots:
  virtual void slotOk( void );
  virtual void slotHoldsML( bool );
  virtual void slotExpireFolder( bool );

protected:
  QComboBox *fileInFolder;
  QComboBox *mailboxType;
  QLineEdit *nameEdit;
  KMAcctFolder* folder;
  KMFolder *mFolder;
  KMFolderDir* mFolderDir;
  QValueList<QGuardedPtr<KMFolder> > mFolders;

  QCheckBox *holdsMailingList, *markAnyMessage, *expireFolder;
  QLineEdit *mailingListPostAddress;
  QComboBox *identity;
//   QLineEdit *mailingListAdminAddress;

  KIntNumInput *readExpiryTime, *unreadExpiryTime;
  QComboBox    *readExpiryUnits, *unreadExpiryUnits;
};

#endif /*__KMFOLDERDIA*/

