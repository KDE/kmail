/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGEMAILJOBTEST_H
#define POTENTIALPHISHINGEMAILJOBTEST_H

#include <QObject>

class PotentialPhishingEmailJobTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailJobTest(QObject *parent = nullptr);
    ~PotentialPhishingEmailJobTest();

private Q_SLOTS:
    void shouldNotStartIfNoEmails();
    void shouldReturnPotentialPhishingEmails_data();
    void shouldReturnPotentialPhishingEmails();
    void shouldEmitSignal();
    void shouldCreateCorrectListOfEmails_data();
    void shouldCreateCorrectListOfEmails();
};

#endif // POTENTIALPHISHINGEMAILJOBTEST_H
