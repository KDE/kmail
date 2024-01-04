/*
   SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class FolderArchiveAccountInfoTest : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveAccountInfoTest(QObject *parent = nullptr);
    ~FolderArchiveAccountInfoTest() override;

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldBeValid();
    void shouldRestoreFromSettings();
};
