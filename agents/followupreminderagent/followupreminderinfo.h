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

#ifndef FOLLOWUPREMINDERINFO_H
#define FOLLOWUPREMINDERINFO_H

#include <AkonadiCore/Item>
#include <QDate>

class KConfigGroup;
namespace FollowUpReminder {
/** Follow up reminder information. */
class FollowUpReminderInfo
{
public:
    FollowUpReminderInfo();
    FollowUpReminderInfo(const KConfigGroup &config);
    FollowUpReminderInfo(const FollowUpReminderInfo &info);

    //Can be invalid.
    Q_REQUIRED_RESULT Akonadi::Item::Id originalMessageItemId() const;
    void setOriginalMessageItemId(Akonadi::Item::Id value);

    Q_REQUIRED_RESULT Akonadi::Item::Id todoId() const;
    void setTodoId(Akonadi::Item::Id value);

    Q_REQUIRED_RESULT bool isValid() const;

    Q_REQUIRED_RESULT QString messageId() const;
    void setMessageId(const QString &messageId);

    void setTo(const QString &to);
    Q_REQUIRED_RESULT QString to() const;

    Q_REQUIRED_RESULT QDate followUpReminderDate() const;
    void setFollowUpReminderDate(QDate followUpReminderDate);

    void writeConfig(KConfigGroup &config, qint32 identifier);

    Q_REQUIRED_RESULT QString subject() const;
    void setSubject(const QString &subject);

    bool operator ==(const FollowUpReminderInfo &other) const;

    Q_REQUIRED_RESULT bool answerWasReceived() const;
    void setAnswerWasReceived(bool answerWasReceived);

    Q_REQUIRED_RESULT Akonadi::Item::Id answerMessageItemId() const;
    void setAnswerMessageItemId(Akonadi::Item::Id answerMessageItemId);

    Q_REQUIRED_RESULT qint32 uniqueIdentifier() const;
    void setUniqueIdentifier(qint32 uniqueIdentifier);

private:
    void readConfig(const KConfigGroup &config);
    Akonadi::Item::Id mOriginalMessageItemId = -1;
    Akonadi::Item::Id mAnswerMessageItemId = -1;
    Akonadi::Item::Id mTodoId = -1;
    QString mMessageId;
    QDate mFollowUpReminderDate;
    QString mTo;
    QString mSubject;
    qint32 mUniqueIdentifier = -1;
    bool mAnswerWasReceived = false;
};
}

QDebug operator<<(QDebug debug, const FollowUpReminder::FollowUpReminderInfo &info);

#endif // FOLLOWUPREMINDERINFO_H
