/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FOLLOWUPREMINDERNOANSWERDIALOGTEST_H
#define FOLLOWUPREMINDERNOANSWERDIALOGTEST_H

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

#endif // FOLLOWUPREMINDERNOANSWERDIALOGTEST_H
