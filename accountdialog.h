/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _ACCOUNT_DIALOG_H_
#define _ACCOUNT_DIALOG_H_

#include <kdialogbase.h>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class KIntNumInput;
class KMAccount;
class KMFolder;
class KMServerTest;

class AccountDialog : public KDialogBase
{
  Q_OBJECT
  
  public:
    AccountDialog( KMAccount *account, const QStringList &identity, 
		   QWidget *parent=0, const char *name=0, bool modal=true );
    virtual ~AccountDialog();
  private:
    struct LocalWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QComboBox    *locationEdit;
      QRadioButton *lockMutt;
      QRadioButton *lockMuttPriv;
      QRadioButton *lockProcmail;
      QComboBox    *procmailLockFileName;
      QRadioButton *lockFcntl;
      QRadioButton *lockNone;
      QLineEdit    *precommand;
      QCheckBox    *excludeCheck;
      QCheckBox    *intervalCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      QComboBox    *folderCombo;
      QComboBox    *identityCombo;
    };

    struct MaildirWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QComboBox    *locationEdit;
      QLineEdit    *precommand;
      QCheckBox    *excludeCheck;
      QCheckBox    *intervalCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      QComboBox    *folderCombo;
      QComboBox    *identityCombo;
    };

    struct PopWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QLineEdit    *loginEdit;
      QLineEdit    *passwordEdit;
      QLineEdit    *hostEdit;
      QLineEdit    *portEdit;
      QLineEdit    *precommand;
      QButtonGroup *encryptionGroup;
      QRadioButton *encryptionNone;
      QRadioButton *encryptionSSL;
      QRadioButton *encryptionTLS;
      QButtonGroup *authGroup;
      QRadioButton *authUser;
      QRadioButton *authPlain;
      QRadioButton *authLogin;
      QRadioButton *authCRAM_MD5;
      QRadioButton *authAPOP;
      QPushButton  *checkCapabilities;
      QCheckBox    *usePipeliningCheck;
      QCheckBox    *storePasswordCheck;
      QCheckBox    *deleteMailCheck;
      QCheckBox    *retriveAllCheck;
      QCheckBox    *excludeCheck;
      QCheckBox    *intervalCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      QComboBox    *folderCombo;
      QComboBox    *identityCombo;
    };

    struct ImapWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QLineEdit    *loginEdit;
      QLineEdit    *passwordEdit;
      QLineEdit    *hostEdit;
      QLineEdit    *portEdit;
      QLineEdit    *prefixEdit;
      QCheckBox    *autoExpungeCheck;
      QCheckBox    *hiddenFoldersCheck;
      QCheckBox    *storePasswordCheck;
      QButtonGroup *encryptionGroup;
      QRadioButton *encryptionNone;
      QRadioButton *encryptionSSL;
      QRadioButton *encryptionTLS;
      QButtonGroup *authGroup;
      QRadioButton *authUser;
      QRadioButton *authPlain;
      QRadioButton *authLogin;
      QRadioButton *authCramMd5;
      QRadioButton *authAnonymous;
      QPushButton  *checkCapabilities;
    };

  private slots:
    virtual void slotOk();
    void slotLocationChooser();
    void slotMaildirChooser();
    void slotEnablePopInterval( bool state );
    void slotEnableLocalInterval( bool state );
    void slotEnableMaildirInterval( bool state );
    void slotFontChanged();
    void slotPopEncryptionChanged(int);
    void slotImapEncryptionChanged(int);
    void slotCheckPopCapabilities();
    void slotCheckImapCapabilities();
    void slotPopCapabilities(const QStringList &);
    void slotImapCapabilities(const QStringList &);
    
  private:
    void makeLocalAccountPage();
    void makeMaildirAccountPage();
    void makePopAccountPage();
    void makeImapAccountPage();
    void setupSettings();
    void saveSettings();
    void checkHighest(QButtonGroup *);

  private:
    LocalWidgets mLocal;
    MaildirWidgets mMaildir;
    PopWidgets   mPop;
    ImapWidgets  mImap;
    KMAccount    *mAccount;
    QStringList  mIdentityList;
    QValueList<QGuardedPtr<KMFolder> > mFolderList;
    QStringList  mFolderNames;
    KMServerTest *mServerTest;
};


#endif
