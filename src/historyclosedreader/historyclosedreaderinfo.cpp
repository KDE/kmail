/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#include "historyclosedreaderinfo.h"

HistoryClosedReaderInfo::HistoryClosedReaderInfo()
{
}

HistoryClosedReaderInfo::~HistoryClosedReaderInfo()
{
}

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

QDebug operator<<(QDebug d, const HistoryClosedReaderInfo &t)
{
    d << " mSubject " << t.subject();
    d << " mItem " << t.item();
    return d;
}
