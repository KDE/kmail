/*   -*- c++ -*-
 *   accountdialog.h
 *
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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _ACCOUNT_DIALOG_H_
#define _ACCOUNT_DIALOG_H_

#include <config-kmail.h>
#include "imapaccountbase.h"

#include <kdialog.h>
#include <klineedit.h>

#include <QPointer>
#include <QLabel>
#include <QList>

class QRegExpValidator;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QRadioButton;
class QToolButton;
class KIntNumInput;
class KMAccount;
class KMFolder;
class QButtonGroup;
class QGroupBox;

namespace MailTransport {
class ServerTest;
}

namespace KPIMIdentities {
class IdentityCombo;
}

namespace KMail {

class SieveConfigEditor;
class FolderRequester;

class AccountDialog : public KDialog
{
  Q_OBJECT

  public:
    AccountDialog( const QString & caption, KMAccount *account,
                   QWidget *parent=0 );
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
      QCheckBox    *includeInCheck;
      QCheckBox    *intervalCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      QComboBox    *folderCombo;
    };

    struct MaildirWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QComboBox    *locationEdit;
      QLineEdit    *precommand;
      QCheckBox    *includeInCheck;
      QCheckBox    *intervalCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      QComboBox    *folderCombo;
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
      QGroupBox    *encryptionGroup;
      QButtonGroup *encryptionButtonGroup;
      QRadioButton *encryptionNone;
      QRadioButton *encryptionSSL;
      QRadioButton *encryptionTLS;
      QGroupBox    *authGroup;
      QButtonGroup *authButtonGroup;
      QRadioButton *authUser;
      QRadioButton *authPlain;
      QRadioButton *authLogin;
      QRadioButton *authCRAM_MD5;
      QRadioButton *authDigestMd5;
      QRadioButton *authNTLM;
      QRadioButton *authGSSAPI;
      QRadioButton *authAPOP;

      QPushButton  *checkCapabilities;
      QCheckBox    *usePipeliningCheck;
      QCheckBox    *storePasswordCheck;
      QCheckBox    *leaveOnServerCheck;
      QCheckBox    *leaveOnServerDaysCheck;
      KIntNumInput *leaveOnServerDaysSpin;
      QCheckBox    *leaveOnServerCountCheck;
      KIntNumInput *leaveOnServerCountSpin;
      QCheckBox    *leaveOnServerSizeCheck;
      KIntNumInput *leaveOnServerSizeSpin;
      QCheckBox    *includeInCheck;
      QCheckBox    *intervalCheck;
      QCheckBox    *filterOnServerCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      KIntNumInput *filterOnServerSizeSpin;
      QComboBox    *folderCombo;
    };

    struct ImapWidgets
    {
      QLabel       *titleLabel;
      QLineEdit    *nameEdit;
      QLineEdit    *loginEdit;
      QLineEdit    *passwordEdit;
      QLineEdit    *hostEdit;
      QLineEdit    *portEdit;
      QCheckBox    *autoExpungeCheck;     // only used by normal (online) IMAP
      QCheckBox    *hiddenFoldersCheck;
      QCheckBox    *subscribedFoldersCheck;
      QCheckBox    *locallySubscribedFoldersCheck;
      QCheckBox    *loadOnDemandCheck;
      QCheckBox    *storePasswordCheck;
      QCheckBox    *progressDialogCheck;  // only used by Disconnected IMAP
      QCheckBox    *includeInCheck;
      QCheckBox    *intervalCheck;
      QCheckBox    *listOnlyOpenCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      QGroupBox    *encryptionGroup;
      QButtonGroup *encryptionButtonGroup;
      QRadioButton *encryptionNone;
      QRadioButton *encryptionSSL;
      QRadioButton *encryptionTLS;
      QGroupBox    *authGroup;
      QButtonGroup *authButtonGroup;
      QRadioButton *authUser;
      QRadioButton *authPlain;
      QRadioButton *authLogin;
      QRadioButton *authCramMd5;
      QRadioButton *authDigestMd5;
      QRadioButton *authGSSAPI;
      QRadioButton *authNTLM;
      QRadioButton *authAnonymous;
      QPushButton  *checkCapabilities;
      FolderRequester *trashCombo;
      KLineEdit    *personalNS;
      KLineEdit    *otherUsersNS;
      KLineEdit    *sharedNS;
      QToolButton  *editPNS;
      QToolButton  *editONS;
      QToolButton  *editSNS;
      ImapAccountBase::nsDelimMap nsMap;
      KPIMIdentities::IdentityCombo    *identityCombo;
      QCheckBox    *useDefaultIdentityCheck;
      QLabel       *identityLabel;
    };

  private slots:
    virtual void slotOk();
    void slotLocationChooser();
    void slotMaildirChooser();
    void slotEnablePopInterval( bool state );
    void slotEnableImapInterval( bool state );
    void slotEnableLocalInterval( bool state );
    void slotEnableMaildirInterval( bool state );
    void slotFontChanged();
    void slotLeaveOnServerClicked();
    void slotEnableLeaveOnServerDays( bool state );
    void slotEnableLeaveOnServerCount( bool state );
    void slotEnableLeaveOnServerSize( bool state );
    void slotFilterOnServerClicked();
    void slotPipeliningClicked();
    void slotPopEncryptionChanged(int);
    void slotPopPasswordChanged(const QString& text);
    void slotImapEncryptionChanged(int);
    void slotCheckPopCapabilities();
    void slotCheckImapCapabilities();
    void slotPopCapabilities( QList<int> );
    void slotImapCapabilities( QList<int> );
    void slotReloadNamespaces();
    void slotSetupNamespaces( const ImapAccountBase::nsDelimMap& map );
    void slotEditPersonalNamespace();
    void slotEditOtherUsersNamespace();
    void slotEditSharedNamespace();
    void slotConnectionResult( int errorCode, const QString& );
    void slotLeaveOnServerDaysChanged( int value );
    void slotLeaveOnServerCountChanged( int value );
    void slotFilterOnServerSizeChanged( int value );
    void slotIdentityCheckboxChanged();

  private:
    void makeLocalAccountPage();
    void makeMaildirAccountPage();
    void makePopAccountPage();
    void makeImapAccountPage( bool disconnected = false );
    void setupSettings();
    void saveSettings();
    void checkHighest( QButtonGroup * );
    void enablePopFeatures();
    void enableImapAuthMethods();
    void initAccountForConnect();
    const QString namespaceListToString( const QStringList& list );

  private:
    LocalWidgets mLocal;
    MaildirWidgets mMaildir;
    PopWidgets   mPop;
    ImapWidgets  mImap;
    KMAccount    *mAccount;
    QList<QPointer<KMFolder> > mFolderList;
    QStringList  mFolderNames;
    MailTransport::ServerTest *mServerTest;
    KMail::SieveConfigEditor *mSieveConfigEditor;
    QRegExpValidator *mValidator;
    bool mServerTestFailed;
};

class NamespaceLineEdit: public KLineEdit
{
  Q_OBJECT

  public:
    NamespaceLineEdit( QWidget* parent );

    const QString& lastText() { return mLastText; }

  public slots:
    virtual void setText ( const QString & );

  private:
    QString mLastText;
};

class NamespaceEditDialog: public KDialog
{
  Q_OBJECT

  public:
    NamespaceEditDialog( QWidget* parent, ImapAccountBase::imapNamespace type,
        ImapAccountBase::nsDelimMap* map );

  protected slots:
    void slotOk();
    void slotRemoveEntry( int );

  private:
    ImapAccountBase::imapNamespace mType;
    ImapAccountBase::nsDelimMap* mNamespaceMap;
    ImapAccountBase::namespaceDelim mDelimMap;
    QMap<int, NamespaceLineEdit*> mLineEditMap;
    QButtonGroup* mBg;
};

} // namespace KMail

#endif
