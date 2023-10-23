/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include <QObject>

class HistoryClosedReaderInfoTest : public QObject
{
    Q_OBJECT
public:
    explicit HistoryClosedReaderInfoTest(QObject *parent = nullptr);
    ~HistoryClosedReaderInfoTest() override = default;

private Q_SLOTS:
    void shouldHaveDefaultValues();
};
