/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ExternalEditorWarningTest : public QObject
{
    Q_OBJECT
public:
    explicit ExternalEditorWarningTest(QObject *parent = nullptr);
    ~ExternalEditorWarningTest() = default;
private Q_SLOTS:
    void shouldHaveDefaultValue();
};

