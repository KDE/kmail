/*
   Copyright (C) 2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "externaleditorwarningtest.h"
#include "../externaleditorwarning.h"
#include <QHBoxLayout>
#include <QTest>

QTEST_MAIN(ExternalEditorWarningTest)

ExternalEditorWarningTest::ExternalEditorWarningTest(QObject *parent)
    : QObject(parent)
{
}

void ExternalEditorWarningTest::shouldHaveDefaultValue()
{
    QWidget *wid = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(wid);
    ExternalEditorWarning w;
    layout->addWidget(&w);
    wid->show();
    QVERIFY(!w.isVisible());
    //QVERIFY(w.isCloseButtonVisible());
    QCOMPARE(w.messageType(), KMessageWidget::Information);
    QVERIFY(w.wordWrap());
    QVERIFY(!w.text().isEmpty());
}
