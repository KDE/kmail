/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettingsassistant.h"
#include "refreshsettingscleanuppage.h"
#include "refreshsettingsfirstpage.h"
#include "refreshsettringsfinishpage.h"
#include <AkonadiWidgets/controlgui.h>
#include <KAboutData>
#include <KHelpMenu>
#include <KLocalizedString>
#include <QMenu>
#include <QPushButton>

RefreshSettingsAssistant::RefreshSettingsAssistant(QWidget *parent)
    : KAssistantDialog(parent)
{
    setModal(true);
    setWindowTitle(i18nc("@title:window", "KMail Refresh Settings"));
    setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    resize(640, 480);
    Akonadi::ControlGui::widgetNeedsAkonadi(this);
    initializePages();
    auto helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    // Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    button(QDialogButtonBox::Help)->setMenu(menu);
}

RefreshSettingsAssistant::~RefreshSettingsAssistant() = default;

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
    connect(mCleanUpPage, &RefreshSettingsCleanupPage::cleanUpDone, this, &RefreshSettingsAssistant::cleanUpDone);
}

void RefreshSettingsAssistant::cleanUpDone()
{
    next();
}
