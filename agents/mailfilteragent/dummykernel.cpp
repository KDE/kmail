#include "dummykernel.h"

#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <MessageComposer/AkonadiSender>
#include <MailCommon/FolderCollectionMonitor>
#include <AkonadiCore/session.h>
#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/changerecorder.h>
#include <KSharedConfig>

DummyKernel::DummyKernel(QObject *parent)
    : QObject(parent)
{
    mMessageSender = new MessageComposer::AkonadiSender(this);
    mIdentityManager = new KIdentityManagement::IdentityManager(true, this);
    Akonadi::Session *session = new Akonadi::Session("MailFilter Kernel ETM", this);

    mFolderCollectionMonitor = new MailCommon::FolderCollectionMonitor(session, this);

    mEntityTreeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor(), this);
    mEntityTreeModel->setListFilter(Akonadi::CollectionFetchScope::Enabled);
    mEntityTreeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    mCollectionModel = new Akonadi::EntityMimeTypeFilterModel(this);
    mCollectionModel->setSourceModel(mEntityTreeModel);
    mCollectionModel->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
    mCollectionModel->setHeaderGroup(Akonadi::EntityTreeModel::CollectionTreeHeaders);
    mCollectionModel->setDynamicSortFilter(true);
    mCollectionModel->setSortCaseSensitivity(Qt::CaseInsensitive);
}

KIdentityManagement::IdentityManager *DummyKernel::identityManager()
{
    return mIdentityManager;
}

MessageComposer::MessageSender *DummyKernel::msgSender()
{
    return mMessageSender;
}

Akonadi::EntityMimeTypeFilterModel *DummyKernel::collectionModel() const
{
    return mCollectionModel;
}

KSharedConfig::Ptr DummyKernel::config()
{
    return KSharedConfig::openConfig();
}

void DummyKernel::syncConfig()
{
    Q_ASSERT(false);
}

MailCommon::JobScheduler *DummyKernel::jobScheduler() const
{
    Q_ASSERT(false);
    return Q_NULLPTR;
}

Akonadi::ChangeRecorder *DummyKernel::folderCollectionMonitor() const
{
    return mFolderCollectionMonitor->monitor();
}

void DummyKernel::updateSystemTray()
{
    Q_ASSERT(false);
}

bool DummyKernel::showPopupAfterDnD()
{
    return false;
}

qreal DummyKernel::closeToQuotaThreshold()
{
    return 80;
}

QStringList DummyKernel::customTemplates()
{
    Q_ASSERT(false);
    return QStringList();
}

bool DummyKernel::excludeImportantMailFromExpiry()
{
    Q_ASSERT(false);
    return true;
}

Akonadi::Collection::Id DummyKernel::lastSelectedFolder()
{
    Q_ASSERT(false);
    return Akonadi::Collection::Id();
}

void DummyKernel::setLastSelectedFolder(Akonadi::Collection::Id col)
{
    Q_UNUSED(col);
}

void DummyKernel::expunge(Akonadi::Collection::Id id, bool sync)
{
    Akonadi::Collection col(id);
    if (col.isValid()) {
        mFolderCollectionMonitor->expunge(Akonadi::Collection(col), sync);
    }
}
