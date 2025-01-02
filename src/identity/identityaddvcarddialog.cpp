/*
  SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identityaddvcarddialog.h"
using namespace Qt::Literals::StringLiterals;

#include <KLocalizedString>
#include <KSeparator>
#include <KUrlRequester>
#include <QComboBox>

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

IdentityAddVcardDialog::IdentityAddVcardDialog(const QStringList &shadowIdentities, QWidget *parent)
    : QDialog(parent)
    , mButtonGroup(new QButtonGroup(this))
    , mComboBox(new QComboBox(this))
    , mVCardPath(new KUrlRequester(this))
{
    setWindowTitle(i18nc("@title:window", "Create own vCard"));
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto mainLayout = new QVBoxLayout(this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityAddVcardDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityAddVcardDialog::reject);
    setModal(true);

    auto mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);

    auto vlay = new QVBoxLayout(mainWidget);
    vlay->setContentsMargins({});

    mButtonGroup->setObjectName("buttongroup"_L1);

    // row 1: radio button
    auto radio = new QRadioButton(i18nc("@option:radio", "&With empty fields"), this);
    radio->setChecked(true);
    vlay->addWidget(radio);
    mButtonGroup->addButton(radio, static_cast<int>(Empty));

    // row 2: radio button
    auto fromExistingVCard = new QRadioButton(i18nc("@option:radio", "&From existing vCard"), this);
    vlay->addWidget(fromExistingVCard);
    mButtonGroup->addButton(fromExistingVCard, static_cast<int>(FromExistingVCard));

    // row 3: KUrlRequester
    auto hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout(hlay);

    mVCardPath->setObjectName("kurlrequester_vcardpath"_L1);
    mVCardPath->setMimeTypeFilters({QStringLiteral("text/vcard"), QStringLiteral("all/allfiles")});

    mVCardPath->setMode(KFile::LocalOnly | KFile::File);
    auto label = new QLabel(i18nc("@label:textbox", "&vCard path:"), this);
    label->setBuddy(mVCardPath);
    label->setEnabled(false);
    mVCardPath->setEnabled(false);
    hlay->addWidget(label);
    hlay->addWidget(mVCardPath);

    connect(fromExistingVCard, &QRadioButton::toggled, label, &QLabel::setEnabled);
    connect(fromExistingVCard, &QRadioButton::toggled, mVCardPath, &KUrlRequester::setEnabled);

    // row 4: radio button
    auto duplicateExistingVCard = new QRadioButton(i18nc("@option:radio", "&Duplicate existing vCard"), this);
    vlay->addWidget(duplicateExistingVCard);
    mButtonGroup->addButton(duplicateExistingVCard, static_cast<int>(ExistingEntry));

    // row 5: combobox with existing identities and label
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout(hlay);
    mComboBox->setObjectName("identity_combobox"_L1);
    mComboBox->setEditable(false);

    mComboBox->addItems(shadowIdentities);
    mComboBox->setEnabled(false);
    label = new QLabel(i18nc("@label:textbox", "&Existing identities:"), this);
    label->setBuddy(mComboBox);
    label->setEnabled(false);
    hlay->addWidget(label);
    hlay->addWidget(mComboBox, 1);

    vlay->addWidget(new KSeparator(this));
    vlay->addStretch(1); // spacer

    // enable/disable combobox and label depending on the third radio
    // button's state:
    connect(duplicateExistingVCard, &QRadioButton::toggled, label, &QLabel::setEnabled);
    connect(duplicateExistingVCard, &QRadioButton::toggled, mComboBox, &QComboBox::setEnabled);
    resize(350, 130);
}

IdentityAddVcardDialog::~IdentityAddVcardDialog() = default;

IdentityAddVcardDialog::DuplicateMode IdentityAddVcardDialog::duplicateMode() const
{
    const int id = mButtonGroup->checkedId();
    return static_cast<DuplicateMode>(id);
}

QString IdentityAddVcardDialog::duplicateVcardFromIdentity() const
{
    return mComboBox->currentText();
}

QUrl IdentityAddVcardDialog::existingVCard() const
{
    return mVCardPath->url();
}

#include "moc_identityaddvcarddialog.cpp"
