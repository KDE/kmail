/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class KActionMenuTransportTest : public QObject
{
    Q_OBJECT
public:
    explicit KActionMenuTransportTest(QObject *parent = nullptr);
    ~KActionMenuTransportTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
};
