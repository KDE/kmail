/*
   SPDX-FileCopyrightText: 2014-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <KSharedConfig>
#include <QObject>
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
    explicit FollowUpReminderManager(QObject *parent = nullptr);
    ~FollowUpReminderManager() override;

    void load(bool forceReloadConfig = false);
    void addReminder(FollowUpReminder::FollowUpReminderInfo *reminder); // takes ownership
    void checkFollowUp(const Akonadi::Item &item, const Akonadi::Collection &col);

    [[nodiscard]] QString printDebugInfo() const;

private:
    void slotCheckFollowUpFinished(const QString &messageId, Akonadi::Item::Id id);

    void slotFinishTaskDone();
    void slotFinishTaskFailed();
    void slotReparseConfiguration();
    void answerReceived(const QString &from);
    [[nodiscard]] QString infoToStr(FollowUpReminder::FollowUpReminderInfo *info) const;

    KSharedConfig::Ptr mConfig;
    QList<FollowUpReminder::FollowUpReminderInfo *> mFollowUpReminderInfoList;
    QPointer<FollowUpReminderNoAnswerDialog> mNoAnswerDialog;
    bool mInitialize = false;
};
