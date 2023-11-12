/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "commandlineinfotest.h"
#include "commandlineinfo.h"
#include <QTest>

QTEST_GUILESS_MAIN(CommandLineInfoTest)
CommandLineInfoTest::CommandLineInfoTest(QObject *parent)
    : QObject{parent}
{
}

void CommandLineInfoTest::shouldHaveDefaultValues()
{
    CommandLineInfo w;
    // TODO
}
