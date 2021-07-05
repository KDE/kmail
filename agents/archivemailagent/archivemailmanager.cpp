/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailmanager.h"
#include "archivemailagentutil.h"
#include "archivemailinfo.h"
#include "archivemailkernel.h"
#include "job/archivejob.h"

#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include "archivemailagent_debug.h"
#include <KConfigGroup>

#include <QDate>
#include <QFile>
#include <QRegularExpression>

ArchiveMailManager::ArchiveMailManager(QObject *parent)
    : QObject(parent)
{
    mArchiveMailKernel = ArchiveMailKernel::self();
    CommonKernel->registerKernelIf(mArchiveMailKernel); // register KernelIf early, it is used by the Filter classes
    CommonKernel->registerSettingsIf(mArchiveMailKernel); // SettingsIf is used in FolderTreeWidget
    mConfig = KSharedConfig::openConfig();
}

ArchiveMailManager::~ArchiveMailManager()
{
    qDeleteAll(mListArchiveInfo);
}

void ArchiveMailManager::load()
{
    qDeleteAll(mListArchiveInfo);
    mListArchiveInfo.clear();

    const QStringList collectionList = mConfig->groupList().filter(QRegularExpression(QStringLiteral("ArchiveMailCollection \\d+")));
    const int numberOfCollection = collectionList.count();
    for (int i = 0; i < numberOfCollection; ++i) {
        KConfigGroup group = mConfig->group(collectionList.at(i));
        auto info = new ArchiveMailInfo(group);

        if (ArchiveMailAgentUtil::needToArchive(info)) {
            for (ArchiveMailInfo *oldInfo : std::as_const(mListArchiveInfo)) {
                if (oldInfo->saveCollectionId() == info->saveCollectionId()) {
                    // already in jobscheduler
                    delete info;
                    info = nullptr;
                    break;
                }
            }
            if (info) {
                // Store task started
                mListArchiveInfo.append(info);
                auto task = new ScheduledArchiveTask(this, info, Akonadi::Collection(info->saveCollectionId()), /*immediate*/ false);
                mArchiveMailKernel->jobScheduler()->registerTask(task);
            }
        } else {
            delete info;
        }
    }
}

void ArchiveMailManager::removeCollection(const Akonadi::Collection &collection)
{
    removeCollectionId(collection.id());
}

void ArchiveMailManager::removeCollectionId(Akonadi::Collection::Id id)
{
    const QString groupname = ArchiveMailAgentUtil::archivePattern.arg(id);
    if (mConfig->hasGroup(groupname)) {
        KConfigGroup group = mConfig->group(groupname);
        group.deleteGroup();
        mConfig->sync();
        mConfig->reparseConfiguration();
        const auto lst = mListArchiveInfo;
        for (ArchiveMailInfo *info : lst) {
            if (info->saveCollectionId() == id) {
                mListArchiveInfo.removeAll(info);
            }
        }
    }
}

void ArchiveMailManager::backupDone(ArchiveMailInfo *info)
{
    info->setLastDateSaved(QDate::currentDate());
    const QString groupname = ArchiveMailAgentUtil::archivePattern.arg(info->saveCollectionId());
    // Don't store it if we removed this task
    if (mConfig->hasGroup(groupname)) {
        KConfigGroup group = mConfig->group(groupname);
        info->writeConfig(group);
    }
    Akonadi::Collection collection(info->saveCollectionId());
    const QString realPath = MailCommon::Util::fullCollectionPath(collection);
    bool dirExist = true;
    const QStringList lst = info->listOfArchive(realPath, dirExist);
    if (dirExist) {
        if (info->maximumArchiveCount() != 0) {
            if (lst.count() > info->maximumArchiveCount()) {
                const int diff = (lst.count() - info->maximumArchiveCount());
                for (int i = 0; i < diff; ++i) {
                    const QString fileToRemove(info->url().path() + QLatin1Char('/') + lst.at(i));
                    qCDebug(ARCHIVEMAILAGENT_LOG) << " file to remove " << fileToRemove;
                    QFile::remove(fileToRemove);
                }
            }
        }
    }
    mListArchiveInfo.removeAll(info);

    Q_EMIT needUpdateConfigDialogBox();
}

void ArchiveMailManager::collectionDoesntExist(ArchiveMailInfo *info)
{
    removeCollectionId(info->saveCollectionId());
    mListArchiveInfo.removeAll(info);
    Q_EMIT needUpdateConfigDialogBox();
}

void ArchiveMailManager::pause()
{
    mArchiveMailKernel->jobScheduler()->pause();
}

void ArchiveMailManager::resume()
{
    mArchiveMailKernel->jobScheduler()->resume();
}

QString ArchiveMailManager::printCurrentListInfo() const
{
    QString infoStr;
    if (mListArchiveInfo.isEmpty()) {
        infoStr = QStringLiteral("No archive in queue");
    } else {
        for (ArchiveMailInfo *info : std::as_const(mListArchiveInfo)) {
            if (!infoStr.isEmpty()) {
                infoStr += QLatin1Char('\n');
            }
            infoStr += infoToStr(info);
        }
    }
    return infoStr;
}

QString ArchiveMailManager::infoToStr(ArchiveMailInfo *info) const
{
    QString infoStr = QLatin1String("collectionId: ") + QString::number(info->saveCollectionId()) + QLatin1Char('\n');
    infoStr += QLatin1String("save sub collection: ") + (info->saveSubCollection() ? QStringLiteral("true") : QStringLiteral("false")) + QLatin1Char('\n');
    infoStr += QLatin1String("last Date Saved: ") + info->lastDateSaved().toString() + QLatin1Char('\n');
    infoStr += QLatin1String("maximum archive number: ") + QString::number(info->maximumArchiveCount()) + QLatin1Char('\n');
    infoStr += QLatin1String("directory: ") + info->url().toDisplayString() + QLatin1Char('\n');
    infoStr += QLatin1String("Enabled: ") + (info->isEnabled() ? QStringLiteral("true") : QStringLiteral("false"));
    return infoStr;
}

QString ArchiveMailManager::printArchiveListInfo() const
{
    QString infoStr;
    const QStringList collectionList = mConfig->groupList().filter(QRegularExpression(QStringLiteral("ArchiveMailCollection \\d+")));
    const int numberOfCollection = collectionList.count();
    for (int i = 0; i < numberOfCollection; ++i) {
        KConfigGroup group = mConfig->group(collectionList.at(i));
        ArchiveMailInfo info(group);
        if (!infoStr.isEmpty()) {
            infoStr += QLatin1Char('\n');
        }
        infoStr += infoToStr(&info);
    }
    return infoStr;
}

void ArchiveMailManager::archiveFolder(const QString &path, Akonadi::Collection::Id collectionId)
{
    auto info = new ArchiveMailInfo;
    info->setSaveCollectionId(collectionId);
    info->setUrl(QUrl::fromLocalFile(path));
    mListArchiveInfo.append(info);
    auto task = new ScheduledArchiveTask(this, info, Akonadi::Collection(info->saveCollectionId()), true /*immediat*/);
    mArchiveMailKernel->jobScheduler()->registerTask(task);
}
