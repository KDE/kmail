/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "displaymessageformatactionmenu.h"

#include <KLocalizedString>
#include <KAction>
#include <KMenu>
#include <KToggleAction>
#include "messageviewer/viewer/viewer.h"

DisplayMessageFormatActionMenu::DisplayMessageFormatActionMenu(QObject *parent)
    : KActionMenu(parent)
{
    setText(i18n("Message Default Format"));
    KMenu *subMenu = new KMenu;
    setMenu(subMenu);
    QActionGroup *actionGroup = new QActionGroup(this);
    KToggleAction *act = new KToggleAction(i18n("Prefer &HTML to Plain Text"), this);
    act->setData(MessageViewer::Viewer::Html);
    actionGroup->addAction(act);
    subMenu->addAction(act);
    connect(act, SIGNAL(triggered(bool)), this, SLOT(slotChangeDisplayMessageFormat()));

    act = new KToggleAction(i18n("Prefer &Plain Text to HTML"), this);
    act->setData(MessageViewer::Viewer::Text);
    actionGroup->addAction(act);
    subMenu->addAction(act);
    connect(act, SIGNAL(triggered(bool)), this, SLOT(slotChangeDisplayMessageFormat()));

    act = new KToggleAction(i18n("Use KMail global setting"), this);
    act->setData(MessageViewer::Viewer::UseGlobalSetting);
    connect(act, SIGNAL(triggered(bool)), this, SLOT(slotChangeDisplayMessageFormat()));
    actionGroup->addAction(act);
    subMenu->addAction(act);
}


DisplayMessageFormatActionMenu::~DisplayMessageFormatActionMenu()
{

}

void DisplayMessageFormatActionMenu::slotChangeDisplayMessageFormat()
{
    if (sender()) {
        KToggleAction *act = dynamic_cast<KToggleAction *>(sender());
        if (act) {

        }
    }
}
