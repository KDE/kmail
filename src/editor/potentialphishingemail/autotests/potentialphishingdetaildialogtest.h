/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGDETAILDIALOGTEST_H
#define POTENTIALPHISHINGDETAILDIALOGTEST_H

#include <QObject>

class PotentialPhishingDetailDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailDialogTest(QObject *parent = nullptr);
    ~PotentialPhishingDetailDialogTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void initTestCase();
};

#endif // POTENTIALPHISHINGDETAILDIALOGTEST_H
