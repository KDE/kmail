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

#include "utils.h"

#include <QObject>
#include <QSet>
#include <QSettings>

#include <KSharedConfig>

#include <AkonadiCore/ChangeRecorder>

#include <unordered_map>
#include <functional>
#include <memory>

class UnifiedMailbox;
class UnifiedMailboxManager : public QObject
{
    Q_OBJECT
    friend class UnifiedMailbox;
public:
    using FinishedCallback = std::function<void()>;
    using Entry = std::pair<const QString, std::unique_ptr<UnifiedMailbox>>;

    explicit UnifiedMailboxManager(const KSharedConfigPtr &config, QObject *parent = nullptr);
    ~UnifiedMailboxManager() override;

    void loadBoxes(FinishedCallback &&cb = {});
    void saveBoxes();
    void discoverBoxCollections(FinishedCallback &&cb = {});

    void insertBox(std::unique_ptr<UnifiedMailbox> box);
    void removeBox(const QString &id);

    UnifiedMailbox *unifiedMailboxForSource(qint64 source) const;
    UnifiedMailbox *unifiedMailboxFromCollection(const Akonadi::Collection &col) const;

    inline auto begin() const
    {
        return mMailboxes.begin();
    }

    inline auto end() const
    {
        return mMailboxes.end();
    }

    static bool isUnifiedMailbox(const Akonadi::Collection &col);

    // Internal change recorder, for unittests
    Akonadi::ChangeRecorder &changeRecorder();
Q_SIGNALS:
    void updateBox(const UnifiedMailbox *box);

private:
    void createDefaultBoxes(FinishedCallback &&cb);

    const UnifiedMailbox *unregisterSpecialSourceCollection(qint64 colId);
    const UnifiedMailbox *registerSpecialSourceCollection(const Akonadi::Collection &col);

    // Using std::unique_ptr because QScopedPointer is not movable
    // Using std::unordered_map because Qt containers do not support movable-only types,
    std::unordered_map<QString, std::unique_ptr<UnifiedMailbox>> mMailboxes;
    std::unordered_map<qint64, UnifiedMailbox*> mSourceToBoxMap;

    Akonadi::ChangeRecorder mMonitor;
    QSettings mMonitorSettings;

    KSharedConfigPtr mConfig;
};
#endif
