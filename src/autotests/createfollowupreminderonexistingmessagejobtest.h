/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class CreateFollowupReminderOnExistingMessageJobTest : public QObject
{
    Q_OBJECT
public:
    explicit CreateFollowupReminderOnExistingMessageJobTest(QObject *parent = nullptr);
    ~CreateFollowupReminderOnExistingMessageJobTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldValidBeforeStartJob();
};

