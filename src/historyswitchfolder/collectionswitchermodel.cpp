/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionswitchermodel.h"

CollectionSwitcherModel::CollectionSwitcherModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

CollectionSwitcherModel::~CollectionSwitcherModel() = default;

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
    const CollectionInfo info{currentCol, fullPath};
    if (!mCollectionsInfo.isEmpty()) {
        if (mCollectionsInfo.at(mCollectionsInfo.count() - 1) == info) {
            return;
        }
    }
    if (mCollectionsInfo.count() > 10) {
        mCollectionsInfo.takeFirst();
    }
    mCollectionsInfo.removeAll(info);
    mCollectionsInfo.prepend(info);
    Q_EMIT dataChanged(createIndex(0, 0), createIndex(mCollectionsInfo.count() - 1, 1), {});
}

const Akonadi::Collection CollectionSwitcherModel::collection(int index)
{
    if (index < 0 || index >= mCollectionsInfo.count()) {
        return {};
    }
    return mCollectionsInfo.at(index).mNewCollection;
}

bool CollectionSwitcherModel::CollectionInfo::operator==(const CollectionInfo &other) const
{
    return other.mNewCollection == mNewCollection && other.mFullPath == mFullPath;
}

#include "moc_collectionswitchermodel.cpp"
