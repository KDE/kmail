/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDialog>
namespace FollowUpReminder
{
class FollowUpReminderInfo;
}
class FollowUpReminderInfoWidget;
class FollowUpReminderNoAnswerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FollowUpReminderNoAnswerDialog(QWidget *parent = nullptr);
    ~FollowUpReminderNoAnswerDialog() override;

    void setInfo(const QList<FollowUpReminder::FollowUpReminderInfo *> &info);

    void wakeUp();
Q_SIGNALS:
    void needToReparseConfiguration();

private:
    void slotDBusNotificationsPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void slotSave();
    void readConfig();
    void writeConfig();
    FollowUpReminderInfoWidget *mWidget = nullptr;
};

