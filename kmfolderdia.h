// -*- mode: C++; c-file-style: "gnu" -*-
#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include "mailinglist-magic.h"
using KMail::MailingList;

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
class KEditListBox;
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
  void slotFolderNameChanged( const QString& );
  /*
   * Detects mailing-list related stuff
   */
  void slotDetectMailingList();
  void slotInvokeHandler();
  void slotMLHandling( int element );
  void slotHoldsML( bool holdsML );
  void slotAddressChanged( int addr );

private:
  void initializeWithValuesFromFolder( KMFolder* folder );
  void createGeneralTab( const QString& aName );
  void createMLTab();
  void fillMLFromWidgets();
  void fillEditBox();

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

  QCheckBox *mExpireFolderCheckBox;
  IdentityCombo *mIdentityComboBox;
  QGroupBox *mExpireGroupBox;
  QGroupBox *mMailboxTypeGroupBox;
//   QLineEdit *mailingListAdminAddress;

  KIntNumInput *mReadExpiryTimeNumInput, *mUnreadExpiryTimeNumInput;
  QComboBox    *mReadExpiryUnitsComboBox, *mUnreadExpiryUnitsComboBox;


  //Mailing-list tab Gui
  bool          mMLInfoChanged;
  QCheckBox    *mHoldsMailingList;
  QComboBox    *mMLHandlerCombo;
  QPushButton  *mDetectButton;
  QComboBox    *mAddressCombo;
  int           mLastItem;
  KEditListBox *mEditList;
  QLabel       *mMLId;
  MailingList   mMailingList;
};

#endif /*__KMFOLDERDIA*/

