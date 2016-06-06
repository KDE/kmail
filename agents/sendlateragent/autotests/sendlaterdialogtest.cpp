/*
   Copyright (C) 2015-2016 Montel Laurent <montel@kde.org>

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

#include "sendlaterdialogtest.h"
#include "sendlaterdialog.h"
#include <KTimeComboBox>
#include <KDateComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <qtest.h>
#include <QStandardPaths>
SendLaterDialogTest::SendLaterDialogTest(QObject *parent)
    : QObject(parent)
{

}

SendLaterDialogTest::~SendLaterDialogTest()
{

}

void SendLaterDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void SendLaterDialogTest::shouldHaveDefaultValue()
{
    SendLater::SendLaterDialog dlg(0);
    KTimeComboBox *timeCombo = dlg.findChild<KTimeComboBox *>(QStringLiteral("time_sendlater"));
    QVERIFY(timeCombo);
    KDateComboBox *dateCombo = dlg.findChild<KDateComboBox *>(QStringLiteral("date_sendlater"));
    QVERIFY(dateCombo);
    QVERIFY(dateCombo->date().isValid());
    QPushButton *okButton = dlg.findChild<QPushButton *>(QStringLiteral("okbutton"));
    QVERIFY(okButton);
    QVERIFY(okButton->isEnabled());
    dateCombo->lineEdit()->clear();
    QVERIFY(!dateCombo->date().isValid());
    QVERIFY(!okButton->isEnabled());
}

QTEST_MAIN(SendLaterDialogTest)
