/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>
#include <QDateTime>

/**
 * Marks a message as snoozed until a given date/time.
 *
 * The attribute is the source of truth: the message list proxy hides any item
 * carrying it whose wake-up date is still in the future. Keeping the date in
 * the predicate (rather than a plain boolean) means a snoozed message
 * reappears on its own once the deadline has passed, even if the agent is
 * disabled or was never started.
 *
 * The date is all there is to store. The folder the message sits in is never
 * changed by snoozing, so it stays readable from the item itself, and whether
 * to mark the message unread on wake-up is a global preference
 * (SnoozeAgentSettings::markAsUnreadOnWakeUp), not a per-message one.
 */
class SnoozeAttribute : public Akonadi::Attribute
{
public:
    SnoozeAttribute();
    ~SnoozeAttribute() override;

    [[nodiscard]] QByteArray type() const override;
    [[nodiscard]] SnoozeAttribute *clone() const override;
    [[nodiscard]] QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

    /** Wake-up instant, always stored in UTC. */
    [[nodiscard]] QDateTime wakeUpDateTime() const;
    void setWakeUpDateTime(const QDateTime &dateTime);

    [[nodiscard]] bool operator==(const SnoozeAttribute &other) const;

private:
    QDateTime mWakeUpDateTime;
};
