/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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

