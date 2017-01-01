/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "displaymessageformatactionmenu.h"

#include <KLocalizedString>
#include <QAction>
#include <QMenu>
#include <KToggleAction>

DisplayMessageFormatActionMenu::DisplayMessageFormatActionMenu(QObject *parent)
    : KActionMenu(parent),
      mDisplayMessageFormat(MessageViewer::Viewer::UseGlobalSetting)
{
    setText(i18n("Message Default Format"));
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
    Q_FOREACH (QAction *act, menu()->actions()) {
        MessageViewer::Viewer::DisplayFormatMessage format = static_cast<MessageViewer::Viewer::DisplayFormatMessage>(act->data().toInt());
        if (format == mDisplayMessageFormat) {
            act->setChecked(true);
            break;
        }
    }
}
