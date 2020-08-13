/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef FOLDERARCHIVEACCOUNTINFOTEST_H
#define FOLDERARCHIVEACCOUNTINFOTEST_H

#include <QObject>

class FolderArchiveAccountInfoTest : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveAccountInfoTest(QObject *parent = nullptr);
    ~FolderArchiveAccountInfoTest();

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldBeValid();
    void shouldRestoreFromSettings();
};

#endif // FOLDERARCHIVEACCOUNTINFOTEST_H
