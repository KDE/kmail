/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class IncorrectIdentityFolderWarningTest : public QObject
{
    Q_OBJECT
public:
    explicit IncorrectIdentityFolderWarningTest(QObject *parent = nullptr);
    ~IncorrectIdentityFolderWarningTest() = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
    void shouldShowWarningInvalidIdentity();
    void shouldShowWarningInvalidMailTransport();
    void shouldShowWarningInvalidFcc();
};

