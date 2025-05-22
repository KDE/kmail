/*
    SPDX-FileCopyrightText: 2011 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentWidgetBase>

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <MailCommon/SearchPattern>

#include <Akonadi/AgentInstance>

class FilterLogDialog;
class FilterManager;
class KJob;
class DummyKernel;

namespace Akonadi
{
class Monitor;
}

class MailFilterAgent : public Akonadi::AgentWidgetBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit MailFilterAgent(const QString &id);
    ~MailFilterAgent() override;

    [[nodiscard]] QString createUniqueName(const QString &nameTemplate);
    void filterItems(const QList<qint64> &itemIds, int filterSet);

    void filterItem(qint64 item, int filterSet, const QString &resourceId);
    void filter(qint64 item, const QString &filterIdentifier, const QString &resourceId);
    void filterCollections(const QList<qint64> &collections, int filterSet);
    void applySpecificFilters(const QList<qint64> &itemIds, int requiresPart, const QStringList &listFilters);
    void applySpecificFiltersOnCollections(const QList<qint64> &colIds, const QStringList &listFilters, int filterSet);

    void reload();

    void showFilterLogDialog(qlonglong windowId = 0);
    [[nodiscard]] QString printCollectionMonitored() const;

    void expunge(qint64 collectionId);

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;

private:
    void initializeCollections();
    void initialCollectionFetchingDone(KJob *);
    void mailCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void mailCollectionChanged(const Akonadi::Collection &collection);
    void mailCollectionRemoved(const Akonadi::Collection &collection);
    void emitProgress(int percent = 0);
    void emitProgressMessage(const QString &message);
    void itemsReceivedForFiltering(const Akonadi::Item::List &items);
    void clearMessage();
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);
    void slotItemChanged(const Akonadi::Item &item);

public Q_SLOTS:
    void configure(WId windowId) override;

private:
    [[nodiscard]] bool isFilterableCollection(const Akonadi::Collection &collection) const;

    FilterManager *mFilterManager = nullptr;

    FilterLogDialog *mFilterLogDialog = nullptr;
    QTimer *const mProgressTimer;
    DummyKernel *mMailFilterKernel = nullptr;
    int mProgressCounter = 0;
    Akonadi::Monitor *mItemMonitor = nullptr;

    void filterItem(const Akonadi::Item &item, const Akonadi::Collection &collection);
};
