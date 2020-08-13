/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "displaymessageformatactionmenu.h"

#include <KLocalizedString>
#include <QAction>
#include <QMenu>
#include <KToggleAction>

DisplayMessageFormatActionMenu::DisplayMessageFormatActionMenu(QObject *parent)
    : KActionMenu(parent)
{
    setText(i18n("Message Default Format"));
    delete menu();
    QMenu *subMenu = new QMenu;
    setMenu(subMenu);

    QActionGroup *actionGroup = new QActionGroup(this);

    KToggleAction *act = new KToggleAction(i18n("Prefer &HTML to Plain Text"), this);
    act->setObjectName(QStringLiteral("prefer-html-action"));
    act->setData(MessageViewer::Viewer::Html);
    actionGroup->addAction(act);
    subMenu->addAction(act);

    act = new KToggleAction(i18n("Prefer &Plain Text to HTML"), this);
    act->setData(MessageViewer::Viewer::Text);
    act->setObjectName(QStringLiteral("prefer-text-action"));
    actionGroup->addAction(act);
    subMenu->addAction(act);

    act = new KToggleAction(i18n("Use Global Setting"), this);
    act->setObjectName(QStringLiteral("use-global-setting-action"));
    act->setData(MessageViewer::Viewer::UseGlobalSetting);
    actionGroup->addAction(act);
    subMenu->addAction(act);
    connect(actionGroup, &QActionGroup::triggered, this, &DisplayMessageFormatActionMenu::slotChangeDisplayMessageFormat);
    updateMenu();
}

DisplayMessageFormatActionMenu::~DisplayMessageFormatActionMenu()
{
}

void DisplayMessageFormatActionMenu::slotChangeDisplayMessageFormat(QAction *act)
{
    MessageViewer::Viewer::DisplayFormatMessage format = static_cast<MessageViewer::Viewer::DisplayFormatMessage>(act->data().toInt());
    if (format != mDisplayMessageFormat) {
        mDisplayMessageFormat = format;
        Q_EMIT changeDisplayMessageFormat(format);
    }
}

MessageViewer::Viewer::DisplayFormatMessage DisplayMessageFormatActionMenu::displayMessageFormat() const
{
    return mDisplayMessageFormat;
}

void DisplayMessageFormatActionMenu::setDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage displayMessageFormat)
{
    if (mDisplayMessageFormat != displayMessageFormat) {
        mDisplayMessageFormat = displayMessageFormat;
        updateMenu();
    }
}

void DisplayMessageFormatActionMenu::updateMenu()
{
    const QList<QAction *> actList = menu()->actions();
    for (QAction *act : actList) {
        MessageViewer::Viewer::DisplayFormatMessage format = static_cast<MessageViewer::Viewer::DisplayFormatMessage>(act->data().toInt());
        if (format == mDisplayMessageFormat) {
            act->setChecked(true);
            break;
        }
    }
}
