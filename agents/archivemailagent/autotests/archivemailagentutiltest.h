/*
   SPDX-FileCopyrightText: 2012-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ArchiveMailAgentUtilTest : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveMailAgentUtilTest(QObject *parent = nullptr);
    ~ArchiveMailAgentUtilTest() override = default;

private Q_SLOTS:
    void shouldTestTimeIsInRange();
    void shouldTestTimeIsInRange_data();
};
