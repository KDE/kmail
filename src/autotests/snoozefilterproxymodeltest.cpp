/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozefilterproxymodeltest.h"
#include "snooze/snoozefilterproxymodel.h"
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Item>
#include <MailCommon/SnoozeAttribute>
#include <QStandardItemModel>
#include <QTest>
QTEST_GUILESS_MAIN(SnoozeFilterProxyModelTest)

namespace
{
QStandardItem *appendItem(QStandardItemModel *model, Akonadi::Item::Id id, const QDateTime &wakeUpDateTime = {})
{
    Akonadi::Item item(id);
    if (wakeUpDateTime.isValid()) {
        item.attribute<MailCommon::SnoozeAttribute>(Akonadi::Item::AddIfMissing)->setWakeUpDateTime(wakeUpDateTime);
    }
    auto row = new QStandardItem;
    row->setData(QVariant::fromValue(item), Akonadi::EntityTreeModel::ItemRole);
    model->appendRow(row);
    return row;
}
}

SnoozeFilterProxyModelTest::SnoozeFilterProxyModelTest(QObject *parent)
    : QObject(parent)
{
}

void SnoozeFilterProxyModelTest::shouldHaveDefaultValue()
{
    SnoozeFilterProxyModel proxy;
    QVERIFY(!proxy.showSnoozed());
}

void SnoozeFilterProxyModelTest::shouldHideSnoozedMessages()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QStandardItemModel source;
    appendItem(&source, 1);
    appendItem(&source, 2, now.addDays(1));
    appendItem(&source, 3, now.addDays(-1));

    SnoozeFilterProxyModel proxy;
    proxy.setSourceModel(&source);
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(proxy.index(0, 0).data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>().id(), 1);
    QCOMPARE(proxy.index(1, 0).data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>().id(), 3);

    proxy.setShowSnoozed(true);
    QCOMPARE(proxy.rowCount(), 3);
    proxy.setShowSnoozed(false);
    QCOMPARE(proxy.rowCount(), 2);
}

void SnoozeFilterProxyModelTest::shouldShowMessageWhenSnoozeIsRemoved()
{
    QStandardItemModel source;
    QStandardItem *row = appendItem(&source, 1, QDateTime::currentDateTimeUtc().addDays(1));

    SnoozeFilterProxyModel proxy;
    proxy.setSourceModel(&source);
    QCOMPARE(proxy.rowCount(), 0);

    auto item = row->data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    item.removeAttribute<MailCommon::SnoozeAttribute>();

    row->setData({}, Akonadi::EntityTreeModel::ItemRole);
    row->setData(QVariant::fromValue(item), Akonadi::EntityTreeModel::ItemRole);
    QCOMPARE(proxy.rowCount(), 1);
}

#include "moc_snoozefilterproxymodeltest.cpp"
