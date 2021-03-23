/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <QObject>

class PotentialPhishingEmailJobTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailJobTest(QObject *parent = nullptr);
    ~PotentialPhishingEmailJobTest() override;

private Q_SLOTS:
    void shouldNotStartIfNoEmails();
    void shouldReturnPotentialPhishingEmails_data();
    void shouldReturnPotentialPhishingEmails();
    void shouldEmitSignal();
    void shouldCreateCorrectListOfEmails_data();
    void shouldCreateCorrectListOfEmails();
};

