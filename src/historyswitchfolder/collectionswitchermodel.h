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
    enum CollectionInfoRole {
        CollectionFullPath = Qt::UserRole + 1,
        CollectionAkonadId,
    };

    struct CollectionInfo {
        CollectionInfo() = default;
        CollectionInfo(const Akonadi::Collection &col, const QString &path)
            : mNewCollection(col)
            , mFullPath(path)
        {
        }

        const Akonadi::Collection mNewCollection;
        const QString mFullPath;
    };

    explicit CollectionSwitcherModel(QObject *parent = nullptr);
    ~CollectionSwitcherModel() override;

    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override;

    void addHistory(const Akonadi::Collection &currentCol, const QString &fullPath = {});

    Q_REQUIRED_RESULT const Akonadi::Collection collection(int index);

private:
    QVector<CollectionInfo> mCollectionsInfo;
};

Q_DECLARE_METATYPE(CollectionSwitcherModel::CollectionInfo)
Q_DECLARE_TYPEINFO(CollectionSwitcherModel::CollectionInfo, Q_MOVABLE_TYPE);
