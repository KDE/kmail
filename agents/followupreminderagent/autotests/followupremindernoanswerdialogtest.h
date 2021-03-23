/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class FollowupReminderNoAnswerDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit FollowupReminderNoAnswerDialogTest(QObject *parent = nullptr);
    ~FollowupReminderNoAnswerDialogTest();
private Q_SLOTS:
    void shouldHaveDefaultValues();
    void shouldAddItemInTreeList();
    void shouldItemHaveInfo();
    void initTestCase();
};

