#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include <qptrlist.h>

class KMAcctFolder;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QListBox;
class QComboBox;
class QGroupBox;
class KMFolder;
class KMFolderTreeItem;
class KMFolderDir;
class KIntNumInput;
class KIconButton;
class IdentityCombo;
template <typename T> class QGuardedPtr;

/** Dialog for handling the properties of a mail folder
 */
class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog(KMFolder *folder, KMFolderDir *aFolderDir,
		 QWidget *parent, const QString& caption, const QString& name = "");

protected slots:
  virtual void slotOk( void );
  virtual void slotExpireFolder( bool );
  void slotReadExpiryUnitChanged( int );
  void slotUnreadExpiryUnitChanged( int );
  void slotChangeIcon( QString icon );
  /*
   * is called if the folder dropdown changes
   * then we update the other items to reflect the capabilities
   */
  void slotUpdateItems( int );

protected:
  QComboBox *fileInFolder;
  QComboBox *mailboxType, *senderType;
  QLineEdit *nameEdit;
  QGuardedPtr<KMAcctFolder> folder;
  QGuardedPtr<KMFolder> mFolder;
  QGuardedPtr<KMFolderDir> mFolderDir;
  QGuardedPtr<KMFolderTreeItem> mFolderItem;
  KIconButton *mNormalIconButton;
  KIconButton *mUnreadIconButton;
  QCheckBox   *mIconsCheckBox;
  QCheckBox   *mNewMailCheckBox;

  QValueList<QGuardedPtr<KMFolder> > mFolders;

  QCheckBox *holdsMailingList, *markAnyMessage, *expireFolder;
  QLineEdit *mailingListPostAddress;
  IdentityCombo *identity;
  QGroupBox *expGroup, *mtGroup;
//   QLineEdit *mailingListAdminAddress;

  KIntNumInput *readExpiryTime, *unreadExpiryTime;
  QComboBox    *readExpiryUnits, *unreadExpiryUnits;
};

#endif /*__KMFOLDERDIA*/

