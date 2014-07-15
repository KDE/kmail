/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include <KComboBox>
#include <KUrlRequester>
#include <QButtonGroup>

#include <qtest_kde.h>

identityaddvcarddialogtest::identityaddvcarddialogtest()
{
}

void identityaddvcarddialogtest::shouldHaveDefaultValue()
{
    IdentityAddVcardDialog dlg(0,0);
    KComboBox *identityComboBox = qFindChild<KComboBox *>(&dlg, QLatin1String("identity_combobox"));
    QVERIFY(identityComboBox);
    QCOMPARE(identityComboBox->isEnabled(), false);

    KUrlRequester *urlRequester = qFindChild<KUrlRequester *>(&dlg, QLatin1String("kurlrequester_vcardpath"));
    QVERIFY(urlRequester);
    QCOMPARE(urlRequester->isEnabled(), false);

    QButtonGroup *buttonGroup = qFindChild<QButtonGroup *>(&dlg, QLatin1String("buttongroup"));
    QVERIFY(buttonGroup);
    QCOMPARE(dlg.duplicateMode(), IdentityAddVcardDialog::Empty);
}

QTEST_KDEMAIN(identityaddvcarddialogtest, GUI)
