/*
   Copyright (C) 2014-2020 Laurent Montel <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERNOANSWERDIALOG_H
#define FOLLOWUPREMINDERNOANSWERDIALOG_H

#include <QDialog>
namespace FollowUpReminder {
class FollowUpReminderInfo;
}
class FollowUpReminderInfoWidget;
class FollowUpReminderNoAnswerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FollowUpReminderNoAnswerDialog(QWidget *parent = nullptr);
    ~FollowUpReminderNoAnswerDialog();

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

#endif // FOLLOWUPREMINDERNOANSWERDIALOG_H
