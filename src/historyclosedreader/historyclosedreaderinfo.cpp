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

QDebug operator<<(QDebug d, const HistoryClosedReaderInfo &t)
{
    d << " mSubject " << t.subject();
    return d;
}
