#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>

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
  KMFolderDialog( KMFolder *folder, KMFolderDir *aFolderDir,
		  QWidget *parent, const QString& caption,
                  const QString& name = QString::null );

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

private:
  void initializeWithValuesFromFolder( KMFolder* folder );

protected:
  QComboBox *mBelongsToComboBox;
  QComboBox *mMailboxTypeComboBox;
  QComboBox *mShowSenderReceiverComboBox;
  QLineEdit *mNameEdit;
  QGuardedPtr<KMFolder> mFolder;
  QGuardedPtr<KMFolderDir> mFolderDir;
  QLabel      *mNormalIconLabel;
  KIconButton *mNormalIconButton;
  QLabel      *mUnreadIconLabel;
  KIconButton *mUnreadIconButton;
  QCheckBox   *mIconsCheckBox;
  QCheckBox   *mNewMailCheckBox;

  QValueList<QGuardedPtr<KMFolder> > mFolders;

  QCheckBox *mHoldsMailingListCheckBox;
  QCheckBox *mExpireFolderCheckBox;
  QLineEdit *mMailingListPostAddressEdit;
  IdentityCombo *mIdentityComboBox;
  QGroupBox *mExpireGroupBox;
  QGroupBox *mMailboxTypeGroupBox;
//   QLineEdit *mailingListAdminAddress;

  KIntNumInput *mReadExpiryTimeNumInput, *mUnreadExpiryTimeNumInput;
  QComboBox    *mReadExpiryUnitsComboBox, *mUnreadExpiryUnitsComboBox;
};

#endif /*__KMFOLDERDIA*/

