/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#include <KActionMenu>
class QAction;

class KMAILTESTS_TESTS_EXPORT SnoozeMenu : public KActionMenu
{
    Q_OBJECT
public:
    enum class SnoozeType : uint8_t {
        Unknown = 0,
        OneHourLater,
        HeightHourLater,
        Tomorrow,
        NextWeek,
        Custom,
    };

    explicit SnoozeMenu(QObject *parent = nullptr);
    ~SnoozeMenu() override;

Q_SIGNALS:
    void snoozeRequested(SnoozeMenu::SnoozeType type);

private:
    KMAIL_NO_EXPORT void slotSnoozeRequested(QAction *act);
};
