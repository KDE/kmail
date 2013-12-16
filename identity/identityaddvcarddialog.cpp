/*
  Copyright (c) 2012, 2013 Montel Laurent <montel@kde.org>
  
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "identityaddvcarddialog.h"

#include <kpimidentities/identitymanager.h>

#include <KComboBox>
#include <KLocalizedString>
#include <KSeparator>

#include <QButtonGroup>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>


IdentityAddVcardDialog::IdentityAddVcardDialog(KPIMIdentities::IdentityManager *manager, QWidget *parent)
    :KDialog(parent)
{
    setCaption( i18n( "Create own vCard" ) );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    setModal( true );
    QWidget *mainWidget = new QWidget( this );
    QVBoxLayout *vlay = new QVBoxLayout( mainWidget );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );
    setMainWidget( mainWidget );

    mButtonGroup = new QButtonGroup( this );

    // row 1: radio button
    QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), this );
    radio->setChecked( true );
    vlay->addWidget( radio );
    mButtonGroup->addButton( radio, (int)Empty );

    // row 2: radio button
    radio = new QRadioButton( i18n("&Duplicate existing vCard"), this );
    vlay->addWidget( radio );
    mButtonGroup->addButton( radio, (int)ExistingEntry );

    // row 3: combobox with existing identities and label
    QHBoxLayout* hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mComboBox = new KComboBox( this );
    mComboBox->setEditable( false );

    mComboBox->addItems( manager->shadowIdentities() );
    mComboBox->setEnabled( false );
    QLabel *label = new QLabel( i18n("&Existing identities:"), this );
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
    resize(350, 130);
}

IdentityAddVcardDialog::~IdentityAddVcardDialog()
{
}

IdentityAddVcardDialog::DuplicateMode IdentityAddVcardDialog::duplicateMode() const
{
    const int id = mButtonGroup->checkedId();
    return static_cast<DuplicateMode>( id );
}

QString IdentityAddVcardDialog::duplicateVcardFromIdentity() const
{
    return mComboBox->currentText();
}

