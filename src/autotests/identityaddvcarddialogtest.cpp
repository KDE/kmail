/*
  Copyright (c) 2014-2020 Laurent Montel <montel@kde.org>

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

#include "identityaddvcarddialogtest.h"
#include "../identity/identityaddvcarddialog.h"

#include <QComboBox>
#include <KUrlRequester>
#include <QButtonGroup>

#include <QTest>

identityaddvcarddialogtest::identityaddvcarddialogtest()
{
}

void identityaddvcarddialogtest::shouldHaveDefaultValue()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    QComboBox *identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QVERIFY(identityComboBox);
    QCOMPARE(identityComboBox->isEnabled(), false);

    KUrlRequester *urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QVERIFY(urlRequester);
    QCOMPARE(urlRequester->isEnabled(), false);

    QButtonGroup *buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    QVERIFY(buttonGroup);
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::Empty);

    QVERIFY(buttonGroup->button(IdentityAddVcardDialog::Empty));
    QVERIFY(buttonGroup->button(IdentityAddVcardDialog::ExistingEntry));
    QVERIFY(buttonGroup->button(IdentityAddVcardDialog::FromExistingVCard));
}

void identityaddvcarddialogtest::shouldEnabledUrlRequesterWhenSelectFromExistingVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    QButtonGroup *buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(IdentityAddVcardDialog::FromExistingVCard)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::FromExistingVCard);

    QComboBox *identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QCOMPARE(identityComboBox->isEnabled(), false);

    KUrlRequester *urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QCOMPARE(urlRequester->isEnabled(), true);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    QButtonGroup *buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(IdentityAddVcardDialog::ExistingEntry)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::ExistingEntry);

    QComboBox *identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));
    QCOMPARE(identityComboBox->isEnabled(), true);

    KUrlRequester *urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));
    QCOMPARE(urlRequester->isEnabled(), false);
}

void identityaddvcarddialogtest::shouldEnabledComboboxWhenSelectFromExistingVCardAndAfterDuplicateVCard()
{
    IdentityAddVcardDialog dlg(QStringList(), nullptr);
    QButtonGroup *buttonGroup = dlg.findChild<QButtonGroup *>(QStringLiteral("buttongroup"));
    buttonGroup->button(IdentityAddVcardDialog::FromExistingVCard)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::FromExistingVCard);

    QComboBox *identityComboBox = dlg.findChild<QComboBox *>(QStringLiteral("identity_combobox"));

    KUrlRequester *urlRequester = dlg.findChild<KUrlRequester *>(QStringLiteral("kurlrequester_vcardpath"));

    buttonGroup->button(IdentityAddVcardDialog::ExistingEntry)->toggle();
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::ExistingEntry);
    QCOMPARE(identityComboBox->isEnabled(), true);
    QCOMPARE(urlRequester->isEnabled(), false);
}

QTEST_MAIN(identityaddvcarddialogtest)
