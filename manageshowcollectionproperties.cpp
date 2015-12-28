/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "manageshowcollectionproperties.h"
#include "kmmainwidget.h"
#include <KLocalizedString>
#include <KMessageBox>
#include <AkonadiCore/CollectionAttributesSynchronizationJob>
#include "kmail_debug.h"
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiWidgets/CollectionPropertiesDialog>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>

Q_DECLARE_METATYPE(KPIM::ProgressItem *)
Q_DECLARE_METATYPE(Akonadi::Job *)
Q_DECLARE_METATYPE(QPointer<KPIM::ProgressItem>)

ManageShowCollectionProperties::ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent)
    : QObject(parent),
      mMainWidget(mainWidget)
{
    mPages = QStringList() << QStringLiteral("MailCommon::CollectionGeneralPage")
             << QStringLiteral("KMail::CollectionViewPage")
             << QStringLiteral("Akonadi::CachePolicyPage")
             << QStringLiteral("KMail::CollectionTemplatesPage")
             << QStringLiteral("MailCommon::CollectionExpiryPage")
             << QStringLiteral("PimCommon::CollectionAclPage")
             << QStringLiteral("KMail::CollectionMailingListPage")
             << QStringLiteral("KMail::CollectionQuotaPage")
             << QStringLiteral("KMail::CollectionShortcutPage")
             << QStringLiteral("KMail::CollectionMaintenancePage");

}

ManageShowCollectionProperties::~ManageShowCollectionProperties()
{

}

void ManageShowCollectionProperties::slotCollectionProperties()
{
    showCollectionProperties(QString());
}

void ManageShowCollectionProperties::slotShowExpiryProperties()
{
    showCollectionProperties(QStringLiteral("MailCommon::CollectionExpiryPage"));
}

void ManageShowCollectionProperties::slotFolderMailingListProperties()
{
    showCollectionProperties(QStringLiteral("KMail::CollectionMailingListPage"));
}

void ManageShowCollectionProperties::slotShowFolderShortcutDialog()
{
    showCollectionProperties(QStringLiteral("KMail::CollectionShortcutPage"));
}

void ManageShowCollectionProperties::showCollectionProperties(const QString &pageToShow)
{
    if (!mMainWidget->currentFolder()) {
        return;
    }
    const Akonadi::Collection::Id id = mMainWidget->currentFolder()->collection().id();
    QPointer<Akonadi::CollectionPropertiesDialog> dlg = mHashDialogBox.value(id);
    if (dlg) {
        if (!pageToShow.isEmpty()) {
            dlg->setCurrentPage(pageToShow);
        }
        dlg->activateWindow();
        dlg->raise();
        return;
    }
    if (!KMKernel::self()->isOffline()) {
        const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance(mMainWidget->currentFolder()->collection().resource());
        bool isOnline = agentInstance.isOnline();
        if (!isOnline) {
            showCollectionPropertiesContinued(pageToShow, QPointer<KPIM::ProgressItem>());
        } else {
            QPointer<KPIM::ProgressItem> progressItem(KPIM::ProgressManager::createProgressItem(i18n("Retrieving folder properties")));
            progressItem->setUsesBusyIndicator(true);
            progressItem->setCryptoStatus(KPIM::ProgressItem::Unknown);

            Akonadi::CollectionAttributesSynchronizationJob *sync
                = new Akonadi::CollectionAttributesSynchronizationJob(mMainWidget->currentFolder()->collection());
            sync->setProperty("collectionId", id);
            sync->setProperty("pageToShow", pageToShow);          // note for dialog later
            sync->setProperty("progressItem", QVariant::fromValue(progressItem));
            connect(sync, &KJob::result,
                    this, &ManageShowCollectionProperties::slotCollectionPropertiesContinued);
            connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
                    sync, SLOT(kill()));
            connect(progressItem.data(), &KPIM::ProgressItem::progressItemCanceled,
                    KPIM::ProgressManager::instance(), &KPIM::ProgressManager::slotStandardCancelHandler);
            sync->start();
        }
    } else {
        KMessageBox::information(
            mMainWidget,
            i18n("Network is unconnected. Folder information cannot be updated."));
        showCollectionPropertiesContinued(pageToShow, QPointer<KPIM::ProgressItem>());
    }
}

