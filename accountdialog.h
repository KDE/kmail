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

class AccountDialog : public KDialogBase
{
  Q_OBJECT
  
  public:
    AccountDialog( KMAccount *account, const QStringList &identity, 
		   QWidget *parent=0, const char *name=0, bool modal=true );
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
  
    struct PopWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QLineEdit    *loginEdit;
      QLineEdit    *passwordEdit;
      QLineEdit    *hostEdit;
      QLineEdit    *portEdit;
      QLineEdit    *precommand;
      QCheckBox    *useSSLCheck;
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
      QCheckBox    *hiddenFoldersCheck;
      QCheckBox    *storePasswordCheck;
      QRadioButton *authAuto;
      QRadioButton *authLogin;
      QRadioButton *authCramMd5;
      QRadioButton *authAnonymous;
    };

  private slots:
    virtual void slotOk();
    void slotLocationChooser();
    void slotEnablePopInterval( bool state );
    void slotEnableLocalInterval( bool state );
    void slotFontChanged();
    void slotSSLChanged();
    
  private:
    void makeLocalAccountPage();
    void makePopAccountPage();
    void makeImapAccountPage();
    void setupSettings();
    void saveSettings();

  private:
    LocalWidgets mLocal;
    PopWidgets   mPop;
    ImapWidgets  mImap;
    KMAccount    *mAccount;
    QStringList  mIdentityList;
};


#endif
