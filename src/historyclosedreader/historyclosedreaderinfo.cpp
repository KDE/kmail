/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "historyclosedreaderinfo.h"

HistoryClosedReaderInfo::HistoryClosedReaderInfo() = default;

HistoryClosedReaderInfo::~HistoryClosedReaderInfo() = default;

QString HistoryClosedReaderInfo::subject() const
{
    return mSubject;
}

void HistoryClosedReaderInfo::setSubject(const QString &newSubject)
{
    mSubject = newSubject;
}

Akonadi::Item::Id HistoryClosedReaderInfo::item() const
{
    return mItem;
}

void HistoryClosedReaderInfo::setItem(Akonadi::Item::Id newItem)
{
    mItem = newItem;
}

bool HistoryClosedReaderInfo::isValid() const
{
    return mItem != -1;
}

bool HistoryClosedReaderInfo::operator==(const HistoryClosedReaderInfo &other) const
{
    return other.item() == mItem && other.subject() == mSubject;
}

QDebug operator<<(QDebug d, const HistoryClosedReaderInfo &t)
{
    d << " mSubject " << t.subject();
    d << " mItem " << t.item();
    return d;
}
