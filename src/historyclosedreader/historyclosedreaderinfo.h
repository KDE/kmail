/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QDebug>
#include <QString>

class HistoryClosedReaderInfo
{
public:
    HistoryClosedReaderInfo();
    ~HistoryClosedReaderInfo();

    Q_REQUIRED_RESULT QString subject() const;
    void setSubject(const QString &newSubject);

private:
    QString mSubject;
    // TODO add Item::Id
    // TODO add Item::Collection ?
};

QDebug operator<<(QDebug d, const HistoryClosedReaderInfo &t);
