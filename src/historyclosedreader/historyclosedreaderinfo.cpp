/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
    SPDX-License-Identifier: GPL-2.0-only
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
