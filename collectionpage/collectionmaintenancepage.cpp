/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009-2015 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "collectionmaintenancepage.h"
#include "util/mailutil.h"
#include "kmkernel.h"

#include <AkonadiCore/collectionstatistics.h>
#include <AkonadiCore/collection.h>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/ChangeRecorder>

#include <QDBusInterface>
#include <QDBusConnectionInterface>

#include <QDialog>
#include <QPushButton>
#include <kio/global.h>
#include <KLocalizedString>
#include "kmail_debug.h"
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QCheckBox>
#include <AkonadiCore/indexpolicyattribute.h>
#include <KFormat>
#include <KConfigGroup>

using namespace Akonadi;

CollectionMaintenancePage::CollectionMaintenancePage(QWidget *parent) :
    CollectionPropertiesPage(parent),
    mIsNotAVirtualCollection(true),
    mFolderSizeLabel(Q_NULLPTR),
    mCollectionCount(Q_NULLPTR)
{
    setObjectName(QStringLiteral("KMail::CollectionMaintenancePage"));
    setPageTitle(i18n("Maintenance"));
}

void CollectionMaintenancePage::init(const Akonadi::Collection &col)
{
    mCurrentCollection = col;

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    QGroupBox *filesGroup = new QGroupBox(i18n("Files"), this);
    QFormLayout *box = new QFormLayout(filesGroup);
    mIsNotAVirtualCollection = !MailCommon::Util::isVirtualCollection(col);
    connect(KMKernel::self()->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), this, SLOT(updateCollectionStatistic(Akonadi::Collection::Id,Akonadi::CollectionStatistics)));

    const AgentInstance instance = Akonadi::AgentManager::self()->instance(col.resource());
    const QString folderDesc = instance.type().name();

    if (mIsNotAVirtualCollection) {
        QLabel *label = new QLabel(folderDesc, filesGroup);
        box->addRow(new QLabel(i18n("Folder type:"), filesGroup), label);
    }

    mFolderSizeLabel = new QLabel(i18nc("folder size", "Not available"), filesGroup);
    box->addRow(new QLabel(i18n("Size:"), filesGroup), mFolderSizeLabel);

    topLayout->addWidget(filesGroup);

    QGroupBox *messagesGroup = new QGroupBox(i18n("Messages"), this);
    box = new QFormLayout(messagesGroup);

    mCollectionCount = new QLabel(messagesGroup);
    box->addRow(new QLabel(i18n("Total messages:"), messagesGroup), mCollectionCount);

    mCollectionUnread = new QLabel(messagesGroup);
    box->addRow(new QLabel(i18n("Unread messages:"), messagesGroup), mCollectionUnread);

    topLayout->addWidget(messagesGroup);
    QGroupBox *indexingGroup = new QGroupBox(i18n("Indexing"), this);
    QVBoxLayout *indexingLayout = new QVBoxLayout(indexingGroup);
    mIndexingEnabled = new QCheckBox(i18n("Enable Full Text Indexing"));
    indexingLayout->addWidget(mIndexingEnabled);

    mLastIndexed = new QLabel(i18n("Still not indexed."));

    indexingLayout->addWidget(mLastIndexed);
    topLayout->addWidget(indexingGroup);
    topLayout->addStretch(100);
}

void CollectionMaintenancePage::load(const Collection &col)
{
    init(col);
    if (col.isValid()) {
        updateLabel(col.statistics().count(), col.statistics().unreadCount(), col.statistics().size());
        Akonadi::IndexPolicyAttribute *attr = col.attribute<Akonadi::IndexPolicyAttribute>();
        const bool indexingWasEnabled(!attr || attr->indexingEnabled());
        mIndexingEnabled->setChecked(indexingWasEnabled);
        if (!indexingWasEnabled) {
            mLastIndexed->hide();
        } else {
            QDBusInterface interfaceBalooIndexer(QStringLiteral("org.freedesktop.Akonadi.Agent.akonadi_indexing_agent"), QStringLiteral("/"));
            if (interfaceBalooIndexer.isValid()) {
                if (!interfaceBalooIndexer.callWithCallback(QStringLiteral("indexedItems"), QList<QVariant>() << (qlonglong)mCurrentCollection.id(), this, SLOT(onIndexedItemsReceived(qint64)))) {
                    qCWarning(KMAIL_LOG) << "Failed to request indexed items";
                }
            } else {
                qCWarning(KMAIL_LOG) << "Dbus interface invalid.";
            }
        }
    }
}

void CollectionMaintenancePage::onIndexedItemsReceived(qint64 num)
{
    qCDebug(KMAIL_LOG) << num;
    if (num == 0) {
        mLastIndexed->clear();
    } else {
        mLastIndexed->setText(i18np("Indexed %1 item of this collection", "Indexed %1 items of this collection", num));
    }
}

void CollectionMaintenancePage::updateLabel(qint64 nbMail, qint64 nbUnreadMail, qint64 size)
{
    mCollectionCount->setText(QString::number(qMax(0LL, nbMail)));
    mCollectionUnread->setText(QString::number(qMax(0LL, nbUnreadMail)));
    mFolderSizeLabel->setText(KFormat().formatByteSize(qMax(0LL, size)));
}

void CollectionMaintenancePage::save(Collection &collection)
{
    if (!collection.hasAttribute<Akonadi::IndexPolicyAttribute>() && mIndexingEnabled->isChecked()) {
        return;
    }
    Akonadi::IndexPolicyAttribute *attr = collection.attribute<Akonadi::IndexPolicyAttribute>(Akonadi::Collection::AddIfMissing);
    if (mIndexingEnabled->isChecked()) {
        attr->setIndexingEnabled(true);
    } else {
        attr->setIndexingEnabled(false);
    }
}

void CollectionMaintenancePage::updateCollectionStatistic(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistic)
{
    if (id == mCurrentCollection.id()) {
        updateLabel(statistic.count(), statistic.unreadCount(), statistic.size());
    }
}

