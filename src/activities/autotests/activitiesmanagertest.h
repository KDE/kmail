/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ActivitiesManagerTest : public QObject
{
    Q_OBJECT
public:
    explicit ActivitiesManagerTest(QObject *parent = nullptr);
    ~ActivitiesManagerTest() override = default;

private Q_SLOTS:
    void shouldHaveDefaultValues();
};
