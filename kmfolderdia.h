#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include <qlist.h>

class KMAcctFolder;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QListBox;
class QComboBox;
class QGroupBox;
class KMFolder;
class KMFolderDir;
class KIntNumInput;
template <typename T> class QGuardedPtr;

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

  /*
   * is called if the folder dropdown changes
   * then we update the other items to reflect the capabilities
   */
  void slotUpdateItems( int );

protected:
  QComboBox *fileInFolder;
  QComboBox *mailboxType, *senderType;
  QLineEdit *nameEdit;
  KMAcctFolder* folder;
  KMFolder *mFolder;
  KMFolderDir* mFolderDir;
  QValueList<QGuardedPtr<KMFolder> > mFolders;

  QCheckBox *holdsMailingList, *markAnyMessage, *expireFolder;
  QLineEdit *mailingListPostAddress;
  QComboBox *identity;
  QGroupBox *expGroup, *mtGroup;
//   QLineEdit *mailingListAdminAddress;

  KIntNumInput *readExpiryTime, *unreadExpiryTime;
  QComboBox    *readExpiryUnits, *unreadExpiryUnits;
};

#endif /*__KMFOLDERDIA*/

