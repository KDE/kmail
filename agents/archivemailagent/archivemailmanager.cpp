/*
   Copyright (C) 2012-2017 Montel Laurent <montel@kde.org>

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

#include "archivemailmanager.h"
#include "archivemailinfo.h"
#include "job/archivejob.h"
#include "archivemailkernel.h"
#include "archivemailagentutil.h"
#include "helper_p.h"
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <KConfigGroup>
#include <KSharedConfig>
#include "archivemailagent_debug.h"

#include <QDate>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

ArchiveMailManager::ArchiveMailManager(QObject *parent)
    : QObject(parent)
{
    mArchiveMailKernel = new ArchiveMailKernel(this);
    CommonKernel->registerKernelIf(mArchiveMailKernel);   //register KernelIf early, it is used by the Filter classes
    CommonKernel->registerSettingsIf(mArchiveMailKernel);   //SettingsIf is used in FolderTreeWidget
    mConfig = KSharedConfig::openConfig();
}

ArchiveMailManager::~ArchiveMailManager()
{
    qDeleteAll(mListArchiveInfo);
}

void ArchiveMailManager::slotArchiveNow(ArchiveMailInfo *info)
{
    if (!info) {
        return;
    }
    ArchiveMailInfo *stockInfo = new ArchiveMailInfo(*info);
    mListArchiveInfo.append(stockInfo);
    ScheduledArchiveTask *task = new ScheduledArchiveTask(this, stockInfo, Akonadi::Collection(stockInfo->saveCollectionId()), true /*immediat*/);
    mArchiveMailKernel->jobScheduler()->registerTask(task);
}

void ArchiveMailManager::load()
{
    qDeleteAll(mListArchiveInfo);
    mListArchiveInfo.clear();

    const QStringList collectionList = mConfig->groupList().filter(QRegularExpression(QStringLiteral("ArchiveMailCollection \\d+")));
    const int numberOfCollection = collectionList.count();
    for (int i = 0; i < numberOfCollection; ++i) {
        KConfigGroup group = mConfig->group(collectionList.at(i));
        ArchiveMailInfo *info = new ArchiveMailInfo(group);

        if (ArchiveMailAgentUtil::needToArchive(info)) {
            Q_FOREACH (ArchiveMailInfo *oldInfo, mListArchiveInfo) {
                if (oldInfo->saveCollectionId() == info->saveCollectionId()) {
                    //already in jobscheduler
                    delete info;
                    info = nullptr;
                    break;
                }
            }
            if (info) {
                //Store task started
                mListArchiveInfo.append(info);
                ScheduledArchiveTask *task = new ScheduledArchiveTask(this, info, Akonadi::Collection(info->saveCollectionId()), /*immediate*/false);
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
        Q_FOREACH (ArchiveMailInfo *info, mListArchiveInfo) {
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
    //Don't store it if we removed this task
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
                    const QString fileToRemove(info->url().path() + QDir::separator() + lst.at(i));
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

QString ArchiveMailManager::printCurrentListInfo()
{
    QString infoStr;
    if (mListArchiveInfo.isEmpty()) {
        infoStr = QStringLiteral("No archive in queue");
    } else {
        for (ArchiveMailInfo *info : qAsConst(mListArchiveInfo)) {
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

QString ArchiveMailManager::printArchiveListInfo()
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
    ArchiveMailInfo *info = new ArchiveMailInfo;
    info->setSaveCollectionId(collectionId);
    info->setUrl(QUrl::fromLocalFile(path));
    slotArchiveNow(info);
    delete info;
}

