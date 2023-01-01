/*
   SPDX-FileCopyrightText: 2020-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class CreateForwardMessageJobTest : public QObject
{
    Q_OBJECT
public:
    explicit CreateForwardMessageJobTest(QObject *parent = nullptr);
    ~CreateForwardMessageJobTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};
