/*  -*- mode: C++; c-file-style: "gnu" -*-
    identitydialog.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "identitydialog.h"

// other KMail headers:
#include "xfaceconfigurator.h"
#include "folderrequester.h"
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
using KMail::FolderRequester;
#include "kmfoldermgr.h"
#include "dictionarycombobox.h"
#include "kleo_util.h"
#include "kmmainwidget.h"
#include "kmfolder.h"
#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"
#include "util.h"
// other kdepim headers:
#include <kpimidentities/identity.h>
#include <kpimidentities/signatureconfigurator.h>

#include <libkdepim/addresseelineedit.h>
// libkleopatra:
#include "libkleo/ui/keyrequester.h"
#include "kleo/cryptobackendfactory.h"

#include <kpimutils/email.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transportcombobox.h>
using MailTransport::TransportManager;

// other KDE headers:
#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kfileitem.h>
#include <kurl.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kconfiggroup.h>

// Qt headers:
#include <QTabWidget>
#include <QLabel>

#include <QLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>

// other headers:
#include <gpgme++/key.h>
#include <iterator>
#include <algorithm>

using namespace KPIM;
using namespace MailTransport;

namespace KMail {

  IdentityDialog::IdentityDialog( QWidget * parent )
    : KDialog( parent )
  {
    setCaption( i18n("Edit Identity") );
    setButtons( Ok|Cancel|Help );
    setDefaultButton( Ok );

    // tmp. vars:
    QWidget * tab;
    QLabel  * label;
    int row;
    QGridLayout * glay;
    QString msg;

    //
    // Tab Widget: General
    //
    row = -1;
    QWidget *page = new QWidget( this );
    setMainWidget( page );
    QVBoxLayout * vlay = new QVBoxLayout( page );
    vlay->setSpacing( spacingHint() );
    vlay->setMargin( 0 );
    QTabWidget *tabWidget = new QTabWidget( page );
    tabWidget->setObjectName( "config-identity-tab" );
    vlay->addWidget( tabWidget );

    tab = new QWidget( tabWidget );
    tabWidget->addTab( tab, i18n("&General") );
    glay = new QGridLayout( tab );
    glay->setSpacing( spacingHint() );
    glay->setMargin( marginHint() );
    glay->setRowStretch( 3, 1 );
    glay->setColumnStretch( 1, 1 );

    // "Name" line edit and label:
    ++row;
    mNameEdit = new KLineEdit( tab );
    glay->addWidget( mNameEdit, row, 1 );
    label = new QLabel( i18n("&Your name:"), tab );
    label->setBuddy( mNameEdit );
    glay->addWidget( label, row, 0 );
    msg = i18n("<qt><h3>Your name</h3>"
               "<p>This field should contain your name as you would like "
               "it to appear in the email header that is sent out;</p>"
               "<p>if you leave this blank your real name will not "
               "appear, only the email address.</p></qt>");
    label->setWhatsThis( msg );
    mNameEdit->setWhatsThis( msg );

    // "Organization" line edit and label:
    ++row;
    mOrganizationEdit = new KLineEdit( tab );
    glay->addWidget( mOrganizationEdit, row, 1 );
    label =  new QLabel( i18n("Organi&zation:"), tab );
    label->setBuddy( mOrganizationEdit );
    glay->addWidget( label, row, 0 );
    msg = i18n("<qt><h3>Organization</h3>"
               "<p>This field should have the name of your organization "
               "if you would like it to be shown in the email header that "
               "is sent out.</p>"
               "<p>It is safe (and normal) to leave this blank.</p></qt>");
    label->setWhatsThis( msg );
    mOrganizationEdit->setWhatsThis( msg );

    // "Email Address" line edit and label:
    // (row 3: spacer)
    ++row;
    mEmailEdit = new KLineEdit( tab );
    glay->addWidget( mEmailEdit, row, 1 );
    label = new QLabel( i18n("&Email address:"), tab );
    label->setBuddy( mEmailEdit );
    glay->addWidget( label, row, 0 );
    msg = i18n("<qt><h3>Email address</h3>"
               "<p>This field should have your full email address.</p>"
               "<p>If you leave this blank, or get it wrong, people "
               "will have trouble replying to you.</p></qt>");
    label->setWhatsThis( msg );
    mEmailEdit->setWhatsThis( msg );

    //
    // Tab Widget: Cryptography
    //
    row = -1;
    mCryptographyTab = tab = new QWidget( tabWidget );
    tabWidget->addTab( tab, i18n("Cryptograph&y") );
    glay = new QGridLayout( tab );
    glay->setSpacing( spacingHint() );
    glay->setMargin( marginHint() );
    glay->setColumnStretch( 1, 1 );

    // "OpenPGP Signature Key" requester and label:
    ++row;
    mPGPSigningKeyRequester = new Kleo::SigningKeyRequester( false, Kleo::SigningKeyRequester::OpenPGP, tab );
    mPGPSigningKeyRequester->dialogButton()->setText( i18n("Chang&e...") );
    mPGPSigningKeyRequester->setDialogCaption( i18n("Your OpenPGP Signature Key") );
    msg = i18n("Select the OpenPGP key which should be used to "
               "digitally sign your messages.");
    mPGPSigningKeyRequester->setDialogMessage( msg );

    msg = i18n("<qt><p>The OpenPGP key you choose here will be used "
               "to digitally sign messages. You can also use GnuPG keys.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to digitally sign emails using OpenPGP; "
               "normal mail functions will not be affected.</p>"
               "<p>You can find out more about keys at <a>http://www.gnupg.org</a></p></qt>");

    label = new QLabel( i18n("OpenPGP signing key:"), tab );
    label->setBuddy( mPGPSigningKeyRequester );
    mPGPSigningKeyRequester->setWhatsThis( msg );
    label->setWhatsThis( msg );

    glay->addWidget( label, row, 0 );
    glay->addWidget( mPGPSigningKeyRequester, row, 1 );


    // "OpenPGP Encryption Key" requester and label:
    ++row;
    mPGPEncryptionKeyRequester = new Kleo::EncryptionKeyRequester( false, Kleo::EncryptionKeyRequester::OpenPGP, tab );
    mPGPEncryptionKeyRequester->dialogButton()->setText( i18n("Chang&e...") );
    mPGPEncryptionKeyRequester->setDialogCaption( i18n("Your OpenPGP Encryption Key") );
    msg = i18n("Select the OpenPGP key which should be used when encrypting "
               "to yourself and for the \"Attach My Public Key\" "
               "feature in the composer.");
    mPGPEncryptionKeyRequester->setDialogMessage( msg );

    msg = i18n("<qt><p>The OpenPGP key you choose here will be used "
               "to encrypt messages to yourself and for the \"Attach My Public Key\" "
               "feature in the composer. You can also use GnuPG keys.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to encrypt copies of outgoing messages to you using OpenPGP; "
               "normal mail functions will not be affected.</p>"
               "<p>You can find out more about keys at <a>http://www.gnupg.org</a></p></qt>");
    label = new QLabel( i18n("OpenPGP encryption key:"), tab );
    label->setBuddy( mPGPEncryptionKeyRequester );
    mPGPEncryptionKeyRequester->setWhatsThis( msg );
    label->setWhatsThis( msg );

    glay->addWidget( label, row, 0 );
    glay->addWidget( mPGPEncryptionKeyRequester, row, 1 );


    // "S/MIME Signature Key" requester and label:
    ++row;
    mSMIMESigningKeyRequester = new Kleo::SigningKeyRequester( false, Kleo::SigningKeyRequester::SMIME, tab );
    mSMIMESigningKeyRequester->dialogButton()->setText( i18n("Chang&e...") );
    mSMIMESigningKeyRequester->setDialogCaption( i18n("Your S/MIME Signature Certificate") );
    msg = i18n("Select the S/MIME certificate which should be used to "
               "digitally sign your messages.");
    mSMIMESigningKeyRequester->setDialogMessage( msg );

    msg = i18n("<qt><p>The S/MIME (X.509) certificate you choose here will be used "
               "to digitally sign messages.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to digitally sign emails using S/MIME; "
               "normal mail functions will not be affected.</p></qt>");
    label = new QLabel( i18n("S/MIME signing certificate:"), tab );
    label->setBuddy( mSMIMESigningKeyRequester );
    mSMIMESigningKeyRequester->setWhatsThis( msg );
    label->setWhatsThis( msg );
    glay->addWidget( label, row, 0 );
    glay->addWidget( mSMIMESigningKeyRequester, row, 1 );

    const Kleo::CryptoBackend::Protocol * smimeProtocol
      = Kleo::CryptoBackendFactory::instance()->smime();

    label->setEnabled( smimeProtocol );
    mSMIMESigningKeyRequester->setEnabled( smimeProtocol );

    // "S/MIME Encryption Key" requester and label:
    ++row;
    mSMIMEEncryptionKeyRequester = new Kleo::EncryptionKeyRequester( false, Kleo::EncryptionKeyRequester::SMIME, tab );
    mSMIMEEncryptionKeyRequester->dialogButton()->setText( i18n("Chang&e...") );
    mSMIMEEncryptionKeyRequester->setDialogCaption( i18n("Your S/MIME Encryption Certificate") );
    msg = i18n("Select the S/MIME certificate which should be used when encrypting "
               "to yourself and for the \"Attach My Certificate\" "
               "feature in the composer.");
    mSMIMEEncryptionKeyRequester->setDialogMessage( msg );

    msg = i18n("<qt><p>The S/MIME certificate you choose here will be used "
               "to encrypt messages to yourself and for the \"Attach My Certificate\" "
               "feature in the composer.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to encrypt copies of outgoing messages to you using S/MIME; "
               "normal mail functions will not be affected.</p></qt>");
    label = new QLabel( i18n("S/MIME encryption certificate:"), tab );
    label->setBuddy( mSMIMEEncryptionKeyRequester );
    mSMIMEEncryptionKeyRequester->setWhatsThis( msg );
    label->setWhatsThis( msg );

    glay->addWidget( label, row, 0 );
    glay->addWidget( mSMIMEEncryptionKeyRequester, row, 1 );

    label->setEnabled( smimeProtocol );
    mSMIMEEncryptionKeyRequester->setEnabled( smimeProtocol );

    // "Preferred Crypto Message Format" combobox and label:
    ++row;
    mPreferredCryptoMessageFormat = new QComboBox( tab );
    mPreferredCryptoMessageFormat->setEditable( false );
    QStringList l;
    l << Kleo::cryptoMessageFormatToLabel( Kleo::AutoFormat )
      << Kleo::cryptoMessageFormatToLabel( Kleo::InlineOpenPGPFormat )
      << Kleo::cryptoMessageFormatToLabel( Kleo::OpenPGPMIMEFormat )
      << Kleo::cryptoMessageFormatToLabel( Kleo::SMIMEFormat )
      << Kleo::cryptoMessageFormatToLabel( Kleo::SMIMEOpaqueFormat );
    mPreferredCryptoMessageFormat->addItems( l );
    label = new QLabel( i18n("Preferred crypto message format:"), tab );
    label->setBuddy( mPreferredCryptoMessageFormat );

    glay->addWidget( label, row, 0 );
    glay->addWidget( mPreferredCryptoMessageFormat, row, 1 );

    ++row;
    glay->setRowStretch( row, 1 );

    //
    // Tab Widget: Advanced
    //
    row = -1;
    tab = new QWidget( tabWidget );
    tabWidget->addTab( tab, i18n("&Advanced") );
    glay = new QGridLayout( tab );
    glay->setSpacing( spacingHint() );
    glay->setMargin( marginHint() );
    // the last (empty) row takes all the remaining space
    glay->setRowStretch( 8-1, 1 );
    glay->setColumnStretch( 1, 1 );

    // "Reply-To Address" line edit and label:
    ++row;
    mReplyToEdit = new KPIM::AddresseeLineEdit( tab, true );
    mReplyToEdit->setObjectName( "mReplyToEdit" );
    glay->addWidget( mReplyToEdit, row, 1 );
    label = new QLabel ( i18n("&Reply-To address:"), tab );
    label->setBuddy( mReplyToEdit );
    glay->addWidget( label, row, 0 );
    msg = i18n("<qt><h3>Reply-To addresses</h3>"
               "<p>This sets the <tt>Reply-to:</tt> header to contain a "
               "different email address to the normal <tt>From:</tt> "
               "address.</p>"
               "<p>This can be useful when you have a group of people "
               "working together in similar roles. For example, you "
               "might want any emails sent to have your email in the "
               "<tt>From:</tt> field, but any responses to go to "
               "a group address.</p>"
               "<p>If in doubt, leave this field blank.</p></qt>");
    label->setWhatsThis( msg );
    mReplyToEdit->setWhatsThis( msg );

    // "BCC addresses" line edit and label:
    ++row;
    mBccEdit = new KPIM::AddresseeLineEdit( tab, true );
    mBccEdit->setObjectName( "mBccEdit" );
    glay->addWidget( mBccEdit, row, 1 );
    label = new QLabel( i18n("&BCC addresses:"), tab );
    label->setBuddy( mBccEdit );
    glay->addWidget( label, row, 0 );
    msg = i18n("<qt><h3>BCC (Blind Carbon Copy) addresses</h3>"
               "<p>The addresses that you enter here will be added to each "
               "outgoing mail that is sent with this identity. They will not "
               "be visible to other recipients.</p>"
               "<p>This is commonly used to send a copy of each sent message to "
               "another account of yours.</p>"
               "<p>To specify more than one address, use commas to separate "
               "the list of BCC recipients.</p>"
               "<p>If in doubt, leave this field blank.</p></qt>");
    label->setWhatsThis( msg );
    mBccEdit->setWhatsThis( msg );

    // "Dictionary" combo box and label:
    ++row;
    mDictionaryCombo = new KPIM::DictionaryComboBox( tab );
    glay->addWidget( mDictionaryCombo, row, 1 );
    label = new QLabel( i18n("D&ictionary:"), tab );
    label->setBuddy( mDictionaryCombo );
    glay->addWidget( label, row, 0 );

    // "Sent-mail Folder" combo box and label:
    ++row;
    mFccCombo = new FolderRequester( tab,
        kmkernel->getKMMainWidget()->folderTree() );
    mFccCombo->setShowOutbox( false );
    glay->addWidget( mFccCombo, row, 1 );
    label = new QLabel( i18n("Sent-mail &folder:"), tab );
    label->setBuddy( mFccCombo );
    glay->addWidget( label, row, 0 );

    // "Drafts Folder" combo box and label:
    ++row;
    mDraftsCombo = new FolderRequester( tab,
        kmkernel->getKMMainWidget()->folderTree() );
    mDraftsCombo->setShowOutbox( false );
    glay->addWidget( mDraftsCombo, row, 1 );
    label = new QLabel( i18n("&Drafts folder:"), tab );
    label->setBuddy( mDraftsCombo );
    glay->addWidget( label, row, 0 );

    // "Templates Folder" combo box and label:
    ++row;
    mTemplatesCombo = new FolderRequester( tab,
        kmkernel->getKMMainWidget()->folderTree() );
    mTemplatesCombo->setShowOutbox( false );
    glay->addWidget( mTemplatesCombo, row, 1 );
    label = new QLabel( i18n("&Templates folder:"), tab );
    label->setBuddy( mTemplatesCombo );
    glay->addWidget( label, row, 0 );

    // "Special transport" combobox and label:
    ++row;
    mTransportCheck = new QCheckBox( i18n("Special &transport:"), tab );
    glay->addWidget( mTransportCheck, row, 0 );
    mTransportCombo = new TransportComboBox( tab );
    mTransportCombo->setEnabled( false ); // since !mTransportCheck->isChecked()
    glay->addWidget( mTransportCombo, row, 1 );
    connect( mTransportCheck, SIGNAL(toggled(bool)),
             mTransportCombo, SLOT(setEnabled(bool)) );

    // the last row is a spacer

    //
    // Tab Widget: Templates
    //
    tab = new QWidget( tabWidget );
    tabWidget->addTab( tab, i18n("&Templates") );
    vlay = new QVBoxLayout( tab );
    vlay->setMargin( marginHint() );
    vlay->setSpacing( spacingHint() );

    QHBoxLayout *tlay = new QHBoxLayout();
    vlay->addLayout( tlay );

    mCustom = new QCheckBox( i18n("&Use custom message templates"), tab );
    tlay->addWidget( mCustom, Qt::AlignLeft );

    mWidget = new TemplatesConfiguration( tab, "identity-templates" );
    mWidget->setEnabled( false );

    // Move the help label outside of the templates configuration widget,
    // so that the help can be read even if the widget is not enabled.
    tlay->addStretch( 9 );
    tlay->addWidget( mWidget->helpLabel(), Qt::AlignRight );

    vlay->addWidget( mWidget );

    QHBoxLayout *btns = new QHBoxLayout();
    btns->setSpacing( spacingHint() );
    mCopyGlobal = new KPushButton( i18n("&Copy Global Templates"), tab );
    mCopyGlobal->setEnabled( false );
    btns->addWidget( mCopyGlobal );
    vlay->addLayout( btns );
    connect( mCustom, SIGNAL(toggled( bool )),
             mWidget, SLOT(setEnabled( bool )) );
    connect( mCustom, SIGNAL(toggled( bool )),
             mCopyGlobal, SLOT(setEnabled( bool )) );
    connect( mCopyGlobal, SIGNAL(clicked()),
             this, SLOT(slotCopyGlobal()) );

    //
    // Tab Widget: Signature
    //
    mSignatureConfigurator = new KPIMIdentities::SignatureConfigurator( tabWidget );
    mSignatureConfigurator->layout()->setMargin( KDialog::marginHint() );
    tabWidget->addTab( mSignatureConfigurator, i18n("&Signature") );

    mXFaceConfigurator = new XFaceConfigurator( tabWidget );
    mXFaceConfigurator->layout()->setMargin( KDialog::marginHint() );
    tabWidget->addTab( mXFaceConfigurator, i18n("&Picture") );

    KConfigGroup geometry( KMKernel::config(), "Geometry" );
    if ( geometry.hasKey( "Identity Dialog size" ) )
      resize( geometry.readEntry( "Identity Dialog size", QSize() ));
    mNameEdit->setFocus();

    connect( tabWidget, SIGNAL(currentChanged(QWidget*)),
             SLOT(slotAboutToShow(QWidget*)) );
    setHelp( QString(), "kmail" );
  }

  IdentityDialog::~IdentityDialog() {
    KConfigGroup geometry( KMKernel::config(), "Geometry" );
    geometry.writeEntry( "Identity Dialog size", size() );
  }

  void IdentityDialog::slotAboutToShow( QWidget * w ) {
    if ( w == mCryptographyTab ) {
      // set the configured email address as initial query of the key
      // requesters:
      const QString email = mEmailEdit->text().trimmed();
      mPGPEncryptionKeyRequester->setInitialQuery( email );
      mPGPSigningKeyRequester->setInitialQuery( email );
      mSMIMEEncryptionKeyRequester->setInitialQuery( email );
      mSMIMESigningKeyRequester->setInitialQuery( email );
    }
  }

  void IdentityDialog::slotCopyGlobal() {
    mWidget->loadFromGlobal();
  }

  namespace {
    struct DoesntMatchEMailAddress {
      explicit DoesntMatchEMailAddress( const QString & s )
        : email( s.trimmed().toLower() ) {}
      bool operator()( const GpgME::Key & key ) const;
    private:
      bool checkForEmail( const char * email ) const;
      static QString extractEmail( const char * email );
      const QString email;
    };

    bool DoesntMatchEMailAddress::operator()( const GpgME::Key & key ) const {
      const std::vector<GpgME::UserID> uids = key.userIDs();
      for ( std::vector<GpgME::UserID>::const_iterator it = uids.begin() ; it != uids.end() ; ++it )
        if ( checkForEmail( it->email() ? it->email() : it->id() ) )
          return false;
      return true; // note the negation!
    }

    bool DoesntMatchEMailAddress::checkForEmail( const char * e ) const {
      const QString em = extractEmail( e );
      return !em.isEmpty() && email.toLower() == em.toLower();
    }

    QString DoesntMatchEMailAddress::extractEmail( const char * e ) {
      if ( !e || !*e )
        return QString();
      const QString em = QString::fromUtf8( e );
      if ( e[0] == '<' )
        return em.mid( 1, em.length() - 2 );
      else
        return em;
    }
  }

  void IdentityDialog::slotButtonClicked( int button )
  {
    if ( button != KDialog::Ok ) {
      KDialog::slotButtonClicked( button );
      return;
    }

    const QString email = mEmailEdit->text().trimmed();

    // Validate email addresses
    if ( !KPIMUtils::isValidSimpleAddress( email ) ) {
      QString errorMsg( KPIMUtils::simpleEmailAddressErrorMsg() );
      KMessageBox::sorry( this, errorMsg, i18n("Invalid Email Address") );
      return;
    }

    if ( !Util::validateAddresses( this, mReplyToEdit->text().trimmed() ) ) {
      return;
    }

    if ( !Util::validateAddresses( this, mBccEdit->text().trimmed() ) ) {
      return;
    }

    const std::vector<GpgME::Key> &pgpSigningKeys =
      mPGPSigningKeyRequester->keys();
    const std::vector<GpgME::Key> &pgpEncryptionKeys =
      mPGPEncryptionKeyRequester->keys();
    const std::vector<GpgME::Key> &smimeSigningKeys =
      mSMIMESigningKeyRequester->keys();
    const std::vector<GpgME::Key> &smimeEncryptionKeys =
      mSMIMEEncryptionKeyRequester->keys();

    QString msg;
    bool err = false;
    if ( std::find_if( pgpSigningKeys.begin(), pgpSigningKeys.end(),
                       DoesntMatchEMailAddress( email ) ) != pgpSigningKeys.end() ) {
      msg = i18n("One of the configured OpenPGP signing keys does not contain "
                 "any user ID with the configured email address for this "
                 "identity (%1).\n"
                 "This might result in warning messages on the receiving side "
                 "when trying to verify signatures made with this configuration.", email);
      err = true;
    } else if ( std::find_if( pgpEncryptionKeys.begin(), pgpEncryptionKeys.end(),
                              DoesntMatchEMailAddress( email ) ) != pgpEncryptionKeys.end() ) {
      msg = i18n("One of the configured OpenPGP encryption keys does not contain "
                 "any user ID with the configured email address for this "
                 "identity (%1).", email);
      err = true;
    } else if ( std::find_if( smimeSigningKeys.begin(), smimeSigningKeys.end(),
                              DoesntMatchEMailAddress( email ) ) != smimeSigningKeys.end() ) {
      msg = i18n("One of the configured S/MIME signing certificates does not contain "
                 "the configured email address for this "
                 "identity (%1).\n"
                 "This might result in warning messages on the receiving side "
                 "when trying to verify signatures made with this configuration.", email);
      err = true;
    } else if ( std::find_if( smimeEncryptionKeys.begin(), smimeEncryptionKeys.end(),
                              DoesntMatchEMailAddress( email ) ) != smimeEncryptionKeys.end() ) {
      msg = i18n("One of the configured S/MIME encryption certificates does not contain "
                 "the configured email address for this "
                 "identity (%1).", email);
      err = true;
    }

    if ( err ) {
      if ( KMessageBox::warningContinueCancel( this, msg,
                                               i18n("Email Address Not Found in Key/Certificates"),
                                               KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel(),
                                               "warn_email_not_in_certificate" )
           != KMessageBox::Continue) {
        return;
      }
    }


    if ( mSignatureConfigurator->isSignatureEnabled() &&
         mSignatureConfigurator->signatureType()==Signature::FromFile ) {
      KUrl url( mSignatureConfigurator->fileURL() );
      KFileItem signatureFile( KFileItem::Unknown, KFileItem::Unknown, url );
      if ( !signatureFile.isFile() || !signatureFile.isReadable() || !signatureFile.isLocalFile() ) {
        KMessageBox::error( this, i18n( "The signature file is not valid" ) );
        return;
      }
    }

    accept();
  }

  bool IdentityDialog::checkFolderExists( const QString & folderID,
                                          const QString & msg ) {
    KMFolder * folder = kmkernel->findFolderById( folderID );
    if ( !folder ) {
      KMessageBox::sorry( this, msg );
      return false;
    }
    return true;
  }

  void IdentityDialog::setIdentity( KPIMIdentities::Identity & ident ) {

    setCaption( i18n("Edit Identity \"%1\"", ident.identityName() ) );

    // "General" tab:
    mNameEdit->setText( ident.fullName() );
    mOrganizationEdit->setText( ident.organization() );
    mEmailEdit->setText( ident.emailAddr() );

    // "Cryptography" tab:
    mPGPSigningKeyRequester->setFingerprint( ident.pgpSigningKey() );
    mPGPEncryptionKeyRequester->setFingerprint( ident.pgpEncryptionKey() );
    mSMIMESigningKeyRequester->setFingerprint( ident.smimeSigningKey() );
    mSMIMEEncryptionKeyRequester->setFingerprint( ident.smimeEncryptionKey() );

    mPreferredCryptoMessageFormat->setCurrentIndex( format2cb(
       Kleo::stringToCryptoMessageFormat( ident.preferredCryptoMessageFormat() ) ) );

    // "Advanced" tab:
    mReplyToEdit->setText( ident.replyToAddr() );
    mBccEdit->setText( ident.bcc() );
    QString transportName = ident.transport();
    Transport *transport =
              TransportManager::self()->transportByName( transportName, false );
    mTransportCheck->setChecked( transport != 0 );
    mTransportCombo->setEnabled( transport != 0 );
    if ( transport )
      mTransportCombo->setCurrentTransport( transport->id() );
    mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

    if ( ident.fcc().isEmpty() ||
         !checkFolderExists( ident.fcc(),
                             i18n("The custom sent-mail folder for identity "
                                  "\"%1\" does not exist (anymore); "
                                  "therefore, the default sent-mail folder "
                                  "will be used.",
                               ident.identityName() ) ) )
      mFccCombo->setFolder( kmkernel->sentFolder() );
    else
      mFccCombo->setFolder( ident.fcc() );

    if ( ident.drafts().isEmpty() ||
         !checkFolderExists( ident.drafts(),
                             i18n("The custom drafts folder for identity "
                                  "\"%1\" does not exist (anymore); "
                                  "therefore, the default drafts folder "
                                  "will be used.",
                               ident.identityName() ) ) )
      mDraftsCombo->setFolder( kmkernel->draftsFolder() );
    else
      mDraftsCombo->setFolder( ident.drafts() );

    if ( ident.templates().isEmpty() ||
         !checkFolderExists( ident.templates(),
                             i18n("The custom templates folder for identity "
                                  "\"%1\" does not exist (anymore); "
                                  "therefore, the default templates folder "
                                  "will be used.", ident.identityName()) ) )
      mTemplatesCombo->setFolder( kmkernel->templatesFolder() );
    else
      mTemplatesCombo->setFolder( ident.templates() );

    // "Templates" tab:
    uint identity = ident.uoid();
    QString iid = QString("IDENTITY_%1").arg( identity );
    Templates t( iid );
    mCustom->setChecked(t.useCustomTemplates());
    mWidget->loadFromIdentity( identity );

    // "Signature" tab:
    mSignatureConfigurator->setSignature( ident.signature() );
    mXFaceConfigurator->setXFace( ident.xface() );
    mXFaceConfigurator->setXFaceEnabled( ident.isXFaceEnabled() );
  }

  void IdentityDialog::updateIdentity( KPIMIdentities::Identity & ident ) {
    // "General" tab:
    ident.setFullName( mNameEdit->text() );
    ident.setOrganization( mOrganizationEdit->text() );
    QString email = mEmailEdit->text();
    ident.setEmailAddr( email );
    // "Cryptography" tab:
    ident.setPGPSigningKey( mPGPSigningKeyRequester->fingerprint().toLatin1() );
    ident.setPGPEncryptionKey( mPGPEncryptionKeyRequester->fingerprint().toLatin1() );
    ident.setSMIMESigningKey( mSMIMESigningKeyRequester->fingerprint().toLatin1() );
    ident.setSMIMEEncryptionKey( mSMIMEEncryptionKeyRequester->fingerprint().toLatin1() );
    ident.setPreferredCryptoMessageFormat(
       Kleo::cryptoMessageFormatToString(cb2format( mPreferredCryptoMessageFormat->currentIndex() ) ) );
    // "Advanced" tab:
    ident.setReplyToAddr( mReplyToEdit->text() );
    ident.setBcc( mBccEdit->text() );
    ident.setTransport( ( mTransportCheck->isChecked() ) ?
                          mTransportCombo->currentText() : QString() );
    ident.setDictionary( mDictionaryCombo->currentDictionary() );
    ident.setFcc( mFccCombo->folder() ?
                  mFccCombo->folder()->idString() : QString() );
    ident.setDrafts( mDraftsCombo->folder() ?
                     mDraftsCombo->folder()->idString() : QString() );
    ident.setTemplates( mTemplatesCombo->folder() ?
                     mTemplatesCombo->folder()->idString() : QString() );

    // "Templates" tab:
    uint identity = ident.uoid();
    QString iid = QString("IDENTITY_%1").arg( identity );
    Templates t( iid );
    kDebug(5006) <<"use custom templates for identity" << identity <<":" << mCustom->isChecked();
    t.setUseCustomTemplates(mCustom->isChecked());
    t.writeConfig();
    mWidget->saveToIdentity( identity );

    // "Signature" tab:
    ident.setSignature( mSignatureConfigurator->signature() );
    ident.setXFace( mXFaceConfigurator->xface() );
    ident.setXFaceEnabled( mXFaceConfigurator->isXFaceEnabled() );
  }
}

#include "identitydialog.moc"
