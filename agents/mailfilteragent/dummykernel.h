#ifndef DUMMYKERNEL_H
#define DUMMYKERNEL_H

#include <MailCommon/MailInterfaces>

namespace Akonadi {
class EntityTreeModel;
class EntityMimeTypeFilterModel;
}

namespace MailCommon {
class FolderCollectionMonitor;
}

class DummyKernel : public QObject, public MailCommon::IKernel, public MailCommon::ISettings
{
    Q_OBJECT
public:
    explicit DummyKernel(QObject *parent = nullptr);

    KIdentityManagement::IdentityManager *identityManager() override;
    MessageComposer::MessageSender *msgSender() override;

    Akonadi::EntityMimeTypeFilterModel *collectionModel() const override;
    KSharedConfig::Ptr config() override;
    void syncConfig() override;
    MailCommon::JobScheduler *jobScheduler() const override;
    Akonadi::ChangeRecorder *folderCollectionMonitor() const override;
    void updateSystemTray() override;

    qreal closeToQuotaThreshold() override;
    bool excludeImportantMailFromExpiry() override;
    QStringList customTemplates() override;
    Akonadi::Collection::Id lastSelectedFolder() override;
    void setLastSelectedFolder(Akonadi::Collection::Id col) override;
    bool showPopupAfterDnD() override;
    void expunge(Akonadi::Collection::Id id, bool sync) override;

private:
    Q_DISABLE_COPY(DummyKernel)
    KIdentityManagement::IdentityManager *mIdentityManager = nullptr;
    MessageComposer::MessageSender *mMessageSender = nullptr;
    MailCommon::FolderCollectionMonitor *mFolderCollectionMonitor = nullptr;
    Akonadi::EntityTreeModel *mEntityTreeModel = nullptr;
    Akonadi::EntityMimeTypeFilterModel *mCollectionModel = nullptr;
};

#endif
