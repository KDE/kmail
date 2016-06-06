/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERMANAGER_H
#define FOLLOWUPREMINDERMANAGER_H

#include <QObject>
#include <KSharedConfig>
#include <AkonadiCore/Item>
#include <QPointer>
namespace FollowUpReminder
{
class FollowUpReminderInfo;
}
class FollowUpReminderNoAnswerDialog;
class FollowUpReminderManager : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderManager(QObject *parent = Q_NULLPTR);
    ~FollowUpReminderManager();

    void load(bool forceReloadConfig = false);
    void checkFollowUp(const Akonadi::Item &item, const Akonadi::Collection &col);

    QString printDebugInfo();
private Q_SLOTS:
    void slotCheckFollowUpFinished(const QString &messageId, Akonadi::Item::Id id);

    void slotFinishTaskDone();
    void slotFinishTaskFailed();
    void slotReparseConfiguration();
private:
    void answerReceived(const QString &from);
    QString infoToStr(FollowUpReminder::FollowUpReminderInfo *info);

    KSharedConfig::Ptr mConfig;
    QList<FollowUpReminder::FollowUpReminderInfo *> mFollowUpReminderInfoList;
    QPointer<FollowUpReminderNoAnswerDialog> mNoAnswerDialog;
    bool mInitialize;
};

#endif // FOLLOWUPREMINDERMANAGER_H
