/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSortFilterProxyModel>

class SnoozeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SnoozeFilterProxyModel(QObject *parent = nullptr);
    ~SnoozeFilterProxyModel() override;

    [[nodiscard]] bool showSnoozed() const;
    void setShowSnoozed(bool newShowSnoozed);

    [[nodiscard]] bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
    bool mShowSnoozed = false;
};
