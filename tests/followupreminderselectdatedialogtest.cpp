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
#include <KDateComboBox>
#include <qtest_kde.h>
#include <Akonadi/CollectionComboBox>
#include <Akonadi/EntityTreeModel>
#include <QLineEdit>
#include <KPushButton>
#include <QStandardItemModel>
#include <KCalCore/Todo>

FollowupReminderSelectDateDialogTest::FollowupReminderSelectDateDialogTest(QObject *parent)
    : QObject(parent)
{

}

FollowupReminderSelectDateDialogTest::~FollowupReminderSelectDateDialogTest()
{

}

QStandardItemModel *FollowupReminderSelectDateDialogTest::defaultItemModel()
{
    QStandardItemModel *model = new QStandardItemModel;
    for (int id = 42; id < 51; ++id) {
        Akonadi::Collection collection(id);
        collection.setRights(Akonadi::Collection::AllRights);
        collection.setName(QString::number(id));
        collection.setContentMimeTypes(QStringList() << KCalCore::Todo::todoMimeType());

        QStandardItem *item = new QStandardItem(collection.name());
        item->setData(QVariant::fromValue(collection),
                      Akonadi::EntityTreeModel::CollectionRole);
        item->setData(QVariant::fromValue(collection.id()),
                      Akonadi::EntityTreeModel::CollectionIdRole);

        model->appendRow(item);
    }
    return model;
}


void FollowupReminderSelectDateDialogTest::shouldHaveDefaultValue()
{
    FollowUpReminderSelectDateDialog dlg(0, defaultItemModel());
    KDateComboBox *datecombobox = qFindChild<KDateComboBox *>(&dlg, QLatin1String("datecombobox"));
    QVERIFY(datecombobox);

    Akonadi::CollectionComboBox *combobox = qFindChild<Akonadi::CollectionComboBox *>(&dlg, QLatin1String("collectioncombobox"));
    QVERIFY(combobox);
    QDate currentDate = QDate::currentDate();
    QCOMPARE(datecombobox->date(), currentDate.addDays(1));
}

void FollowupReminderSelectDateDialogTest::shouldDisableOkButtonIfDateIsEmpty()
{
    FollowUpReminderSelectDateDialog dlg(0, defaultItemModel());
    KDateComboBox *datecombobox = qFindChild<KDateComboBox *>(&dlg, QLatin1String("datecombobox"));
    QVERIFY(datecombobox);
    datecombobox->lineEdit()->clear();
    QVERIFY(!dlg.button(KDialog::Ok)->isEnabled());
}

void FollowupReminderSelectDateDialogTest::shouldDisableOkButtonIfDateIsNotValid()
{
    FollowUpReminderSelectDateDialog dlg(0, defaultItemModel());
    KDateComboBox *datecombobox = qFindChild<KDateComboBox *>(&dlg, QLatin1String("datecombobox"));
    QVERIFY(datecombobox);
    datecombobox->lineEdit()->setText(QLatin1String(" "));
    QVERIFY(!dlg.button(KDialog::Ok)->isEnabled());
    const QDate date = QDate::currentDate();
    datecombobox->setDate(date);
    QVERIFY(dlg.button(KDialog::Ok)->isEnabled());
}

void FollowupReminderSelectDateDialogTest::shouldDisableOkButtonIfModelIsEmpty()
{
    FollowUpReminderSelectDateDialog dlg(0, new QStandardItemModel(0));
    KDateComboBox *datecombobox = qFindChild<KDateComboBox *>(&dlg, QLatin1String("datecombobox"));
    QVERIFY(datecombobox);
    datecombobox->lineEdit()->setText(QLatin1String(" "));
    QVERIFY(!dlg.button(KDialog::Ok)->isEnabled());
    const QDate date = QDate::currentDate();
    datecombobox->setDate(date);
    QVERIFY(!dlg.button(KDialog::Ok)->isEnabled());
}

QTEST_KDEMAIN(FollowupReminderSelectDateDialogTest, GUI)
