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
    setText(i18nc("List of message viewer closed", "Closed Reader"));
    delete menu();
    auto subMenu = new QMenu;

    subMenu->addSeparator();
    auto clearAction = new QAction(i18n("Clear History"), this);
    connect(clearAction, &QAction::toggled, this, &HistoryClosedReaderMenu::slotClear);
    subMenu->addAction(clearAction);
    setMenu(subMenu);
    connect(HistoryClosedReaderManager::self(), &HistoryClosedReaderManager::historyClosedReaderChanged, this, &HistoryClosedReaderMenu::updateMenu);
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
