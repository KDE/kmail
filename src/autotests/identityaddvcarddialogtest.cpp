/*
  SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "identityaddvcarddialogtest.h"
#include "../identity/identityaddvcarddialog.h"

#include <KUrlRequester>
#include <QButtonGroup>
#include <QComboBox>

#include <QTest>
using namespace Qt::Literals::StringLiterals;

QTEST_MAIN(identityaddvcarddialogtest)
identityaddvcarddialogtest::identityaddvcarddialogtest(QObject *parent)
    : QObject(parent)
{
}

void identityaddvcarddialogtest::shouldHaveDefaultValue()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto identityComboBox = dlg.findChild<QComboBox *>(u"identity_combobox"_s);
    QVERIFY(identityComboBox);
    QCOMPARE(identityComboBox->isEnabled(), false);

    auto urlRequester = dlg.findChild<KUrlRequester *>(u"kurlrequester_vcardpath"_s);
    QVERIFY(urlRequester);
    QCOMPARE(urlRequester->isEnabled(), false);

    auto buttonGroup = dlg.findChild<QButtonGroup *>(u"buttongroup"_s);
    QVERIFY(buttonGroup);
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::Empty);

    QVERIFY(buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::Empty)));
    QVERIFY(buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::ExistingEntry)));
    QVERIFY(buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::FromExistingVCard)));
}

void identityaddvcarddialogtest::shouldEnabledUrlRequesterWhenSelectFromExistingVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(u"buttongroup"_s);
    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::FromExistingVCard))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::FromExistingVCard);

    auto identityComboBox = dlg.findChild<QComboBox *>(u"identity_combobox"_s);
    QCOMPARE(identityComboBox->isEnabled(), false);

    auto urlRequester = dlg.findChild<KUrlRequester *>(u"kurlrequester_vcardpath"_s);
    QCOMPARE(urlRequester->isEnabled(), true);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(u"buttongroup"_s);
    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::ExistingEntry))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::ExistingEntry);

    auto identityComboBox = dlg.findChild<QComboBox *>(u"identity_combobox"_s);
    QCOMPARE(identityComboBox->isEnabled(), true);

    auto urlRequester = dlg.findChild<KUrlRequester *>(u"kurlrequester_vcardpath"_s);
    QCOMPARE(urlRequester->isEnabled(), false);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectFromExistingVCardAndAfterDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    auto buttonGroup = dlg.findChild<QButtonGroup *>(u"buttongroup"_s);
    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::FromExistingVCard))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::FromExistingVCard);

    auto identityComboBox = dlg.findChild<QComboBox *>(u"identity_combobox"_s);

    auto urlRequester = dlg.findChild<KUrlRequester *>(u"kurlrequester_vcardpath"_s);

    buttonGroup->button(static_cast<int>(IdentityAddVcardDialog::DuplicateMode::ExistingEntry))->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::DuplicateMode::ExistingEntry);
    QCOMPARE(identityComboBox->isEnabled(), true);
    QCOMPARE(urlRequester->isEnabled(), false);
}

#include "moc_identityaddvcarddialogtest.cpp"
