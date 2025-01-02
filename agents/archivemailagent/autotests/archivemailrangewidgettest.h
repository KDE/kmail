/*
   SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ArchiveMailRangeWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveMailRangeWidgetTest(QObject *parent = nullptr);
    ~ArchiveMailRangeWidgetTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};
