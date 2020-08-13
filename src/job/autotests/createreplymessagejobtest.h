/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CREATEREPLYMESSAGEJOBTEST_H
#define CREATEREPLYMESSAGEJOBTEST_H

#include <QObject>

class CreateReplyMessageJobTest : public QObject
{
    Q_OBJECT
public:
    explicit CreateReplyMessageJobTest(QObject *parent = nullptr);
    ~CreateReplyMessageJobTest() = default;
private Q_SLOTS:
    void shouldHaveDefaultValue();
};

#endif // CREATEREPLYMESSAGEJOBTEST_H
