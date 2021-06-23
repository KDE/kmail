/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "utils.h"

#include <QObject>
#include <QSet>
#include <QSettings>

#include <KSharedConfig>

#include <AkonadiCore/ChangeRecorder>

#include <functional>
#include <memory>
#include <unordered_map>

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

    Q_REQUIRED_RESULT static bool isUnifiedMailbox(const Akonadi::Collection &col);

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
    std::unordered_map<qint64, UnifiedMailbox *> mSourceToBoxMap;

    Akonadi::ChangeRecorder mMonitor;
    QSettings mMonitorSettings;

    KSharedConfigPtr mConfig;
};
