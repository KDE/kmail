/*
  SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "createnewcontactjob.h"
#include "util.h"

#include <PimCommon/BroadcastStatus>

#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include <Akonadi/Contact/ContactEditorDialog>
#include <AkonadiCore/AgentFilterProxyModel>
#include <AkonadiCore/AgentInstanceCreateJob>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiWidgets/AgentTypeDialog>

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
                auto job = new Akonadi::AgentInstanceCreateJob(agentType, this);
                connect(job, &Akonadi::AgentInstanceCreateJob::result, this, &CreateNewContactJob::slotResourceCreationDone);
                job->configure(mParentWidget);
                job->start();
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
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    createContact();
    emitResult();
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
    KMessageBox::error(mParentWidget, i18n("Contact cannot be stored: %1", error), i18n("Failed to store contact"));
}
