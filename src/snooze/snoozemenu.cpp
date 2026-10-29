/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemenu.h"
#include <KLocalizedString>
#include <QActionGroup>
#include <QMenu>
#include <QTimeZone>

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

QDateTime SnoozeMenu::wakeUpDateTime(SnoozeType type, const QDateTime &now)
{
    // Messages snoozed until a later day pop back up at the start of the day.
    const QTime morning(8, 0);
    switch (type) {
    case SnoozeType::OneHourLater:
        return now.addSecs(3600);
    case SnoozeType::EightHoursLater:
        return now.addSecs(8 * 3600);
    case SnoozeType::Tomorrow:
        return {now.date().addDays(1), morning, now.timeZone()};
    case SnoozeType::NextWeek:
        // Next Monday, a week later when today is already Monday.
        return {now.date().addDays(8 - now.date().dayOfWeek()), morning, now.timeZone()};
    case SnoozeType::Unknown:
    case SnoozeType::Custom:
        break;
    }
    return {};
}

void SnoozeMenu::slotSnoozeRequested(QAction *act)
{
    const SnoozeMenu::SnoozeType selectedType = act->data().value<SnoozeMenu::SnoozeType>();
    Q_EMIT snoozeRequested(selectedType);
}

#include "moc_snoozemenu.cpp"
