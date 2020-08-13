/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ARCHIVEMAILINFOTEST_H
#define ARCHIVEMAILINFOTEST_H

#include <QObject>

class ArchiveMailInfoTest : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveMailInfoTest(QObject *parent = nullptr);
    ~ArchiveMailInfoTest();

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldCopyArchiveInfo();
    void shouldRestoreFromSettings();
};

#endif // ARCHIVEMAILINFOTEST_H
