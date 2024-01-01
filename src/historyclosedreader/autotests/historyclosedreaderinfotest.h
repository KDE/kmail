/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
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
