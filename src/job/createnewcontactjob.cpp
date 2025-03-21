/*
  SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "createnewcontactjob.h"

#include <PimCommon/BroadcastStatus>

#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include <Akonadi/AgentConfigurationDialog>
#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/AgentTypeDialog>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ContactEditorDialog>

#include <KLocalizedString>
#include <KMessageBox>
#include <QPointer>

CreateNewContactJob::CreateNewContactJob(QWidget *parentWidget, QObject *parent)
    : KJob(parent)
    , mParentWidget(parentWidget)
{
}

CreateNewContactJob::~CreateNewContactJob() = default;

void CreateNewContactJob::start()
{
    auto const addressBookJob = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive);

    addressBookJob->fetchScope().setContentMimeTypes(QStringList() << KContacts::Addressee::mimeType());
    connect(addressBookJob, &KJob::result, this, &CreateNewContactJob::slotCollectionsFetched);
}

void CreateNewContactJob::slotCollectionsFetched(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    const Akonadi::CollectionFetchJob *addressBookJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);

    Akonadi::Collection::List canCreateItemCollections;

    const Akonadi::Collection::List lstAddressCollection = addressBookJob->collections();
    for (const Akonadi::Collection &collection : lstAddressCollection) {
        if (Akonadi::Collection::CanCreateItem & collection.rights()) {
            canCreateItemCollections.append(collection);
        }
    }
    if (canCreateItemCollections.isEmpty()) {
        QPointer<Akonadi::AgentTypeDialog> dlg = new Akonadi::AgentTypeDialog(mParentWidget);
        dlg->setWindowTitle(i18nc("@title:window", "Add to Address Book"));
        dlg->agentFilterProxyModel()->addMimeTypeFilter(KContacts::Addressee::mimeType());
        dlg->agentFilterProxyModel()->addMimeTypeFilter(KContacts::ContactGroup::mimeType());
        dlg->agentFilterProxyModel()->addCapabilityFilter(QStringLiteral("Resource"));

        if (dlg->exec()) {
            const Akonadi::AgentType agentType = dlg->agentType();
            delete dlg;
            if (agentType.isValid()) {
                auto createAgentJob = new Akonadi::AgentInstanceCreateJob(agentType, this);
                connect(createAgentJob, &Akonadi::AgentInstanceCreateJob::result, this, &CreateNewContactJob::slotResourceCreationDone);
                createAgentJob->start();
                return;
            } else { // if agent is not valid => return error and finish job
                setError(UserDefinedError);
                emitResult();
                return;
            }
        } else { // dialog canceled => return error and finish job
            delete dlg;
            setError(UserDefinedError);
            emitResult();
            return;
        }
    }
    createContact();
    emitResult();
}

void CreateNewContactJob::slotResourceCreationDone(KJob *job)
{
    auto createAgentJob = qobject_cast<Akonadi::AgentInstanceCreateJob *>(job);
    Q_ASSERT(createAgentJob);

    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    auto configureDialog = new Akonadi::AgentConfigurationDialog(createAgentJob->instance(), mParentWidget);
    connect(configureDialog, &QDialog::accepted, this, [this] {
        createContact();
        emitResult();
    });

    connect(configureDialog, &QDialog::rejected, this, [this, instance = createAgentJob->instance()] {
        Akonadi::AgentManager::self()->removeInstance(instance);
        emitResult();
    });
    configureDialog->show();
}

void CreateNewContactJob::createContact()
{
    QPointer<Akonadi::ContactEditorDialog> dlg = new Akonadi::ContactEditorDialog(Akonadi::ContactEditorDialog::CreateMode, mParentWidget);
    connect(dlg.data(), &Akonadi::ContactEditorDialog::contactStored, this, &CreateNewContactJob::contactStored);
    connect(dlg.data(), &Akonadi::ContactEditorDialog::error, this, &CreateNewContactJob::slotContactEditorError);
    dlg->exec();
    delete dlg;
}

void CreateNewContactJob::contactStored(const Akonadi::Item &item)
{
    Q_UNUSED(item)
    PimCommon::BroadcastStatus::instance()->setStatusMsg(i18n("Contact created successfully"));
}

void CreateNewContactJob::slotContactEditorError(const QString &error)
{
    KMessageBox::error(mParentWidget, i18n("Contact cannot be stored: %1", error), i18nc("@title:window", "Failed to store contact"));
}

#include "moc_createnewcontactjob.cpp"
