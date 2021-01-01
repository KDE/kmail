/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef REFRESHSETTINGSFIRSTPAGE_H
#define REFRESHSETTINGSFIRSTPAGE_H

#include <QWidget>

class RefreshSettingsFirstPage : public QWidget
{
    Q_OBJECT
public:
    explicit RefreshSettingsFirstPage(QWidget *parent = nullptr);
    ~RefreshSettingsFirstPage();
};

#endif // REFRESHSETTINGSFIRSTPAGE_H
