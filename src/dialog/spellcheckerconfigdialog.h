/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Sonnet/ConfigDialog>

class SpellCheckerConfigDialog : public Sonnet::ConfigDialog
{
    Q_OBJECT
public:
    explicit SpellCheckerConfigDialog(QWidget *parent = nullptr);
    ~SpellCheckerConfigDialog() override;

private:
    void readConfig();
    void writeConfig();
};

