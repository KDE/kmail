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
    setText(i18n("Snooze"));
    QMenu *subMenu = menu();
    auto actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(false);

    /*
        auto act = new KToggleAction(i18nc("@action", "Prefer &HTML to Plain Text"), this);
        act->setObjectName("prefer-html-action"_L1);
        act->setData(MessageViewer::Viewer::Html);
        actionGroup->addAction(act);
        subMenu->addAction(act);

        act = new KToggleAction(i18nc("@action", "Prefer &Plain Text to HTML"), this);
        act->setData(MessageViewer::Viewer::Text);
        act->setObjectName("prefer-text-action"_L1);
        actionGroup->addAction(act);
        subMenu->addAction(act);

        act = new KToggleAction(i18nc("@action", "Use Global Setting"), this);
        act->setObjectName("use-global-setting-action"_L1);
        act->setData(MessageViewer::Viewer::UseGlobalSetting);
        actionGroup->addAction(act);
        subMenu->addAction(act);
        connect(actionGroup, &QActionGroup::triggered, this, &DisplayMessageFormatActionMenu::slotChangeDisplayMessageFormat);
        updateMenu();
    */
    connect(actionGroup, &QActionGroup::triggered, this, &SnoozeMenu::slotSnoozeRequested);
}

SnoozeMenu::~SnoozeMenu() = default;

void SnoozeMenu::slotSnoozeRequested(QAction *act)
{
    /*
    if (MessageViewer::Viewer::DisplayFormatMessage format = static_cast<MessageViewer::Viewer::DisplayFormatMessage>(act->data().toInt());
        format != mDisplayMessageFormat) {
        mDisplayMessageFormat = format;
        Q_EMIT snoozeRequested(format);
    }
    */
}

#include "moc_snoozemenu.cpp"
