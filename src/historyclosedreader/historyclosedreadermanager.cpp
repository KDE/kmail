/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "historyclosedreadermanager.h"

HistoryClosedReaderManager::HistoryClosedReaderManager(QObject *parent)
    : QObject{parent}
{
}

HistoryClosedReaderManager::~HistoryClosedReaderManager() = default;

HistoryClosedReaderManager *HistoryClosedReaderManager::self()
{
    static HistoryClosedReaderManager s_self;
    return &s_self;
}

bool HistoryClosedReaderManager::isEmpty() const
{
    return mClosedReaderInfos.isEmpty();
}

void HistoryClosedReaderManager::addInfo(const HistoryClosedReaderInfo &info)
{
    if (info.isValid()) {
        if (!mClosedReaderInfos.isEmpty()) {
            auto infoIt = std::find_if(mClosedReaderInfos.cbegin(), mClosedReaderInfos.cend(), [&info](const HistoryClosedReaderInfo &i) {
                return i == info;
            });
            // Remove same element.
            if (infoIt != mClosedReaderInfos.cend()) {
                mClosedReaderInfos.removeAll(*infoIt);
            }
        }
        if (mClosedReaderInfos.count() >= 10) {
            mClosedReaderInfos.takeFirst();
        }
        mClosedReaderInfos.append(info);
        Q_EMIT historyClosedReaderChanged();
    }
}

HistoryClosedReaderInfo HistoryClosedReaderManager::lastInfo()
{
    if (mClosedReaderInfos.isEmpty()) {
        return {};
    }
    const auto lastElement = mClosedReaderInfos.takeLast();
    if (mClosedReaderInfos.isEmpty()) {
        Q_EMIT historyClosedReaderChanged();
    }
    return lastElement;
}

void HistoryClosedReaderManager::clear()
{
    if (mClosedReaderInfos.isEmpty()) {
        return;
    }
    mClosedReaderInfos.clear();
    Q_EMIT historyClosedReaderChanged();
}

void HistoryClosedReaderManager::removeItem(Akonadi::Item::Id id)
{
    if (mClosedReaderInfos.isEmpty()) {
        return;
    }
    auto infoIt = std::find_if(mClosedReaderInfos.cbegin(), mClosedReaderInfos.cend(), [&id](const HistoryClosedReaderInfo &info) {
        return info.item() == id;
    });
    if (infoIt != mClosedReaderInfos.cend()) {
        mClosedReaderInfos.removeAll(*infoIt);
        Q_EMIT historyClosedReaderChanged();
    }
}

int HistoryClosedReaderManager::count() const
{
    return mClosedReaderInfos.count();
}

QList<HistoryClosedReaderInfo> HistoryClosedReaderManager::closedReaderInfos() const
{
    return mClosedReaderInfos;
}

#include "moc_historyclosedreadermanager.cpp"
