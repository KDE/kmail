/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once
#include "kmail_private_export.h"
#include <Akonadi/Item>
#include <QDebug>
#include <QString>

class KMAILTESTS_TESTS_EXPORT HistoryClosedReaderInfo
{
public:
    HistoryClosedReaderInfo();
    ~HistoryClosedReaderInfo();

    [[nodiscard]] QString subject() const;
    void setSubject(const QString &newSubject);

    [[nodiscard]] Akonadi::Item::Id item() const;
    void setItem(Akonadi::Item::Id newItem);

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] bool operator==(const HistoryClosedReaderInfo &other) const;

private:
    QString mSubject;
    Akonadi::Item::Id mItem;
    // TODO add Item::Collection ?
};
Q_DECLARE_TYPEINFO(HistoryClosedReaderInfo, Q_MOVABLE_TYPE);
QDebug operator<<(QDebug d, const HistoryClosedReaderInfo &t);
