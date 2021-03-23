/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::Empty);

    QVERIFY(buttonGroup->button(IdentityAddVcardDialog::Empty));
    QVERIFY(buttonGroup->button(IdentityAddVcardDialog::ExistingEntry));
    QVERIFY(buttonGroup->button(IdentityAddVcardDialog::FromExistingVCard));
}

void identityaddvcarddialogtest::shouldEnabledUrlRequesterWhenSelectFromExistingVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(IdentityAddVcardDialog::FromExistingVCard)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::FromExistingVCard);

    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QCOMPARE(identityComboBox->isEnabled(), false);

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QCOMPARE(urlRequester->isEnabled(), true);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(IdentityAddVcardDialog::ExistingEntry)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::ExistingEntry);

    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QCOMPARE(identityComboBox->isEnabled(), true);

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QCOMPARE(urlRequester->isEnabled(), false);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectFromExistingVCardAndAfterDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(IdentityAddVcardDialog::FromExistingVCard)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::FromExistingVCard);

    auto identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));

    auto urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));

    buttonGroup->button(IdentityAddVcardDialog::ExistingEntry)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::ExistingEntry);
    QCOMPARE(identityComboBox->isEnabled(), true);
    QCOMPARE(urlRequester->isEnabled(), false);
}

QTEST_MAIN(identityaddvcarddialogtest)
