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
    void applySpecificFilters(const QList< qint64 > &itemIds, int requires, const QStringList &listFilters);

    void reload();

    void showFilterLogDialog(qlonglong windowId = 0);
    QString printCollectionMonitored();

    void showConfigureDialog(qlonglong windowId = 0);

    void expunge(qint64 collectionId);
protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

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
public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;

private:
    FilterManager *m_filterManager;

    FilterLogDialog *m_filterLogDialog;
    QTimer *mProgressTimer;
    DummyKernel *mMailFilterKernel;
    int mProgressCounter;
};

#endif
