/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

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

#include "refreshsettingsassistant.h"
#include "refreshsettingscleanuppage.h"
#include "refreshsettingsfirstpage.h"
#include "refreshsettringsfinishpage.h"
#include <AkonadiWidgets/controlgui.h>
#include <KHelpMenu>
#include <KLocalizedString>
#include <KAboutData>
#include <QMenu>
#include <QPushButton>

RefreshSettingsAssistant::RefreshSettingsAssistant(QWidget *parent)
    : KAssistantDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("KMail Refresh Settings"));
    setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    resize(640, 480);
    Akonadi::ControlGui::widgetNeedsAkonadi(this);
    initializePages();
    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    //Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    button(QDialogButtonBox::Help)->setMenu(menu);
}

RefreshSettingsAssistant::~RefreshSettingsAssistant()
{
}

void RefreshSettingsAssistant::initializePages()
{
    mFirstPage = new RefreshSettingsFirstPage(this);
    mFirstPageItem = new KPageWidgetItem(mFirstPage, i18n("Warning"));
    addPage(mFirstPageItem);

    mCleanUpPage = new RefreshSettingsCleanupPage(this);
    mCleanUpPageItem = new KPageWidgetItem(mCleanUpPage, i18n("Clean up Settings"));
    addPage(mCleanUpPageItem);

    mFinishPage = new RefreshSettringsFinishPage(this);
    mFinishPageItem = new KPageWidgetItem(mFinishPage, i18n("Finish"));
    addPage(mFinishPageItem);

    connect(mCleanUpPage, &RefreshSettingsCleanupPage::cleanDoneInfo, mFinishPage, &RefreshSettringsFinishPage::cleanDoneInfo);
}
