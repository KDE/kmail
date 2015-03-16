/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kactionmenuchangecasetest.h"
#include "../widgets/kactionmenuchangecase.h"
#include <qtest.h>
#include <QSignalSpy>
#include <QMenu>

KActionMenuChangeCaseTest::KActionMenuChangeCaseTest(QObject *parent)
    : QObject(parent)
{

}

KActionMenuChangeCaseTest::~KActionMenuChangeCaseTest()
{

}

void KActionMenuChangeCaseTest::shouldHaveDefaultValue()
{
    KActionMenuChangeCase menu;
    QVERIFY(menu.upperCaseAction());
    QVERIFY(menu.sentenceCaseAction());
    QVERIFY(menu.lowerCaseAction());
    QCOMPARE(menu.menu()->actions().count(), 3);
}

void KActionMenuChangeCaseTest::shouldEmitSignals()
{
    KActionMenuChangeCase menu;
    QSignalSpy spyUpper(&menu, SIGNAL(upperCase()));
    QSignalSpy spyLower(&menu, SIGNAL(lowerCase()));
    QSignalSpy spySentence(&menu, SIGNAL(sentenceCase()));
    menu.upperCaseAction()->trigger();
    menu.lowerCaseAction()->trigger();
    menu.sentenceCaseAction()->trigger();
    QCOMPARE(spyUpper.count(), 1);
    QCOMPARE(spyLower.count(), 1);
    QCOMPARE(spySentence.count(), 1);
}

QTEST_MAIN(KActionMenuChangeCaseTest)
