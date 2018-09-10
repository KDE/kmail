/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef UNIFIEDMAILBOX_H
#define UNIFIEDMAILBOX_H

#include <QString>
#include <QMetaType>
#include <QSet>

#include "utils.h"

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

    stdx::optional<qint64> collectionId() const;
    void setCollectionId(qint64 id);

    QString id() const;
    void setId(const QString &id);

    QString name() const;
    void setName(const QString &name);

    QString icon() const;
    void setIcon(const QString &icon);

    void addSourceCollection(qint64 source);
    void removeSourceCollection(qint64 source);
    void setSourceCollections(const QSet<qint64> &sources);
    QSet<qint64> sourceCollections() const;

private:
    void attachManager(UnifiedMailboxManager *manager);

    stdx::optional<qint64> mCollectionId;
    QString mId;
    QString mName;
    QString mIcon;
    QSet<qint64> mSources;

    UnifiedMailboxManager *mManager = nullptr;
};

Q_DECLARE_METATYPE(UnifiedMailbox*)


#endif
