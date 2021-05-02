/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "manageshowcollectionproperties.h"
#include "kmail_debug.h"
#include "kmmainwidget.h"
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/CollectionAttributesSynchronizationJob>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiWidgets/CollectionPropertiesDialog>
#include <KLocalizedString>
#include <KMessageBox>

Q_DECLARE_METATYPE(KPIM::ProgressItem *)
Q_DECLARE_METATYPE(Akonadi::Job *)
Q_DECLARE_METATYPE(QPointer<KPIM::ProgressItem>)

ManageShowCollectionProperties::ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent)
    : QObject(parent)
    , mMainWidget(mainWidget)
    , mPages({QStringLiteral("MailCommon::CollectionGeneralPage"),
              QStringLiteral("KMail::CollectionViewPage"),
              QStringLiteral("Akonadi::CachePolicyPage"),
              QStringLiteral("KMail::CollectionTemplatesPage"),
              QStringLiteral("MailCommon::CollectionExpiryPage"),
              QStringLiteral("PimCommon::CollectionAclPage"),
              QStringLiteral("KMail::CollectionMailingListPage"),
              QStringLiteral("KMail::CollectionQuotaPage"),
              QStringLiteral("KMail::CollectionShortcutPage"),
              QStringLiteral("Akonadi::CollectionMaintenancePage")})
{
}

ManageShowCollectionProperties::~ManageShowCollectionProperties() = default;

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
    if (!mMainWidget->currentCollection().isValid()) {
        return;
    }
    const Akonadi::Collection col = mMainWidget->currentCollection();
    const Akonadi::Collection::Id id = col.id();
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
        const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance(col.resource());
        bool isOnline = agentInstance.isOnline();
        if (!isOnline) {
            showCollectionPropertiesContinued(pageToShow, QPointer<KPIM::ProgressItem>());
        } else {
            QPointer<KPIM::ProgressItem> progressItem(KPIM::ProgressManager::createProgressItem(i18n("Retrieving folder properties")));
            progressItem->setUsesBusyIndicator(true);
            progressItem->setCryptoStatus(KPIM::ProgressItem::Unknown);

            auto sync = new Akonadi::CollectionAttributesSynchronizationJob(col);
            sync->setProperty("collectionId", id);
            sync->setProperty("pageToShow", pageToShow); // note for dialog later
            sync->setProperty("progressItem", QVariant::fromValue(progressItem));
            connect(sync, &KJob::result, this, &ManageShowCollectionProperties::slotCollectionPropertiesContinued);
            connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem *)), sync, SLOT(kill()));
            connect(progressItem.data(),
                    &KPIM::ProgressItem::progressItemCanceled,
                    KPIM::ProgressManager::instance(),
                    &KPIM::ProgressManager::slotStandardCancelHandler);
            sync->start();
        }
    } else {
        KMessageBox::information(mMainWidget, i18n("Network is unconnected. Folder information cannot be updated."));
        showCollectionPropertiesContinued(pageToShow, QPointer<KPIM::ProgressItem>());
    }
}

void ManageShowCollectionProperties::slotCollectionPropertiesContinued(KJob *job)
{
    QString pageToShow;
    QPointer<KPIM::ProgressItem> progressItem;

    if (job) {
        auto sync = qobject_cast<Akonadi::CollectionAttributesSynchronizationJob *>(job);
        Q_ASSERT(sync);
        if (sync->property("collectionId") != mMainWidget->currentCollection().id()) {
            return;
        }
        pageToShow = sync->property("pageToShow").toString();
        progressItem = sync->property("progressItem").value<QPointer<KPIM::ProgressItem>>();
        if (progressItem) {
            disconnect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem *)), sync, SLOT(kill()));
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
        connect(progressItem.data(),
                &KPIM::ProgressItem::progressItemCanceled,
                KPIM::ProgressManager::instance(),
                &KPIM::ProgressManager::slotStandardCancelHandler);
    }

    auto fetch = new Akonadi::CollectionFetchJob(mMainWidget->currentCollection(), Akonadi::CollectionFetchJob::Base);
    connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem *)), fetch, SLOT(kill()));
    fetch->fetchScope().setIncludeStatistics(true);
    fetch->setProperty("pageToShow", pageToShow);
    fetch->setProperty("progressItem", QVariant::fromValue(progressItem));
    connect(fetch, &KJob::result, this, &ManageShowCollectionProperties::slotCollectionPropertiesFinished);
    connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem *)), fetch, SLOT(kill()));
}

void ManageShowCollectionProperties::slotCollectionPropertiesFinished(KJob *job)
{
    if (!job) {
        return;
    }

    auto progressItem = job->property("progressItem").value<QPointer<KPIM::ProgressItem>>();
    // progressItem does not exist anymore, operation has been canceled
    if (!progressItem) {
        return;
    }

    progressItem->setComplete();
    progressItem->setStatus(i18n("Done"));

    auto fetch = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->collections().isEmpty()) {
        qCWarning(KMAIL_LOG) << "no collection";
        return;
    }

    const Akonadi::Collection collection = fetch->collections().constFirst();

    QPointer<Akonadi::CollectionPropertiesDialog> dlg = new Akonadi::CollectionPropertiesDialog(collection, mPages, mMainWidget);
    dlg->setWindowTitle(i18nc("@title:window", "Properties of Folder %1", collection.name()));
    connect(dlg.data(), &Akonadi::CollectionPropertiesDialog::settingsSaved, mMainWidget, &KMMainWidget::slotUpdateConfig);

    const QString pageToShow = fetch->property("pageToShow").toString();
    if (!pageToShow.isEmpty()) { // show a specific page
        dlg->setCurrentPage(pageToShow);
    }
    dlg->show();
    mHashDialogBox.insert(collection.id(), dlg);
}
