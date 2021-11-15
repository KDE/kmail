/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionswitchermodel.h"

CollectionSwitcherModel::CollectionSwitcherModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

CollectionSwitcherModel::~CollectionSwitcherModel()
{
}

int CollectionSwitcherModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0; // flat model
    }
    return mCollectionsInfo.count();
}

QVariant CollectionSwitcherModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= mCollectionsInfo.count()) {
        return {};
    }
    const CollectionInfo collectionInfo = mCollectionsInfo.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case CollectionFullPath:
        return collectionInfo.mFullPath;
    case CollectionAkonadId:
        return collectionInfo.mNewCollection.id();
    }
    return {};
}

void CollectionSwitcherModel::addHistory(const Akonadi::Collection &currentCol, const QString &fullPath)
{
    mCollectionsInfo.append({currentCol, fullPath});
    qDebug() << " void CollectionSwitcherModel::addHistory(const Akonadi::Collection &currentCol, const QString &fullPath) " << fullPath;
    qDebug() << " mCollectionsInfo " << mCollectionsInfo.count();
}

const Akonadi::Collection CollectionSwitcherModel::collection(int index)
{
    if (index < 0 || index >= mCollectionsInfo.count()) {
        return {};
    }
    return mCollectionsInfo.at(index).mNewCollection;
}
