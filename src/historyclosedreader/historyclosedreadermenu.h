/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <KActionMenu>

class HistoryClosedReaderMenu : public KActionMenu
{
    Q_OBJECT
public:
    explicit HistoryClosedReaderMenu(QObject *parent = nullptr);
    ~HistoryClosedReaderMenu() override;
};
