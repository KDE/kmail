/*  -*- c++ -*-
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#include "signatureconfigurator.h"
#include "kmfoldercombobox.h"
#include "kmfoldermgr.h"
#include "transportmanager.h"
#include "kmkernel.h"


// other kdenetwork headers:
#include <kpgpui.h>

// other KDE headers:
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>

// Qt headers:
#include <qtabwidget.h>
#include <qlabel.h>
#include <qwhatsthis.h>

// other headers: (none)

namespace KMail {

  IdentityDialog::IdentityDialog( QWidget * parent, const char * name )
    : KDialogBase( Plain, i18n("Edit Identity"), Ok|Cancel|Help, Ok,
		   parent, name )
  {
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
    QVBoxLayout * vlay = new QVBoxLayout( plainPage(), 0, spacingHint() );
    QTabWidget *tabWidget = new QTabWidget( plainPage(), "config-identity-tab" );
    vlay->addWidget( tabWidget );

    tab = new QWidget( tabWidget );
    tabWidget->addTab( tab, i18n("&General") );
    glay = new QGridLayout( tab, 4, 2, marginHint(), spacingHint() );
    glay->setRowStretch( 3, 1 );
    glay->setColStretch( 1, 1 );

    // "Name" line edit and label:
    ++row;
    mNameEdit = new KLineEdit( tab );
    glay->addWidget( mNameEdit, row, 1 );
    glay->addWidget( new QLabel( mNameEdit, i18n("&Your name:"), tab ), row, 0 );

    // "Organization" line edit and label:
    ++row;
    mOrganizationEdit = new KLineEdit( tab );
    glay->addWidget( mOrganizationEdit, row, 1 );
    glay->addWidget( new QLabel( mOrganizationEdit,
				 i18n("Organi&zation:"), tab ), row, 0 );

    // "Email Address" line edit and label:
    // (row 3: spacer)
    ++row;
    mEmailEdit = new KLineEdit( tab );
    glay->addWidget( mEmailEdit, row, 1 );
    glay->addWidget( new QLabel( mEmailEdit, i18n("&Email address:"), tab ),
		     row, 0 );

    //
    // Tab Widget: Advanced
    //
    row = -1;
    tab = new QWidget( tabWidget );
    tabWidget->addTab( tab, i18n("&Advanced") );
    glay = new QGridLayout( tab, 7, 2, marginHint(), spacingHint() );
    // the last (empty) row takes all the remaining space
    glay->setRowStretch( 7-1, 1 );
    glay->setColStretch( 1, 1 );

    // "Reply-To Address" line edit and label:
    ++row;
    mReplyToEdit = new KLineEdit( tab );
    glay->addWidget( mReplyToEdit, row, 1 );
    glay->addWidget( new QLabel( mReplyToEdit,
				 i18n("&Reply-To address:"), tab ), row, 0 );

    // "BCC addresses" line edit and label:
    ++row;
    mBccEdit = new KLineEdit( tab );
    glay->addWidget( mBccEdit, row, 1 );
    label = new QLabel( mBccEdit, i18n("&BCC addresses:"), tab );
    glay->addWidget( label, row, 0 );
    msg = i18n("<qt><h3>BCC (Blind Carbon Copy) addresses</h3>"
	       "<p>The addresses that you enter here will be added to each "
	       "outgoing mail that is sent with this identity. They will not "
	       "be visible to other recipients.</p>"
	       "<p>This is commonly used to send a copy of each sent message to "
	       "another account of yours.</p>"
	       "<p>If in doubt, leave this field blank.</p></qt>");
    QWhatsThis::add( label, msg );
    QWhatsThis::add( mBccEdit, msg );

    // "OpenPGP Key" requester and label:
    ++row;
    mPgpKeyRequester = new Kpgp::SecretKeyRequester( tab );
    mPgpKeyRequester->dialogButton()->setText( i18n("Chang&e...") );
    mPgpKeyRequester->setDialogCaption( i18n("Your OpenPGP Key") );
    mPgpKeyRequester->setDialogMessage( i18n("Select the OpenPGP key which "
					     "should be used to sign your "
					     "messages and when encrypting to "
					     "yourself.") );
    msg = i18n("<qt><p>The OpenPGP key you choose here will be used "
	       "to sign messages and to encrypt messages to "
	       "yourself.</p></qt>");

    label = new QLabel( mPgpKeyRequester, i18n("OpenPGP key:"), tab );
    QWhatsThis::add( mPgpKeyRequester, msg );
    QWhatsThis::add( label, msg );

    glay->addWidget( label, row, 0 );
    glay->addWidget( mPgpKeyRequester, row, 1 );

    // "Sent-mail Folder" combo box and label:
    ++row;
    mFccCombo = new KMFolderComboBox( tab );
    mFccCombo->showOutboxFolder( false );
    glay->addWidget( mFccCombo, row, 1 );
    glay->addWidget( new QLabel( mFccCombo, i18n("Sent-mail &folder:"), tab ),
		     row, 0 );

    // "Drafts Folder" combo box and label:
    ++row;
    mDraftsCombo = new KMFolderComboBox( tab );
    mDraftsCombo->showOutboxFolder( false );
    glay->addWidget( mDraftsCombo, row, 1 );
    glay->addWidget( new QLabel( mDraftsCombo, i18n("&Drafts folder:"), tab ),
		     row, 0 );

    // "Special transport" combobox and label:
    ++row;
    mTransportCheck = new QCheckBox( i18n("Special &transport:"), tab );
    glay->addWidget( mTransportCheck, row, 0 );
    mTransportCombo = new QComboBox( true, tab );
    mTransportCombo->setEnabled( false ); // since !mTransportCheck->isChecked()
    mTransportCombo->insertStringList( KMail::TransportManager::transportNames() );
    glay->addWidget( mTransportCombo, row, 1 );
    connect( mTransportCheck, SIGNAL(toggled(bool)),
	     mTransportCombo, SLOT(setEnabled(bool)) );

    // the last row is a spacer

    //
    // Tab Widget: Signature
    //
    mSignatureConfigurator = new SignatureConfigurator( tabWidget );
    mSignatureConfigurator->layout()->setMargin( KDialog::marginHint() );
    tabWidget->addTab( mSignatureConfigurator, i18n("&Signature") );

    KConfigGroup geometry( KMKernel::config(), "Geometry" );
    if ( geometry.hasKey( "Identity Dialog size" ) )
      resize( geometry.readSizeEntry( "Identity Dialog size" ) );
    mNameEdit->setFocus();
  }

  IdentityDialog::~IdentityDialog() {
    KConfigGroup geometry( KMKernel::config(), "Geometry" );
    geometry.writeEntry( "Identity Dialog size", size() );
  }

  bool IdentityDialog::checkFolderExists( const QString & folderID,
					  const QString & msg ) {
    KMFolder * folder = kernel->folderMgr()->findIdString( folderID );
    if ( !folder )
      folder = kernel->imapFolderMgr()->findIdString( folderID );
    if ( !folder ) {
      KMessageBox::sorry( this, msg );
      return false;
    }
    return true;
  }

  void IdentityDialog::setIdentity( KMIdentity & ident ) {

    setCaption( i18n("Edit Identity \"%1\"").arg( ident.identityName() ) );

    // "General" tab:
    mNameEdit->setText( ident.fullName() );
    mOrganizationEdit->setText( ident.organization() );
    mEmailEdit->setText( ident.emailAddr() );

    // "Advanced" tab:
    mPgpKeyRequester->setKeyIDs( Kpgp::KeyIDList() << ident.pgpIdentity() );
    mReplyToEdit->setText( ident.replyToAddr() );
    mBccEdit->setText( ident.bcc() );
    mTransportCheck->setChecked( !ident.transport().isEmpty() );
    mTransportCombo->setEditText( ident.transport() );
    mTransportCombo->setEnabled( !ident.transport().isEmpty() );

    if ( ident.fcc().isEmpty() ||
	 !checkFolderExists( ident.fcc(),
			     i18n("The custom sent-mail folder for identity "
				  "\"%1\" doesn't exist (anymore). "
				  "Therefore the default sent-mail folder "
				  "will be used.")
			     .arg( ident.identityName() ) ) )
      mFccCombo->setFolder( kernel->sentFolder() );
    else
      mFccCombo->setFolder( ident.fcc() );

    if ( ident.drafts().isEmpty() ||
	 !checkFolderExists( ident.drafts(),
			     i18n("The custom drafts folder for identity "
				  "\"%1\" doesn't exist (anymore). "
				  "Therefore the default drafts folder "
				  "will be used.")
			     .arg( ident.identityName() ) ) )
      mDraftsCombo->setFolder( kernel->draftsFolder() );
    else
      mDraftsCombo->setFolder( ident.drafts() );

    // "Signature" tab:
    mSignatureConfigurator->setSignature( ident.signature() );
  }

  void IdentityDialog::updateIdentity( KMIdentity & ident ) {
    // "General" tab:
    ident.setFullName( mNameEdit->text() );
    ident.setOrganization( mOrganizationEdit->text() );
    ident.setEmailAddr( mEmailEdit->text() );
    // "Advanced" tab:
    ident.setPgpIdentity( mPgpKeyRequester->keyIDs().first() );
    ident.setReplyToAddr( mReplyToEdit->text() );
    ident.setBcc( mBccEdit->text() );
    ident.setTransport( ( mTransportCheck->isChecked() ) ?
			mTransportCombo->currentText() : QString::null );
    ident.setFcc( mFccCombo->getFolder() ?
		  mFccCombo->getFolder()->idString() : QString::null );
    ident.setDrafts( mDraftsCombo->getFolder() ?
		     mDraftsCombo->getFolder()->idString() : QString::null );
    // "Signature" tab:
    ident.setSignature( mSignatureConfigurator->signature() );
  }

  void IdentityDialog::slotUpdateTransportCombo( const QStringList & sl ) {
    // save old setting:
    QString content = mTransportCombo->currentText();
    // update combo box:
    mTransportCombo->clear();
    mTransportCombo->insertStringList( sl );
    // restore saved setting:
    mTransportCombo->setEditText( content );
  };

};

#include "identitydialog.moc"
