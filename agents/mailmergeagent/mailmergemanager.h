/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class MailMergeManager : public QObject
{
    Q_OBJECT
public:
    explicit MailMergeManager(QObject *parent = nullptr);
    ~MailMergeManager() override;
};
