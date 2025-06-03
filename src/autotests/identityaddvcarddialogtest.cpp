/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "identityaddvcarddialogtest.h"
#include "../identity/identityaddvcarddialog.h"

#include <KUrlRequester>
#include <QButtonGroup>
#include <QComboBox>

#include <QTest>

identityaddvcarddialogtest::identityaddvcarddialogtest(QObject *parent)
    : QObject(parent)
{
}

void identityaddvcarddialogtest::shouldHaveDefaultValue()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QVERIFY(identityComboBox);
    QCOMPARE(identityComboBox->isEnabled(), false);

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QVERIFY(urlRequester);
    QCOMPARE(urlRequester->isEnabled(), false);

    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    QVERIFY(buttonGroup);
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::Empty);

    QVERIFY(buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::Empty)));
    QVERIFY(buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::ExistingEntry)));
    QVERIFY(buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::FromExistingVCard)));
}

void identityaddvcarddialogtest::shouldEnabledUrlRequesterWhenSelectFromExistingVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::FromExistingVCard))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::FromExistingVCard);

    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QCOMPARE(identityComboBox->isEnabled(), false);

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QCOMPARE(urlRequester->isEnabled(), true);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::ExistingEntry))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::ExistingEntry);

    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QCOMPARE(identityComboBox->isEnabled(), true);

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QCOMPARE(urlRequester->isEnabled(), false);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectFromExistingVCardAndAfterDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::FromExistingVCard))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::FromExistingVCard);

    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));

    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::ExistingEntry))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::ExistingEntry);
    QCOMPARE(identityComboBox->isEnabled(), true);
    QCOMPARE(urlRequester->isEnabled(), false);
}

QTEST_MAIN(identityaddvcarddialogtest)

#include "moc_identityaddvcarddialogtest.cpp"
