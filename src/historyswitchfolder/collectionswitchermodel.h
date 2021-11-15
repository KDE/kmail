/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once
#include <Akonadi/Collection>
#include <QAbstractListModel>
#include <QVector>

class CollectionSwitcherModel : public QAbstractListModel
{
    Q_OBJECT
public:
    struct CollectionInfo {
        const Akonadi::Collection mNewCollection;
        QString mFullPath;
    };

    explicit CollectionSwitcherModel(QObject *parent = nullptr);
    ~CollectionSwitcherModel() override;

    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override;

private:
    QVector<CollectionInfo> mCollectionsInfo;
};

Q_DECLARE_METATYPE(CollectionSwitcherModel::CollectionInfo)
Q_DECLARE_TYPEINFO(CollectionSwitcherModel::CollectionInfo, Q_MOVABLE_TYPE);
