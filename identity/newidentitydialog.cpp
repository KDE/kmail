/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   Copyright (C) 2010 Volker Krause <vkrause@kde.org>
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
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "newidentitydialog.h"

#include <kpimidentities/identitymanager.h>

#include <KComboBox>
#include <KLineEdit>
#include <KLocalizedString>
#include <KSeparator>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include <assert.h>

using namespace KMail;

NewIdentityDialog::NewIdentityDialog( KPIMIdentities::IdentityManager* manager, QWidget *parent )
    : KDialog( parent ),
      mIdentityManager( manager )
{
    setCaption( i18n("New Identity") );
    setButtons( Ok|Cancel|Help );
    setHelp( QString::fromLatin1("configure-identity-newidentitydialog") );
    QWidget *page = new QWidget( this );
    setMainWidget( page );
    QVBoxLayout * vlay = new QVBoxLayout( page );
    vlay->setSpacing( spacingHint() );
    vlay->setMargin( 0 );

    // row 0: line edit with label
    QHBoxLayout * hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mLineEdit = new KLineEdit( page );
    mLineEdit->setFocus();
    mLineEdit->setClearButtonShown( true );
    QLabel *l = new QLabel( i18n("&New identity:"), page );
    l->setBuddy( mLineEdit );
    hlay->addWidget( l );
    hlay->addWidget( mLineEdit, 1 );
    connect( mLineEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotEnableOK(QString)) );

    mButtonGroup = new QButtonGroup( page );

    // row 1: radio button
    QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), page );
    radio->setChecked( true );
    vlay->addWidget( radio );
    mButtonGroup->addButton( radio, (int)Empty );

    // row 2: radio button
    radio = new QRadioButton( i18n("&Use System Settings values"), page );
    vlay->addWidget( radio );
    mButtonGroup->addButton( radio, (int)ControlCenter );

    // row 3: radio button
    radio = new QRadioButton( i18n("&Duplicate existing identity"), page );
    vlay->addWidget( radio );
    mButtonGroup->addButton( radio, (int)ExistingEntry );

    // row 4: combobox with existing identities and label
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mComboBox = new KComboBox( page );
    mComboBox->setEditable( false );
    mComboBox->addItems( manager->shadowIdentities() );
    mComboBox->setEnabled( false );
    QLabel *label = new QLabel( i18n("&Existing identities:"), page );
    label->setBuddy( mComboBox );
    label->setEnabled( false );
    hlay->addWidget( label );
    hlay->addWidget( mComboBox, 1 );

    vlay->addWidget(new KSeparator);
    vlay->addStretch( 1 ); // spacer

    // enable/disable combobox and label depending on the third radio
    // button's state:
    connect( radio, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( radio, SIGNAL(toggled(bool)),
             mComboBox, SLOT(setEnabled(bool)) );

    enableButtonOk( false ); // since line edit is empty
    resize(400,180);
}

NewIdentityDialog::DuplicateMode NewIdentityDialog::duplicateMode() const
{
    const int id = mButtonGroup->checkedId();
    assert( id == (int)Empty
            || id == (int)ControlCenter
            || id == (int)ExistingEntry );
    return static_cast<DuplicateMode>( id );
}

void NewIdentityDialog::slotEnableOK( const QString & proposedIdentityName )
{
    // OK button is disabled if
    const QString name = proposedIdentityName.trimmed();
    // name isn't empty
    if ( name.isEmpty() ) {
        enableButtonOk( false );
        return;
    }
    // or name doesn't yet exist.
    if ( !mIdentityManager->isUnique( name ) ) {
        enableButtonOk( false );
        return;
    }
    enableButtonOk( true );
}

QString NewIdentityDialog::identityName() const
{
    return mLineEdit->text();
}

QString NewIdentityDialog::duplicateIdentity() const
{
    return mComboBox->currentText();
}


#include "moc_newidentitydialog.cpp"
