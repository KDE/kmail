/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "followupreminderselectdatedialogtest.h"
#include "../followupreminder/followupreminderselectdatedialog.h"
#include <qtest.h>
#include <KDateComboBox>
#include <AkonadiWidgets/CollectionComboBox>

#include <QLineEdit>
#include <QPushButton>

FollowupReminderSelectDateDialogTest::FollowupReminderSelectDateDialogTest(QObject *parent)
    : QObject(parent)
{

}

FollowupReminderSelectDateDialogTest::~FollowupReminderSelectDateDialogTest()
{

}

void FollowupReminderSelectDateDialogTest::shouldHaveDefaultValue()
{
    FollowUpReminderSelectDateDialog dlg;
    KDateComboBox *datecombobox = dlg.findChild<KDateComboBox *>(QStringLiteral("datecombobox"));
    QVERIFY(datecombobox);

    Akonadi::CollectionComboBox *combobox = dlg.findChild<Akonadi::CollectionComboBox *>(QStringLiteral("collectioncombobox"));
    QVERIFY(combobox);
    QDate currentDate = QDate::currentDate();
    QCOMPARE(datecombobox->date(), currentDate.addDays(1));

    QPushButton *okButton = dlg.findChild<QPushButton *>(QStringLiteral("ok_button"));
    QVERIFY(okButton);
    QVERIFY(okButton->isEnabled());
}

void FollowupReminderSelectDateDialogTest::shouldDisableOkButtonIfDateIsEmpty()
{
    FollowUpReminderSelectDateDialog dlg;
    KDateComboBox *datecombobox = qFindChild<KDateComboBox *>(&dlg, QStringLiteral("datecombobox"));
    QVERIFY(datecombobox);
    QPushButton *okButton = dlg.findChild<QPushButton *>(QStringLiteral("ok_button"));
    QVERIFY(okButton->isEnabled());
    datecombobox->lineEdit()->clear();
    QVERIFY(!okButton->isEnabled());
}

QTEST_MAIN(FollowupReminderSelectDateDialogTest)
