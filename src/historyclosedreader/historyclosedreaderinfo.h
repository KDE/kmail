/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <Akonadi/Item>
#include <QDebug>
#include <QString>

class HistoryClosedReaderInfo
{
public:
    HistoryClosedReaderInfo();
    ~HistoryClosedReaderInfo();

    [[nodiscard]] QString subject() const;
    void setSubject(const QString &newSubject);

    [[nodiscard]] Akonadi::Item::Id item() const;
    void setItem(Akonadi::Item::Id newItem);

    [[nodiscard]] bool isValid() const;

private:
    QString mSubject;
    Akonadi::Item::Id mItem;
    // TODO add Item::Collection ?
};
Q_DECLARE_TYPEINFO(HistoryClosedReaderInfo, Q_MOVABLE_TYPE);
QDebug operator<<(QDebug d, const HistoryClosedReaderInfo &t);
