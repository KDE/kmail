/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QMetaType>
#include <QSet>
#include <QString>

class KConfigGroup;
class UnifiedMailboxManager;

class UnifiedMailbox
{
    friend class UnifiedMailboxManager;

public:
    UnifiedMailbox() = default;
    UnifiedMailbox(UnifiedMailbox &&) = default;
    UnifiedMailbox &operator=(UnifiedMailbox &&) = default;

    UnifiedMailbox(const UnifiedMailbox &) = delete;
    UnifiedMailbox &operator=(const UnifiedMailbox &) = delete;

    /** Compares two boxes by their ID **/
    bool operator==(const UnifiedMailbox &other) const;

    void save(KConfigGroup &group) const;
    void load(const KConfigGroup &group);

    bool isSpecial() const;

    Q_REQUIRED_RESULT qint64 collectionId() const;
    void setCollectionId(qint64 id);

    Q_REQUIRED_RESULT QString id() const;
    void setId(const QString &id);

    Q_REQUIRED_RESULT QString name() const;
    void setName(const QString &name);

    Q_REQUIRED_RESULT QString icon() const;
    void setIcon(const QString &icon);

    void addSourceCollection(qint64 source);
    void removeSourceCollection(qint64 source);
    void setSourceCollections(const QSet<qint64> &sources);
    Q_REQUIRED_RESULT QSet<qint64> sourceCollections() const;

private:
    void attachManager(UnifiedMailboxManager *manager);

    qint64 mCollectionId = -1;
    QString mId;
    QString mName;
    QString mIcon;
    QSet<qint64> mSources;

    UnifiedMailboxManager *mManager = nullptr;
};

Q_DECLARE_METATYPE(UnifiedMailbox *)

