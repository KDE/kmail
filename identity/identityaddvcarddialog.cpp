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

#include <KComboBox>
#include <KLocalizedString>
#include <KSeparator>
#include <KUrlRequester>

#include <QButtonGroup>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>


IdentityAddVcardDialog::IdentityAddVcardDialog(const QStringList &shadowIdentities, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle( i18n( "Create own vCard" ) );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    okButton->setDefault(true);
    setModal( true );

    QWidget *mainWidget = new QWidget( this );
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);

    QVBoxLayout *vlay = new QVBoxLayout( mainWidget );
//TODO PORT QT5     vlay->setSpacing( QDialog::spacingHint() );
//TODO PORT QT5     vlay->setMargin( QDialog::marginHint() );
//PORTING: Verify that widget was added to mainLayout     setMainWidget( mainWidget );

    mButtonGroup = new QButtonGroup( this );
    mButtonGroup->setObjectName(QLatin1String("buttongroup"));

    // row 1: radio button
    QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), this );
    radio->setChecked( true );
    vlay->addWidget( radio );
    mButtonGroup->addButton( radio, (int)Empty );

    // row 2: radio button
    QRadioButton *fromExistingVCard = new QRadioButton( i18n("&From existing vCard"), this );
    vlay->addWidget( fromExistingVCard );
    mButtonGroup->addButton( fromExistingVCard, (int)FromExistingVCard );

    // row 3: KUrlRequester
    QHBoxLayout* hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );


    mVCardPath = new KUrlRequester;
    mVCardPath->setObjectName(QLatin1String("kurlrequester_vcardpath"));
    const QString filter = i18n( "*.vcf|vCard (*.vcf)\n*|all files (*)" );
    mVCardPath->setFilter(filter);

    mVCardPath->setMode(KFile::LocalOnly|KFile::File);
    QLabel *label = new QLabel( i18n("&VCard path:"), this );
    label->setBuddy( mVCardPath );
    label->setEnabled( false );
    mVCardPath->setEnabled( false );
    hlay->addWidget( label );
    hlay->addWidget( mVCardPath );

    connect( fromExistingVCard, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( fromExistingVCard, SIGNAL(toggled(bool)),
             mVCardPath, SLOT(setEnabled(bool)) );


    // row 4: radio button
    QRadioButton *duplicateExistingVCard = new QRadioButton( i18n("&Duplicate existing vCard"), this );
    vlay->addWidget( duplicateExistingVCard );
    mButtonGroup->addButton( duplicateExistingVCard, (int)ExistingEntry );

    // row 5: combobox with existing identities and label
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mComboBox = new KComboBox( this );
    mComboBox->setObjectName(QLatin1String("identity_combobox"));
    mComboBox->setEditable( false );

    mComboBox->addItems( shadowIdentities );
    mComboBox->setEnabled( false );
    label = new QLabel( i18n("&Existing identities:"), this );
    label->setBuddy( mComboBox );
    label->setEnabled( false );
    hlay->addWidget( label );
    hlay->addWidget( mComboBox, 1 );

    vlay->addWidget(new KSeparator);
    vlay->addStretch( 1 ); // spacer

    // enable/disable combobox and label depending on the third radio
    // button's state:
    connect( duplicateExistingVCard, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( duplicateExistingVCard, SIGNAL(toggled(bool)),
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

QUrl IdentityAddVcardDialog::existingVCard() const
{
    return mVCardPath->url();
}

