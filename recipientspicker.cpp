/*
    This file is part of KMail.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#include "recipientspicker.h"
#include "globalsettings.h"

#include <akonadi/contact/emailaddressselectionview.h>
#include <kabc/contactgroup.h>
#include <kconfiggroup.h>
#include <khbox.h>
#include <kldap/ldapsearchdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpimutils/email.h>

#include <QBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QtGui/QTreeView>
#include <QToolButton>
#include <QVBoxLayout>

RecipientsPicker::RecipientsPicker( QWidget *parent )
  : KDialog( parent ),
    mLdapSearchDialog( 0 )
{
  setObjectName( "RecipientsPicker" );
  setWindowTitle( i18n( "Select Recipient" ) );
  setButtons( None );

  QVBoxLayout *topLayout = new QVBoxLayout( mainWidget() );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( 0 );

  mView = new Akonadi::EmailAddressSelectionView( mainWidget() );
  mView->view()->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mView->view()->setAlternatingRowColors( true );
  mView->view()->setSortingEnabled( true );
  mView->view()->sortByColumn( 0, Qt::AscendingOrder );
  topLayout->addWidget( mView );
  topLayout->setStretchFactor( mView, 1 );

  connect( mView->view()->selectionModel(), SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( slotSelectionChanged() ) );
  connect( mView->view(), SIGNAL( doubleClicked( const QModelIndex& ) ),
           SLOT( slotPicked() ) );

  mSearchLDAPButton = new QPushButton( i18n("Search &Directory Service"), mainWidget() );
  connect( mSearchLDAPButton, SIGNAL( clicked() ), SLOT( slotSearchLDAP() ) );
  topLayout->addWidget( mSearchLDAPButton );

  KConfig config( "kabldaprc" );
  KConfigGroup group = config.group( "LDAP" );
  int numHosts = group.readEntry( "NumSelectedHosts", 0 );
  if ( !numHosts )
     mSearchLDAPButton->setVisible( false );

  QBoxLayout *buttonLayout = new QHBoxLayout();
  topLayout->addItem( buttonLayout );

  buttonLayout->addStretch( 1 );  // right align buttons
  buttonLayout->setSpacing( KDialog::spacingHint() );

  mToButton = new QPushButton( i18n("Add as &To"), mainWidget() );
  buttonLayout->addWidget( mToButton );
  connect( mToButton, SIGNAL( clicked() ), SLOT( slotToClicked() ) );

  mCcButton = new QPushButton( i18n("Add as CC"), mainWidget() );
  buttonLayout->addWidget( mCcButton );
  connect( mCcButton, SIGNAL( clicked() ), SLOT( slotCcClicked() ) );

  mBccButton = new QPushButton( i18n("Add as &BCC"), mainWidget() );
  buttonLayout->addWidget( mBccButton );
  connect( mBccButton, SIGNAL( clicked() ), SLOT( slotBccClicked() ) );

  QPushButton *closeButton = new QPushButton( i18n("&Cancel"), mainWidget() );
  buttonLayout->addWidget( closeButton );
  connect( closeButton, SIGNAL( clicked() ), SLOT( close() ) );

  mView->searchLineEdit()->setFocus();

  readConfig();

  slotSelectionChanged();
}

RecipientsPicker::~RecipientsPicker()
{
  writeConfig();
}

void RecipientsPicker::slotSelectionChanged()
{
  const bool hasSelection = !mView->selectedAddresses().isEmpty();
  mToButton->setEnabled( hasSelection );
  mBccButton->setEnabled( hasSelection );
  mCcButton->setEnabled( hasSelection );
}

void RecipientsPicker::setRecipients( const Recipient::List& )
{
  mView->view()->selectionModel()->clear();
}

void RecipientsPicker::setDefaultType( Recipient::Type type )
{
  mDefaultType = type;
  mToButton->setDefault( type == Recipient::To );
  mCcButton->setDefault( type == Recipient::Cc );
  mBccButton->setDefault( type == Recipient::Bcc );
}

void RecipientsPicker::slotToClicked()
{
  pick( Recipient::To );
}

void RecipientsPicker::slotCcClicked()
{
  pick( Recipient::Cc );
}

void RecipientsPicker::slotBccClicked()
{
  pick( Recipient::Bcc );
}

void RecipientsPicker::slotPicked()
{
  pick( mDefaultType );
}

void RecipientsPicker::pick( Recipient::Type type )
{
  kDebug() << int( type );

  const Akonadi::EmailAddressSelectionView::Selection::List selections = mView->selectedAddresses();

  const int count = selections.count();

  if ( count > GlobalSettings::self()->maximumRecipients() ) {
    KMessageBox::sorry( this,
        i18np( "You selected 1 recipient. The maximum supported number of "
               "recipients is %2. Please adapt the selection.",
               "You selected %1 recipients. The maximum supported number of "
               "recipients is %2. Please adapt the selection.", count,
               GlobalSettings::self()->maximumRecipients() ) );
    return;
  }

  foreach ( const Akonadi::EmailAddressSelectionView::Selection &selection, selections ) {
    Recipient recipient;
    recipient.setType( type );

    if ( selection.item().hasPayload<KABC::ContactGroup>() )
      recipient.setEmail( selection.email() );
    else {
      KABC::Addressee contact;
      contact.setName( selection.name() );
      contact.insertEmail( selection.email() );
      recipient.setEmail( contact.fullEmail() );
    }

    emit pickedRecipient( recipient );
  }

  close();
}

void RecipientsPicker::keyPressEvent( QKeyEvent *event )
{
  if ( event->key() == Qt::Key_Escape )
    close();

  KDialog::keyPressEvent( event );
}

void RecipientsPicker::readConfig()
{
  KSharedConfig::Ptr cfg = KGlobal::config();
  KConfigGroup group( cfg, "RecipientsPicker" );
  QSize size = group.readEntry( "Size", QSize() );
  if ( !size.isEmpty() ) {
    resize( size );
  }
}

void RecipientsPicker::writeConfig()
{
  KSharedConfig::Ptr cfg = KGlobal::config();
  KConfigGroup group( cfg, "RecipientsPicker" );
  group.writeEntry( "Size", size() );
}

void RecipientsPicker::slotSearchLDAP()
{
  if ( !mLdapSearchDialog ) {
    mLdapSearchDialog = new KLDAP::LdapSearchDialog( this );
    connect( mLdapSearchDialog, SIGNAL( contactsAdded() ),
             SLOT(ldapSearchResult() ) );
  }

  mLdapSearchDialog->setSearchText( mView->searchLineEdit()->text() );
  mLdapSearchDialog->show();
}

void RecipientsPicker::ldapSearchResult()
{
  const KABC::Addressee::List contacts = mLdapSearchDialog->selectedContacts();
  foreach ( const KABC::Addressee &contact, contacts ) {
    emit pickedRecipient( Recipient( contact.fullEmail(), Recipient::Undefined ) );
  }
}

#include "recipientspicker.moc"
