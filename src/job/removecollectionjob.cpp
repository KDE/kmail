/*
   Copyright (C) 2014-2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "removecollectionjob.h"
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionDeleteJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KGuiItem>
#include "kmmainwidget.h"
#include "MailCommon/MailUtil"
#include "MailCommon/MailKernel"
#include "kmkernel.h"

RemoveCollectionJob::RemoveCollectionJob(QObject *parent)
    : QObject(parent)
{
}

RemoveCollectionJob::~RemoveCollectionJob()
{
}

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
    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(mCurrentCollection, Akonadi::CollectionFetchJob::FirstLevel, this);
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
        str = i18n("<qt>Are you sure you want to delete the search <b>%1</b>?<br />"
                   "Any messages it shows will still be available in their original folder.</qt>",
                   col.name().toHtmlEscaped());
        buttonLabel = i18nc("@action:button Delete search", "&Delete");
    } else {
        title = i18n("Delete Folder");

        if (col.statistics().count() == 0) {
            if (hasNotSubDirectory) {
                str = i18n("<qt>Are you sure you want to delete the empty folder "
                           "<b>%1</b>?</qt>",
                           col.name().toHtmlEscaped());
            } else {
                str = i18n("<qt>Are you sure you want to delete the empty folder "
                           "<resource>%1</resource> and all its subfolders? Those subfolders might "
                           "not be empty and their contents will be discarded as well. "
                           "<p><b>Beware</b> that discarded messages are not saved "
                           "into your Trash folder and are permanently deleted.</p></qt>",
                           col.name().toHtmlEscaped());
            }
        } else {
            if (hasNotSubDirectory) {
                str = i18n("<qt>Are you sure you want to delete the folder "
                           "<resource>%1</resource>, discarding its contents? "
                           "<p><b>Beware</b> that discarded messages are not saved "
                           "into your Trash folder and are permanently deleted.</p></qt>",
                           col.name().toHtmlEscaped());
            } else {
                str = i18n("<qt>Are you sure you want to delete the folder <resource>%1</resource> "
                           "and all its subfolders, discarding their contents? "
                           "<p><b>Beware</b> that discarded messages are not saved "
                           "into your Trash folder and are permanently deleted.</p></qt>",
                           col.name().toHtmlEscaped());
            }
        }
        buttonLabel = i18nc("@action:button Delete folder", "&Delete");
    }

    if (KMessageBox::warningContinueCancel(mMainWidget, str, title,
                                           KGuiItem(buttonLabel, QStringLiteral("edit-delete")),
                                           KStandardGuiItem::cancel(), QString(),
                                           KMessageBox::Notify | KMessageBox::Dangerous)
        == KMessageBox::Continue) {
        kmkernel->checkFolderFromResources(listOfCollection << col);

        if (col.id() == mMainWidget->currentCollection().id()) {
            Q_EMIT clearCurrentFolder();
        }

        Akonadi::CollectionDeleteJob *job = new Akonadi::CollectionDeleteJob(col);
        connect(job, &KJob::result, this, &RemoveCollectionJob::slotDeletionCollectionResult);
    } else {
        deleteLater();
    }
}

void RemoveCollectionJob::slotDeletionCollectionResult(KJob *job)
{
    if (job) {
        MailCommon::Util::showJobErrorMessage(job);
    }
    deleteLater();
}
