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
    menu()->clear();
    const QList<HistoryClosedReaderInfo> list = HistoryClosedReaderManager::self()->closedReaderInfos();
    if (!list.isEmpty()) {
        for (const auto &info : list) {
            auto action = new QAction(info.subject(), menu());
            connect(action, &QAction::triggered, this, [this, info]() {
                Q_EMIT openMessage(info.item());
            });
            menu()->addAction(action);
        }
        menu()->addSeparator();
        auto clearAction = new QAction(i18n("Clear History"), menu());
        connect(clearAction, &QAction::triggered, this, &HistoryClosedReaderMenu::slotClear);
        menu()->addAction(clearAction);
    }
}

#include "moc_historyclosedreadermenu.cpp"
