/*
   SPDX-FileCopyrightText: 2014-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QDate>

class KConfigGroup;
namespace FollowUpReminder
{
/** Follow up reminder information. */
class FollowUpReminderInfo
{
public:
    FollowUpReminderInfo();
    explicit FollowUpReminderInfo(const KConfigGroup &config);
    explicit FollowUpReminderInfo(const FollowUpReminderInfo &info);

    // Can be invalid.
    [[nodiscard]] Akonadi::Item::Id originalMessageItemId() const;
    void setOriginalMessageItemId(Akonadi::Item::Id value);

    [[nodiscard]] Akonadi::Item::Id todoId() const;
    void setTodoId(Akonadi::Item::Id value);

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] QString messageId() const;
    void setMessageId(const QString &messageId);

    void setTo(const QString &to);
    [[nodiscard]] QString to() const;

    [[nodiscard]] QDate followUpReminderDate() const;
    void setFollowUpReminderDate(QDate followUpReminderDate);

    void writeConfig(KConfigGroup &config, qint32 identifier);

    [[nodiscard]] QString subject() const;
    void setSubject(const QString &subject);

    [[nodiscard]] bool operator==(const FollowUpReminderInfo &other) const;

    [[nodiscard]] bool answerWasReceived() const;
    void setAnswerWasReceived(bool answerWasReceived);

    [[nodiscard]] Akonadi::Item::Id answerMessageItemId() const;
    void setAnswerMessageItemId(Akonadi::Item::Id answerMessageItemId);

    [[nodiscard]] qint32 uniqueIdentifier() const;
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
