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

#ifndef UNIFIEDBOXMANAGER_H
#define UNIFIEDBOXMANAGER_H

#include <QObject>
#include <QSet>

#include <KSharedConfig>

#include <unordered_map>
#include <functional>

class UnifiedMailboxManager;
class UnifiedMailbox {
    friend class UnifiedMailboxManager;
public:
    UnifiedMailbox() = default;
    UnifiedMailbox(UnifiedMailbox &&) = default;
    UnifiedMailbox(const UnifiedMailbox &) = default;
    UnifiedMailbox &operator=(const UnifiedMailbox &) = default;
    UnifiedMailbox &operator=(UnifiedMailbox &&) = default;

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
    QString mId;
    QString mName;
    QString mIcon;
    QSet<qint64> mSources;
};

Q_DECLARE_METATYPE(UnifiedMailbox)


class UnifiedMailboxManager : public QObject
{
    Q_OBJECT
public:
    using LoadCallback = std::function<void()>;

    explicit UnifiedMailboxManager(KSharedConfigPtr config, QObject *parent = nullptr);
    ~UnifiedMailboxManager() override;

    void loadBoxes(LoadCallback &&cb);
    void saveBoxes();

    void insertBox(UnifiedMailbox box);
    void removeBox(const QString &name);

    // FIXME: std::optional :-(
    const UnifiedMailbox *unifiedMailboxForSource(qint64 source) const;
    qint64 collectionIdForUnifiedMailbox(const QString &id) const;

    inline QHash<QString, UnifiedMailbox>::const_iterator begin() const
    {
        return mBoxes.begin();
    }

    inline QHash<QString, UnifiedMailbox>::const_iterator end() const
    {
        return mBoxes.end();
    }

    void discoverBoxCollections(LoadCallback &&cb);

Q_SIGNALS:
    void boxesChanged();

private:
    void discoverDefaultBoxes(LoadCallback &&cb);
    void loadBoxesFromConfig(LoadCallback &&cb);

    QHash<QString, UnifiedMailbox> mBoxes;
    QHash<QString, qint64> mBoxId;

    KSharedConfigPtr mConfig;
};
#endif
