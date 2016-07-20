/*
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

#include <KIdentityManagement/kidentitymanagement/identitymanager.h>

#include <PimCommon/PimUtil>
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
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <KHelpClient>
#include <QPushButton>

using namespace KMail;

NewIdentityDialog::NewIdentityDialog(KIdentityManagement::IdentityManager *manager, QWidget *parent)
    : QDialog(parent),
      mIdentityManager(manager)
{
    setWindowTitle(i18n("New Identity"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewIdentityDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewIdentityDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &NewIdentityDialog::slotHelp);

    QWidget *page = new QWidget(this);
    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);
    QVBoxLayout *vlay = new QVBoxLayout(page);
    vlay->setMargin(0);

    // row 0: line edit with label
    QHBoxLayout *hlay = new QHBoxLayout();  // inherits spacing
    vlay->addLayout(hlay);
    mLineEdit = new KLineEdit(page);
    mLineEdit->setFocus();
    mLineEdit->setClearButtonShown(true);
    QLabel *l = new QLabel(i18n("&New identity:"), page);
    l->setBuddy(mLineEdit);
    hlay->addWidget(l);
    hlay->addWidget(mLineEdit, 1);
    connect(mLineEdit, &KLineEdit::textChanged, this, &NewIdentityDialog::slotEnableOK);

    mButtonGroup = new QButtonGroup(page);

    // row 1: radio button
    QRadioButton *radio = new QRadioButton(i18n("&With empty fields"), page);
    radio->setChecked(true);
    vlay->addWidget(radio);
    mButtonGroup->addButton(radio, (int)Empty);

    // row 2: radio button
    radio = new QRadioButton(i18n("&Use System Settings values"), page);
    vlay->addWidget(radio);
    mButtonGroup->addButton(radio, (int)ControlCenter);

    // row 3: radio button
    radio = new QRadioButton(i18n("&Duplicate existing identity"), page);
    vlay->addWidget(radio);
    mButtonGroup->addButton(radio, (int)ExistingEntry);

    // row 4: combobox with existing identities and label
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout(hlay);
    mComboBox = new KComboBox(page);
    mComboBox->setEditable(false);
    mComboBox->addItems(manager->shadowIdentities());
    mComboBox->setEnabled(false);
    QLabel *label = new QLabel(i18n("&Existing identities:"), page);
    label->setBuddy(mComboBox);
    label->setEnabled(false);
    hlay->addWidget(label);
    hlay->addWidget(mComboBox, 1);

    vlay->addWidget(new KSeparator);
    vlay->addStretch(1);   // spacer

    // enable/disable combobox and label depending on the third radio
    // button's state:
    connect(radio, &QRadioButton::toggled, label, &QLabel::setEnabled);
    connect(radio, &QRadioButton::toggled, mComboBox, &KComboBox::setEnabled);

    mOkButton->setEnabled(false);   // since line edit is empty

    resize(400, 180);
}

void NewIdentityDialog::slotHelp()
{
    PimCommon::Util::invokeHelp(QStringLiteral("kmail/configure-identity.html"), QStringLiteral("configure-identity-newidentitydialog"));
}

NewIdentityDialog::DuplicateMode NewIdentityDialog::duplicateMode() const
{
    const int id = mButtonGroup->checkedId();
    assert(id == (int)Empty
           || id == (int)ControlCenter
           || id == (int)ExistingEntry);
    return static_cast<DuplicateMode>(id);
}

void NewIdentityDialog::slotEnableOK(const QString &proposedIdentityName)
{
    // OK button is disabled if
    const QString name = proposedIdentityName.trimmed();
    // name isn't empty
    if (name.isEmpty()) {
        mOkButton->setEnabled(false);
        return;
    }
    // or name doesn't yet exist.
    if (!mIdentityManager->isUnique(name)) {
        mOkButton->setEnabled(false);
        return;
    }
    mOkButton->setEnabled(true);
}

QString NewIdentityDialog::identityName() const
{
    return mLineEdit->text();
}

QString NewIdentityDialog::duplicateIdentity() const
{
    return mComboBox->currentText();
}

