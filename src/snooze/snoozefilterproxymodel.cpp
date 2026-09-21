/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "snoozefilterproxymodel.h"
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Item>
#include <MailCommon/SnoozeAttribute>

SnoozeFilterProxyModel::SnoozeFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
}

SnoozeFilterProxyModel::~SnoozeFilterProxyModel() = default;

bool SnoozeFilterProxyModel::showSnoozed() const
{
    return mShowSnoozed;
}

void SnoozeFilterProxyModel::setShowSnoozed(bool newShowSnoozed)
{
    if (mShowSnoozed != newShowSnoozed) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
#endif
        mShowSnoozed = newShowSnoozed;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
        invalidateFilter();
#endif
    }
}

bool SnoozeFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    if (mShowSnoozed) {
        return true;
    }
    const auto idx = sourceModel()->index(row, 0, parent);
    const auto item = idx.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    const auto *attr = item.attribute<MailCommon::SnoozeAttribute>();
    return !attr || attr->wakeUpDateTime() <= QDateTime::currentDateTimeUtc();
}
