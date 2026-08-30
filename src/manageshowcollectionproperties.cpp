/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "manageshowcollectionproperties.h"
#include "kmail_debug.h"
#include "kmmainwidget.h"
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/CollectionAttributesSynchronizationJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionPropertiesDialog>
#include <KLocalizedString>
#include <KMessageBox>
using namespace Qt::Literals::StringLiterals;

Q_DECLARE_METATYPE(KPIM::ProgressItem *)
Q_DECLARE_METATYPE(Akonadi::Job *)
Q_DECLARE_METATYPE(QPointer<KPIM::ProgressItem>)

ManageShowCollectionProperties::ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent)
    : QObject(parent)
    , mMainWidget(mainWidget)
    , mPages({u"MailCommon::CollectionGeneralPage"_s,
              u"KMail::CollectionViewPage"_s,
              u"Akonadi::CachePolicyPage"_s,
              u"KMail::CollectionTemplatesPage"_s,
              u"MailCommon::CollectionExpiryPage"_s,
              u"PimCommon::CollectionAclPage"_s,
              u"KMail::CollectionMailingListPage"_s,
              u"KMail::CollectionQuotaPage"_s,
              u"KMail::CollectionShortcutPage"_s,
              u"Akonadi::CollectionMaintenancePage"_s})
{
}

ManageShowCollectionProperties::~ManageShowCollectionProperties() = default;

void ManageShowCollectionProperties::slotCollectionProperties()
{
    showCollectionProperties(QString());
}

void ManageShowCollectionProperties::slotShowExpiryProperties()
{
    showCollectionProperties(u"MailCommon::CollectionExpiryPage"_s);
}

void ManageShowCollectionProperties::slotFolderMailingListProperties()
{
    showCollectionProperties(u"KMail::CollectionMailingListPage"_s);
}

void ManageShowCollectionProperties::slotShowFolderShortcutDialog()
{
    showCollectionProperties(u"KMail::CollectionShortcutPage"_s);
}

void ManageShowCollectionProperties::showCollectionProperties(const QString &pageToShow)
{
    if (!mMainWidget->currentCollection().isValid()) {
        return;
    }
    const Akonadi::Collection col = mMainWidget->currentCollection();
    const Akonadi::Collection::Id id = col.id();
    if (QPointer<Akonadi::CollectionPropertiesDialog> dlg = mHashDialogBox.value(id)) {
        if (!pageToShow.isEmpty()) {
            dlg->setCurrentPage(pageToShow);
        }
        dlg->activateWindow();
        dlg->raise();
        return;
    }
    if (!KMKernel::self()->isOffline()) {
        const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance(col.resource());
        if (bool isOnline = agentInstance.isOnline(); !isOnline) {
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
            // clang-format off
            connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)), sync, SLOT(kill()));
            // clang-format on
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
        progressItem = job->property("progressItem").value<QPointer<KPIM::ProgressItem>>();
        if (!progressItem) {
            // progressItem does not exist anymore, operation has been canceled
            return;
        }
        auto sync = qobject_cast<Akonadi::CollectionAttributesSynchronizationJob *>(job);
        if (!sync) {
            progressItem->setComplete();
            return;
        }
        // clang-format off
        disconnect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)), sync, SLOT(kill()));
        // clang-format on
        if (sync->property("collectionId").toLongLong() != mMainWidget->currentCollection().id()) {
            // Another folder was selected in the meantime: don't leave a stuck progress item behind.
            progressItem->setComplete();
            return;
        }
        pageToShow = sync->property("pageToShow").toString();
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
    // clang-format off
    connect(progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)), fetch, SLOT(kill()));
    // clang-format on
    fetch->fetchScope().setIncludeStatistics(true);
    fetch->setProperty("pageToShow", pageToShow);
    fetch->setProperty("progressItem", QVariant::fromValue(progressItem));
    connect(fetch, &KJob::result, this, &ManageShowCollectionProperties::slotCollectionPropertiesFinished);
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
    if (!fetch) {
        return;
    }
    if (fetch->collections().isEmpty()) {
        qCWarning(KMAIL_LOG) << "no collection";
        return;
    }

    const Akonadi::Collection collection = fetch->collections().constFirst();

    QPointer<Akonadi::CollectionPropertiesDialog> dlg = new Akonadi::CollectionPropertiesDialog(collection, mPages, mMainWidget);
    dlg->setWindowTitle(i18nc("@title:window", "Properties of Folder %1", collection.name()));
    connect(dlg.data(), &Akonadi::CollectionPropertiesDialog::settingsSaved, mMainWidget, &KMMainWidget::slotUpdateConfig);

    if (const QString pageToShow = fetch->property("pageToShow").toString(); !pageToShow.isEmpty()) { // show a specific page
        dlg->setCurrentPage(pageToShow);
    }
    dlg->show();
    mHashDialogBox.insert(collection.id(), dlg);
}

#include "moc_manageshowcollectionproperties.cpp"
