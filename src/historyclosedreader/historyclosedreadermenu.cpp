/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#include "historyclosedreadermenu.h"
#include "historyclosedreadermanager.h"
#include <KLocalizedString>
#include <QMenu>

HistoryClosedReaderMenu::HistoryClosedReaderMenu(QObject *parent)
    : KActionMenu{parent}
{
    setText(i18n("Closed Reader"));
    delete menu();
    auto subMenu = new QMenu;
    setMenu(subMenu);
    // TODO add clear
    // TODO add all menu entries
}

HistoryClosedReaderMenu::~HistoryClosedReaderMenu() = default;

void HistoryClosedReaderMenu::slotClear()
{
    HistoryClosedReaderManager::self()->clear();
}

void HistoryClosedReaderMenu::updateMenu()
{
    // TODO
}

#include "moc_historyclosedreadermenu.cpp"
