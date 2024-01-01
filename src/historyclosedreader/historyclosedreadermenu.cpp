/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
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
            QString subject = info.subject();
            const QString originalSubject{subject};
            if (subject.length() > 61) {
                subject = subject.first(60) + QStringLiteral("...");
            }
            auto action = new QAction(subject, menu());
            action->setToolTip(originalSubject);
            connect(action, &QAction::triggered, this, [this, info]() {
                const auto identifier = info.item();
                Q_EMIT openMessage(identifier);
                HistoryClosedReaderManager::self()->removeItem(identifier);
            });
            menu()->addAction(action);
        }
        menu()->addSeparator();
        auto clearAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-clear-history")), i18n("Clear History"), menu());
        connect(clearAction, &QAction::triggered, this, &HistoryClosedReaderMenu::slotClear);
        menu()->addAction(clearAction);
    }
}

#include "moc_historyclosedreadermenu.cpp"
