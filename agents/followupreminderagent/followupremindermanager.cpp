/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "followupremindermanager.h"
using namespace Qt::Literals::StringLiterals;

#include "followupreminderagent_debug.h"
#include "followupreminderinfo.h"
#include "followupremindernoanswerdialog.h"
#include "followupreminderutil.h"
#include "jobs/followupreminderfinishtaskjob.h"
#include "jobs/followupreminderjob.h"

#include <Akonadi/SpecialMailCollections>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotification>
#include <QRegularExpression>
using namespace FollowUpReminder;

FollowUpReminderManager::FollowUpReminderManager(QObject *parent)
    : QObject(parent)
    , mConfig(FollowUpReminder::FollowUpReminderUtil::defaultConfig())
{
}

FollowUpReminderManager::~FollowUpReminderManager()
{
    qDeleteAll(mFollowUpReminderInfoList);
    mFollowUpReminderInfoList.clear();
}

void FollowUpReminderManager::load(bool forceReloadConfig)
{
    if (forceReloadConfig) {
        mConfig->reparseConfiguration();
    }
    const QStringList itemList = mConfig->groupList().filter(QRegularExpression(u"FollowupReminderItem \\d+"_s));
    const int numberOfItems = itemList.count();
    QList<FollowUpReminder::FollowUpReminderInfo *> noAnswerList;
    for (int i = 0; i < numberOfItems; ++i) {
        KConfigGroup group = mConfig->group(itemList.at(i));

        if (auto info = new FollowUpReminderInfo(group); info->isValid()) {
            if (!info->answerWasReceived()) {
                if (mInitialize) {
                    delete info;
                } else {
                    mFollowUpReminderInfoList.append(info);
                    auto noAnswerInfo = new FollowUpReminderInfo(*info);
                    noAnswerList.append(noAnswerInfo);
                }
            } else {
                delete info;
            }
        } else {
            delete info;
        }
    }
    if (!noAnswerList.isEmpty()) {
        mInitialize = true;
        if (!mNoAnswerDialog.data()) {
            mNoAnswerDialog = new FollowUpReminderNoAnswerDialog;
            connect(mNoAnswerDialog.data(),
                    &FollowUpReminderNoAnswerDialog::needToReparseConfiguration,
                    this,
                    &FollowUpReminderManager::slotReparseConfiguration);
        }
        mNoAnswerDialog->setInfo(noAnswerList);
        mNoAnswerDialog->wakeUp();
    }
}

void FollowUpReminderManager::addReminder(FollowUpReminder::FollowUpReminderInfo *info)
{
    if (info->isValid()) {
        FollowUpReminderUtil::writeFollowupReminderInfo(FollowUpReminderUtil::defaultConfig(), info, true);
    } else {
        delete info;
    }
}

void FollowUpReminderManager::slotReparseConfiguration()
{
    load(true);
}

void FollowUpReminderManager::checkFollowUp(const Akonadi::Item &item, const Akonadi::Collection &col)
{
    if (mFollowUpReminderInfoList.isEmpty()) {
        return;
    }

    switch (Akonadi::SpecialMailCollections::self()->specialCollectionType(col)) {
    case Akonadi::SpecialMailCollections::Trash:
    case Akonadi::SpecialMailCollections::Outbox:
    case Akonadi::SpecialMailCollections::Drafts:
    case Akonadi::SpecialMailCollections::Templates:
    case Akonadi::SpecialMailCollections::SentMail:
        return;
    default:
        break;
    }

    auto job = new FollowUpReminderJob(this);
    connect(job, &FollowUpReminderJob::finished, this, &FollowUpReminderManager::slotCheckFollowUpFinished);
    job->setItem(item);
    job->start();
}

void FollowUpReminderManager::slotCheckFollowUpFinished(const QString &messageId, Akonadi::Item::Id id)
{
    for (FollowUpReminderInfo *info : std::as_const(mFollowUpReminderInfoList)) {
        qCDebug(FOLLOWUPREMINDERAGENT_LOG) << "FollowUpReminderManager::slotCheckFollowUpFinished info:" << info;
        if (!info) {
            continue;
        }
        if (info->messageId() == messageId) {
            info->setAnswerMessageItemId(id);
            info->setAnswerWasReceived(true);
            answerReceived(info->to());
            if (info->todoId() != -1) {
                auto job = new FollowUpReminderFinishTaskJob(info->todoId(), this);
                connect(job, &FollowUpReminderFinishTaskJob::finishTaskDone, this, &FollowUpReminderManager::slotFinishTaskDone);
                connect(job, &FollowUpReminderFinishTaskJob::finishTaskFailed, this, &FollowUpReminderManager::slotFinishTaskFailed);
                job->start();
            }
            // Save item
            FollowUpReminder::FollowUpReminderUtil::writeFollowupReminderInfo(FollowUpReminder::FollowUpReminderUtil::defaultConfig(), info, true);
            break;
        }
    }
}

void FollowUpReminderManager::slotFinishTaskDone()
{
    qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " Task Done";
}

void FollowUpReminderManager::slotFinishTaskFailed()
{
    qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " Task Failed";
}

void FollowUpReminderManager::answerReceived(const QString &from)
{
    KNotification::event(u"mailreceived"_s,
                         QString(),
                         i18n("Answer from %1 received", from),
                         u"kmail"_s,
                         KNotification::CloseOnTimeout,
                         u"akonadi_followupreminder_agent"_s);
}

QString FollowUpReminderManager::printDebugInfo() const
{
    QString infoStr;
    if (mFollowUpReminderInfoList.isEmpty()) {
        // Don't translate it. => debug info.
        infoStr = u"No mail"_s;
    } else {
        for (FollowUpReminder::FollowUpReminderInfo *info : std::as_const(mFollowUpReminderInfoList)) {
            if (!infoStr.isEmpty()) {
                infoStr += u'\n';
            }
            infoStr += infoToStr(info);
        }
    }
    return infoStr;
}

QString FollowUpReminderManager::infoToStr(FollowUpReminder::FollowUpReminderInfo *info) const
{
    QString infoStr = u"****************************************"_s;
    infoStr += u"Akonadi Item id :%1\n"_s.arg(info->originalMessageItemId());
    infoStr += u"MessageId :%1\n"_s.arg(info->messageId());
    infoStr += u"Subject :%1\n"_s.arg(info->subject());
    infoStr += u"To :%1\n"_s.arg(info->to());
    infoStr += u"Deadline :%1\n"_s.arg(info->followUpReminderDate().toString());
    infoStr += u"Answer received :%1\n"_s.arg(info->answerWasReceived() ? u"true"_s : u"false"_s);
    infoStr += u"****************************************\n"_s;
    return infoStr;
}

#include "moc_followupremindermanager.cpp"
