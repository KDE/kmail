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

#ifndef FOLLOWUPREMINDERCONFIGTEST_H
#define FOLLOWUPREMINDERCONFIGTEST_H

#include <QObject>
#include <QRegularExpression>
#include <KSharedConfig>

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

#endif // FOLLOWUPREMINDERCONFIGTEST_H
