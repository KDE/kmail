/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#include "historyclosedreadermenu.h"
#include "historyclosedreadermanager.h"
#include "kmail_debug.h"
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
    if (mReopenAction) {
        menu()->removeAction(mReopenAction);
    }
    if (mSeparatorAction) {
        menu()->removeAction(mSeparatorAction);
    }
    menu()->clear();
    const QList<HistoryClosedReaderInfo> list = HistoryClosedReaderManager::self()->closedReaderInfos();
    if (!list.isEmpty()) {
        addReOpenClosedAction();
        for (const auto &info : list) {
            QString subject = info.subject();
            const QString originalSubject{subject};
            if (subject.length() > 61) {
                subject = subject.first(60) + QStringLiteral("…");
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

void HistoryClosedReaderMenu::slotReopenLastClosedViewer()
{
    const QList<HistoryClosedReaderInfo> list = HistoryClosedReaderManager::self()->closedReaderInfos();
    if (!list.isEmpty()) {
        const auto identifier = list.constFirst().item();
        Q_EMIT openMessage(identifier);
        HistoryClosedReaderManager::self()->removeItem(identifier);
    }
}

void HistoryClosedReaderMenu::createReOpenClosedAction()
{
    if (!mReopenAction) {
        mReopenAction = new QAction(i18nc("@action", "Reopen Closed Viewer"), this);
        menu()->addAction(mReopenAction);
        connect(mReopenAction, &QAction::triggered, this, &HistoryClosedReaderMenu::slotReopenLastClosedViewer);

        mSeparatorAction = new QAction(this);
        mSeparatorAction->setSeparator(true);
    }
}

void HistoryClosedReaderMenu::addReOpenClosedAction()
{
    if (!mReopenAction) {
        qCWarning(KMAIL_LOG) << "mReopenAction was not created ! It's a bug";
        return;
    }
    menu()->addAction(mReopenAction);
    menu()->addAction(mSeparatorAction);
}

QAction *HistoryClosedReaderMenu::reopenAction() const
{
    return mReopenAction;
}

#include "moc_historyclosedreadermenu.cpp"
