/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QObject>

class HistoryClosedReaderManagerTest : public QObject
{
    Q_OBJECT
public:
    explicit HistoryClosedReaderManagerTest(QObject *parent = nullptr);
    ~HistoryClosedReaderManagerTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};
