/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class DisplayMessageFormatActionMenuTest : public QObject
{
    Q_OBJECT
public:
    explicit DisplayMessageFormatActionMenuTest(QObject *parent = nullptr);
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldEmitSignalWhenClickOnSubMenu();
    void shouldSelectItemWhenChangeFormat();
    void shouldDontEmitSignalWhenChangeFormat();
};
