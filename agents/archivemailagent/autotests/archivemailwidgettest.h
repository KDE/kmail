/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ARCHIVEMAILWIDGETTEST_H
#define ARCHIVEMAILWIDGETTEST_H

#include <QObject>

class ArchiveMailWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveMailWidgetTest(QObject *parent = nullptr);
    ~ArchiveMailWidgetTest();

private Q_SLOTS:
    void shouldHaveDefaultValue();
};

#endif // ARCHIVEMAILWIDGETTEST_H
