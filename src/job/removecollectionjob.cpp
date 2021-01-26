/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "removecollectionjob.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include <AkonadiCore/CollectionDeleteJob>
#include <AkonadiCore/CollectionFetchJob>
#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

RemoveCollectionJob::RemoveCollectionJob(QObject *parent)
    : QObject(parent)
{
}

RemoveCollectionJob::~RemoveCollectionJob() = default;

void RemoveCollectionJob::setMainWidget(KMMainWidget *mainWidget)
{
    mMainWidget = mainWidget;
}

void RemoveCollectionJob::setCurrentFolder(const Akonadi::Collection &currentFolder)
{
    mCurrentCollection = currentFolder;
}

void RemoveCollectionJob::start()
{
    auto job = new Akonadi::CollectionFetchJob(mCurrentCollection, Akonadi::CollectionFetchJob::FirstLevel, this);
    job->fetchScope().setContentMimeTypes(QStringList() << KMime::Message::mimeType());
    job->setProperty("collectionId", mCurrentCollection.id());
    connect(job, &KJob::result, this, &RemoveCollectionJob::slotDelayedRemoveFolder);
}

void RemoveCollectionJob::slotDelayedRemoveFolder(KJob *job)
{
    const Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    Akonadi::Collection::List listOfCollection = fetchJob->collections();
    const bool hasNotSubDirectory = listOfCollection.isEmpty();

    const Akonadi::Collection::Id id = fetchJob->property("collectionId").toLongLong();
    const auto col = CommonKernel->collectionFromId(id);
    QString str;
    QString title;
    QString buttonLabel;
    if (col.resource() == QLatin1String("akonadi_search_resource")) {
        title = i18n("Delete Search");
        str = i18n(
            "<qt>Are you sure you want to delete the search <b>%1</b>?<br />"
            "Any messages it shows will still be available in their original folder.</qt>",
            col.name().toHtmlEscaped());
        buttonLabel = i18nc("@action:button Delete search", "&Delete");
    } else {
        title = i18n("Delete Folder");

        if (col.statistics().count() == 0) {
            if (hasNotSubDirectory) {
                str = i18n(
                    "<qt>Are you sure you want to delete the empty folder "
                    "<b>%1</b>?</qt>",
                    col.name().toHtmlEscaped());
            } else {
                str = i18n(
                    "<qt>Are you sure you want to delete the empty folder "
                    "<resource>%1</resource> and all its subfolders? Those subfolders might "
                    "not be empty and their contents will be discarded as well. "
                    "<p><b>Beware</b> that discarded messages are not saved "
                    "into your Trash folder and are permanently deleted.</p></qt>",
                    col.name().toHtmlEscaped());
            }
        } else {
            if (hasNotSubDirectory) {
                str = i18n(
                    "<qt>Are you sure you want to delete the folder "
                    "<resource>%1</resource>, discarding its contents? "
                    "<p><b>Beware</b> that discarded messages are not saved "
                    "into your Trash folder and are permanently deleted.</p></qt>",
                    col.name().toHtmlEscaped());
            } else {
                str = i18n(
                    "<qt>Are you sure you want to delete the folder <resource>%1</resource> "
                    "and all its subfolders, discarding their contents? "
                    "<p><b>Beware</b> that discarded messages are not saved "
                    "into your Trash folder and are permanently deleted.</p></qt>",
                    col.name().toHtmlEscaped());
            }
        }
        buttonLabel = i18nc("@action:button Delete folder", "&Delete");
    }

    if (KMessageBox::warningContinueCancel(mMainWidget,
                                           str,
                                           title,
                                           KGuiItem(buttonLabel, QStringLiteral("edit-delete")),
                                           KStandardGuiItem::cancel(),
                                           QString(),
                                           KMessageBox::Notify | KMessageBox::Dangerous)
        == KMessageBox::Continue) {
        kmkernel->checkFolderFromResources(listOfCollection << col);

        if (col.id() == mMainWidget->currentCollection().id()) {
            Q_EMIT clearCurrentFolder();
        }

        auto job = new Akonadi::CollectionDeleteJob(col);
        connect(job, &KJob::result, this, &RemoveCollectionJob::slotDeletionCollectionResult);
    } else {
        deleteLater();
    }
}

void RemoveCollectionJob::slotDeletionCollectionResult(KJob *job)
{
    if (job) {
        if (!MailCommon::Util::showJobErrorMessage(job)) {
            // TODO
        }
    }
    deleteLater();
}
