/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemenu.h"
#include <KLocalizedString>
#include <QActionGroup>
#include <QMenu>

SnoozeMenu::SnoozeMenu(QObject *parent)
    : KActionMenu{parent}
{
    setText(i18nc("@action:inmenu", "Snooze"));
    QMenu *subMenu = menu();
    auto actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(false);

    auto act = new QAction(i18nc("@action", "One hour later"), this);
    act->setData(QVariant::fromValue(SnoozeMenu::SnoozeType::OneHourLater));
    actionGroup->addAction(act);
    subMenu->addAction(act);

    act = new QAction(i18nc("@action", "Eight hours later"), this);
    act->setData(QVariant::fromValue(SnoozeMenu::SnoozeType::EightHoursLater));
    actionGroup->addAction(act);
    subMenu->addAction(act);

    act = new QAction(i18nc("@action", "Tomorrow"), this);
    act->setData(QVariant::fromValue(SnoozeMenu::SnoozeType::Tomorrow));
    actionGroup->addAction(act);
    subMenu->addAction(act);

    act = new QAction(i18nc("@action", "Next week"), this);
    act->setData(QVariant::fromValue(SnoozeMenu::SnoozeType::NextWeek));
    actionGroup->addAction(act);
    subMenu->addAction(act);

    act = new QAction(i18nc("@action", "Custom…"), this);
    act->setData(QVariant::fromValue(SnoozeMenu::SnoozeType::Custom));
    actionGroup->addAction(act);
    subMenu->addAction(act);

    connect(actionGroup, &QActionGroup::triggered, this, &SnoozeMenu::slotSnoozeRequested);
}

SnoozeMenu::~SnoozeMenu() = default;

void SnoozeMenu::slotSnoozeRequested(QAction *act)
{
    const SnoozeMenu::SnoozeType selectedType = act->data().value<SnoozeMenu::SnoozeType>();
    Q_EMIT snoozeRequested(selectedType);
}

#include "moc_snoozemenu.cpp"
