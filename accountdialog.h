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

#include <kdialog.h>
#include <k3listview.h>
#include <klineedit.h>
#include <QPointer>
//Added by qt3to4:
#include <QLabel>
#include <QList>
#include "imapaccountbase.h"

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

namespace KPIM {
class ServerTest;
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
#if 0
      QCheckBox    *resourceCheck;
      QPushButton  *resourceClearButton;
      QPushButton  *resourceClearPastButton;
#endif
      QCheckBox    *includeInCheck;
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
#if 0
      QCheckBox    *resourceCheck;
      QPushButton  *resourceClearButton;
      QPushButton  *resourceClearPastButton;
#endif
      QCheckBox    *includeInCheck;
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
#if 0
      QCheckBox    *resourceCheck;
      QPushButton  *resourceClearButton;
      QPushButton  *resourceClearPastButton;
#endif
      QCheckBox    *includeInCheck;
      QCheckBox    *intervalCheck;
      QCheckBox    *filterOnServerCheck;
      QLabel       *intervalLabel;
      KIntNumInput *intervalSpin;
      KIntNumInput *filterOnServerSizeSpin;
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
#if 0
      QCheckBox    *resourceCheck;
      QPushButton  *resourceClearButton;
      QPushButton  *resourceClearPastButton;
#endif
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
    void slotImapEncryptionChanged(int);
    void slotCheckPopCapabilities();
    void slotCheckImapCapabilities();
    void slotPopCapabilities( const QStringList &, const QStringList & );
    void slotImapCapabilities( const QStringList &, const QStringList & );
    void slotReloadNamespaces();
    void slotSetupNamespaces( const ImapAccountBase::nsDelimMap& map );
    void slotEditPersonalNamespace();
    void slotEditOtherUsersNamespace();
    void slotEditSharedNamespace();
    void slotConnectionResult( int errorCode, const QString& );
    void slotLeaveOnServerDaysChanged( int value );
    void slotLeaveOnServerCountChanged( int value );
    void slotFilterOnServerSizeChanged( int value );
#if 0
    // Moc doesn't understand #if 0, so they are also commented out
    // void slotClearResourceAllocations();
    // void slotClearPastResourceAllocations();
#endif

  private:
    void makeLocalAccountPage();
    void makeMaildirAccountPage();
    void makePopAccountPage();
    void makeImapAccountPage( bool disconnected = false );
    void setupSettings();
    void saveSettings();
    void checkHighest( QButtonGroup * );
    static unsigned int popCapabilitiesFromStringList( const QStringList & );
    static unsigned int imapCapabilitiesFromStringList( const QStringList & );
    void enablePopFeatures( unsigned int );
    void enableImapAuthMethods( unsigned int );
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
    KPIM::ServerTest *mServerTest;
    enum EncryptionMethods {
      NoEncryption = 0,
      SSL = 1,
      TLS = 2
    };
    enum Capabilities {
      Plain      =   1,
      Login      =   2,
      CRAM_MD5   =   4,
      Digest_MD5 =   8,
      Anonymous  =  16,
      APOP       =  32,
      Pipelining =  64,
      TOP        = 128,
      UIDL       = 256,
      STLS       = 512, // TLS for POP
      STARTTLS   = 512, // TLS for IMAP
      GSSAPI     = 1024,
      NTLM       = 2048,
      AllCapa    = 0xffffffff
    };
    unsigned int mCurCapa;
    unsigned int mCapaNormal;
    unsigned int mCapaSSL;
    unsigned int mCapaTLS;
    KMail::SieveConfigEditor *mSieveConfigEditor;
    QRegExpValidator *mValidator;
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
