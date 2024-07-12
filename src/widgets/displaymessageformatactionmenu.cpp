/*
   SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "displaymessageformatactionmenu.h"
using namespace Qt::Literals::StringLiterals;

#include <KLocalizedString>
#include <KToggleAction>
#include <QAction>
#include <QActionGroup>
#include <QMenu>

DisplayMessageFormatActionMenu::DisplayMessageFormatActionMenu(QObject *parent)
    : KActionMenu(parent)
{
    setText(i18n("Message Default Format"));
    delete menu();
    auto subMenu = new QMenu;
    setMenu(subMenu);

    auto actionGroup = new QActionGroup(this);

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
}

DisplayMessageFormatActionMenu::~DisplayMessageFormatActionMenu() = default;

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

#include "moc_displaymessageformatactionmenu.cpp"
