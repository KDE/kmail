/*
    Copyright (c) 2011 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef MAILFILTERAGENT_H
#define MAILFILTERAGENT_H

#include <AkonadiAgentBase/agentbase.h>

#include "MailCommon/SearchPattern"
#include <Collection>
#include <AkonadiCore/item.h>

#include <AkonadiCore/AgentInstance>

class FilterLogDialog;
class FilterManager;
class KJob;
class DummyKernel;

namespace Akonadi {
class Monitor;
}

class MailFilterAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit MailFilterAgent(const QString &id);
    ~MailFilterAgent();

    QString createUniqueName(const QString &nameTemplate);
    void filterItems(const QList< qint64 > &itemIds, int filterSet);

    void filterItem(qint64 item, int filterSet, const QString &resourceId);
    void filter(qint64 item, const QString &filterIdentifier, const QString &resourceId);
    void filterCollections(const QList<qint64> &collections, int filterSet);
    void applySpecificFilters(const QList< qint64 > &itemIds, int requires, const QStringList &listFilters);
    void applySpecificFiltersOnCollections(const QList<qint64> &colIds, const QStringList &listFilters, int filterSet);

    void reload();

    void showFilterLogDialog(qlonglong windowId = 0);
    QString printCollectionMonitored();

    void showConfigureDialog(qlonglong windowId = 0);

    void expunge(qint64 collectionId);
protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;

private Q_SLOTS:
    void initializeCollections();
    void initialCollectionFetchingDone(KJob *);
    void mailCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void mailCollectionChanged(const Akonadi::Collection &collection);
    void mailCollectionRemoved(const Akonadi::Collection &collection);
    void emitProgress(int percent = 0);
    void emitProgressMessage(const QString &message);
    void itemsReceiviedForFiltering(const Akonadi::Item::List &items);
    void clearMessage();
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);
    void slotItemChanged(const Akonadi::Item &item);

public Q_SLOTS:
    void configure(WId windowId) override;

private:
    bool isFilterableCollection(const Akonadi::Collection &collection) const;

    FilterManager *m_filterManager = nullptr;

    FilterLogDialog *m_filterLogDialog = nullptr;
    QTimer *mProgressTimer = nullptr;
    DummyKernel *mMailFilterKernel = nullptr;
    int mProgressCounter;
    Akonadi::Monitor *itemMonitor = nullptr;

    void filterItem(const Akonadi::Item &item, const Akonadi::Collection &collection);
};

#endif
