/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef TAGSELECTDIALOGTEST_H
#define TAGSELECTDIALOGTEST_H

#include <QObject>

class TagSelectDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit TagSelectDialogTest(QObject *parent = nullptr);
    ~TagSelectDialogTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void initTestCase();
};

#endif // TAGSELECTDIALOGTEST_H
