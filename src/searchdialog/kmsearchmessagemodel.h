/*
 * SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once

#include <Akonadi/KMime/MessageModel>
#include <QHash>

class KMSearchMessageModel : public Akonadi::MessageModel
{
    Q_OBJECT

public:
    enum Column {
        Collection,
        Subject,
        Sender,
        Receiver,
        Date,
        Size,
    };
    explicit KMSearchMessageModel(Akonadi::Monitor *monitor, QObject *parent = nullptr);
    ~KMSearchMessageModel() override;

protected:
    int entityColumnCount(HeaderGroup headerGroup) const override;
    QVariant entityData(const Akonadi::Item &item, int column, int role = Qt::DisplayRole) const override;
    QVariant entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const override;

private:
    Q_REQUIRED_RESULT QString fullCollectionPath(Akonadi::Collection::Id id) const;

    mutable QHash<Akonadi::Collection::Id, QString> m_collectionFullPathCache;
};