void ManageShowCollectionProperties::slotCollectionPropertiesContinued(KJob *job)
{
    QString pageToShow;
    QPointer<KPIM::ProgressItem> progressItem;

    if (job) {
        Akonadi::CollectionAttributesSynchronizationJob *sync
            = dynamic_cast<Akonadi::CollectionAttributesSynchronizationJob *>(job);
        Q_ASSERT(sync);
        if (sync->property("collectionId") != mMainWidget->currentFolder()->collection().id()) {
            return;
        }
        pageToShow = sync->property("pageToShow").toString();
        progressItem = sync->property("progressItem").value< QPointer<KPIM::ProgressItem> >();
        if (progressItem) {
            disconnect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
                       sync, SLOT(kill()));
        } else {
            // progressItem does not exist anymore, operation has been canceled
            return;
        }
    }

    showCollectionPropertiesContinued(pageToShow, progressItem);
}

void ManageShowCollectionProperties::showCollectionPropertiesContinued(const QString &pageToShow, QPointer<KPIM::ProgressItem> progressItem)
{
    if (!progressItem) {
        progressItem = KPIM::ProgressManager::createProgressItem(i18n("Retrieving folder properties"));
        progressItem->setUsesBusyIndicator(true);
        progressItem->setCryptoStatus(KPIM::ProgressItem::Unknown);
        connect(progressItem.data(), &KPIM::ProgressItem::progressItemCanceled,
                KPIM::ProgressManager::instance(), &KPIM::ProgressManager::slotStandardCancelHandler);
    }

    Akonadi::CollectionFetchJob *fetch = new Akonadi::CollectionFetchJob(mMainWidget->currentFolder()->collection(),
            Akonadi::CollectionFetchJob::Base);
    connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)), fetch, SLOT(kill()));
    fetch->fetchScope().setIncludeStatistics(true);
    fetch->setProperty("pageToShow", pageToShow);
    fetch->setProperty("progressItem", QVariant::fromValue(progressItem));
    connect(fetch, &KJob::result,
            this, &ManageShowCollectionProperties::slotCollectionPropertiesFinished);
    connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
            fetch, SLOT(kill()));
}

void ManageShowCollectionProperties::slotCollectionPropertiesFinished(KJob *job)
{
    if (!job) {
        return;
    }

    QPointer<KPIM::ProgressItem> progressItem = job->property("progressItem").value< QPointer<KPIM::ProgressItem> >();
    // progressItem does not exist anymore, operation has been canceled
    if (!progressItem) {
        return;
    }

    progressItem->setComplete();
    progressItem->setStatus(i18n("Done"));

    Akonadi::CollectionFetchJob *fetch = dynamic_cast<Akonadi::CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->collections().isEmpty()) {
        qCWarning(KMAIL_LOG) << "no collection";
        return;
    }

    const Akonadi::Collection collection = fetch->collections().first();

    QPointer<Akonadi::CollectionPropertiesDialog> dlg = new Akonadi::CollectionPropertiesDialog(collection, mPages, mMainWidget);
    dlg->setWindowTitle(i18nc("@title:window", "Properties of Folder %1", collection.name()));
    connect(dlg.data(), &Akonadi::CollectionPropertiesDialog::accepted, mMainWidget, &KMMainWidget::slotUpdateConfig);

    const QString pageToShow = fetch->property("pageToShow").toString();
    if (!pageToShow.isEmpty()) {                          // show a specific page
        dlg->setCurrentPage(pageToShow);
    }
    dlg->show();
    mHashDialogBox.insert(collection.id(), dlg);
}
