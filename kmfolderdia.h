// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * kmfolderdia.h
 *
 * Copyright (c) 1997-2004 KMail Developers
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include "mailinglist-magic.h"
using KMail::MailingList;

#include <kdialogbase.h>
#include "configuredialog_p.h"
#include <qvaluevector.h>

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
class KMFolderDialog;
class KMFolderTree;
template <typename T> class QGuardedPtr;

namespace KMail {

/**
 * "General" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDiaGeneralTab : public ConfigModuleTab
{
  Q_OBJECT

public:
  FolderDiaGeneralTab( KMFolderDialog* dlg, KMFolderTree* aParent,
                       const QString& aName,
                       QWidget* parent, const char* name = 0 );

  virtual void load();
  virtual void save();

private slots:
  void slotExpireFolder( bool );
  void slotReadExpiryUnitChanged( int );
  void slotUnreadExpiryUnitChanged( int );
  void slotChangeIcon( QString icon );
  /*
   * is called if the folder dropdown changes
   * then we update the other items to reflect the capabilities
   */
  void slotUpdateItems( int );
  void slotFolderNameChanged( const QString& );

private:
  void initializeWithValuesFromFolder( KMFolder* folder );

private:
  QComboBox *mBelongsToComboBox;
  QComboBox *mMailboxTypeComboBox;
  QComboBox *mShowSenderReceiverComboBox;
  QLineEdit *mNameEdit;
  QLabel      *mNormalIconLabel;
  KIconButton *mNormalIconButton;
  QLabel      *mUnreadIconLabel;
  KIconButton *mUnreadIconButton;
  QCheckBox   *mIconsCheckBox;
  QCheckBox   *mNewMailCheckBox;

  QCheckBox *mExpireFolderCheckBox;
  IdentityCombo *mIdentityComboBox;
  QGroupBox *mExpireGroupBox;
  QGroupBox *mMailboxTypeGroupBox;
//   QLineEdit *mailingListAdminAddress;

  KIntNumInput *mReadExpiryTimeNumInput, *mUnreadExpiryTimeNumInput;
  QComboBox    *mReadExpiryUnitsComboBox, *mUnreadExpiryUnitsComboBox;
  KMFolderDialog* mDlg;
};

/**
 * "Mailing List" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDiaMailingListTab : public ConfigModuleTab
{
  Q_OBJECT

public:
  FolderDiaMailingListTab( KMFolderDialog* dlg, QWidget* parent, const char* name = 0 );

  virtual void load();
  virtual void save();

private slots:
  /*
   * Detects mailing-list related stuff
   */
  void slotDetectMailingList();
  void slotInvokeHandler();
  void slotMLHandling( int element );
  void slotHoldsML( bool holdsML );
  void slotAddressChanged( int addr );

private:
  void fillMLFromWidgets();
  void fillEditBox();

  bool          mMLInfoChanged;
  QCheckBox    *mHoldsMailingList;
  QComboBox    *mMLHandlerCombo;
  QPushButton  *mDetectButton;
  QComboBox    *mAddressCombo;
  int           mLastItem;
  KEditListBox *mEditList;
  QLabel       *mMLId;
  MailingList   mMailingList;

  KMFolderDialog* mDlg;
};

} // end of namespace KMail

/**
 * Dialog for handling the properties of a mail folder
 */
class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog( KMFolder *folder, KMFolderDir *aFolderDir,
		  KMFolderTree* parent, const QString& caption,
                  const QString& name = QString::null );

  KMFolder* folder() const { return mFolder; }
  void setFolder( KMFolder* folder );
  KMFolderDir* folderDir() const { return mFolderDir; }
  typedef QValueList<QGuardedPtr<KMFolder> > FolderList;

  FolderList& folders() { return mFolders; }
  const FolderList& folders() const { return mFolders; }

protected slots:
  void slotChanged( bool );
  virtual void slotOk( void );

protected:
  QGuardedPtr<KMFolder> mFolder;
  QGuardedPtr<KMFolderDir> mFolderDir;
  FolderList mFolders;

  QValueVector<ConfigModuleTab*> mTabs;
};

#endif /*__KMFOLDERDIA*/

