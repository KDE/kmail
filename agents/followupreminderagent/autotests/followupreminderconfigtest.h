/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KSharedConfig>
#include <QObject>
#include <QRegularExpression>

class FollowUpReminderConfigTest : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderConfigTest(QObject *parent = nullptr);
    ~FollowUpReminderConfigTest();
private Q_SLOTS:
    void init();
    void cleanup();
    void cleanupTestCase();
    void shouldConfigBeEmpty();
    void shouldAddAnItem();
    void shouldNotAddAnInvalidItem();
    void shouldReplaceItem();
    void shouldAddSeveralItem();
    void shouldRemoveItems();
    void shouldNotRemoveItemWhenListIsEmpty();
    void shouldNotRemoveItemWhenItemDoesntExist();

private:
    KSharedConfig::Ptr mConfig;
    QRegularExpression mFollowupRegExpFilter;
};

