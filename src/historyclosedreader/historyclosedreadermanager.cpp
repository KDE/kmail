/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/
#include "historyclosedreadermanager.h"

HistoryClosedReaderManager *HistoryClosedReaderManager::self()
{
    static HistoryClosedReaderManager s_self;
    return &s_self;
}

HistoryClosedReaderManager::HistoryClosedReaderManager(QObject *parent)
    : QObject{parent}
{
}

HistoryClosedReaderManager::~HistoryClosedReaderManager() = default;

bool HistoryClosedReaderManager::isEmpty() const
{
    return mClosedReaderInfos.isEmpty();
}

void HistoryClosedReaderManager::addInfo(const HistoryClosedReaderInfo &info)
{
    if (info.isValid()) {
        mClosedReaderInfos.append(info);
        Q_EMIT historyClosedReaderChanged();
    }
}

HistoryClosedReaderInfo HistoryClosedReaderManager::lastInfo() const
{
    if (mClosedReaderInfos.isEmpty()) {
        return {};
    }
    return mClosedReaderInfos.last();
}

void HistoryClosedReaderManager::clear()
{
    mClosedReaderInfos.clear();
    Q_EMIT historyClosedReaderChanged();
}

#include "moc_historyclosedreadermanager.cpp"
