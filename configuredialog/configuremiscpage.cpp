/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "configuremiscpage.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "configureagentswidget.h"
#include "settings/globalsettings.h"

#include <mailcommon/folder/folderrequester.h>
#include "messageviewer/widgets/invitationsettings.h"
#include "messageviewer/widgets/printingsettings.h"
#include "messageviewer/settings/globalsettings.h"

#include <KCModuleProxy>
#include <KCModuleInfo>
#include <KLocalizedString>
#include <QDialog>
#include <KConfigGroup>

using namespace MailCommon;
QString MiscPage::helpAnchor() const
{
    return QString::fromLatin1("configure-misc");
}

MiscPage::MiscPage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    mFolderTab = new FolderTab();
    addTab(mFolderTab, i18n("Folders"));

    mInviteTab = new InviteTab();
    addTab(mInviteTab, i18n("Invitations"));

    mProxyTab = new ProxyTab();
    addTab(mProxyTab, i18n("Proxy"));

    mAgentSettingsTab = new MiscPageAgentSettingsTab();
    addTab(mAgentSettingsTab, i18n("Plugins Settings"));

    mPrintingTab = new MiscPagePrintingTab();
    addTab(mPrintingTab, i18n("Printing"));
}

QString MiscPageFolderTab::helpAnchor() const
{
    return QString::fromLatin1("configure-misc-folders");
}

MiscPageFolderTab::MiscPageFolderTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mMMTab.setupUi(this);
    //replace QWidget with FolderRequester. Promote to doesn't work due to the custom constructor
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    mMMTab.mOnStartupOpenFolder->setLayout(layout);
    mOnStartupOpenFolder = new FolderRequester(mMMTab.mOnStartupOpenFolder);
    layout->addWidget(mOnStartupOpenFolder);

//TODO PORT QT5     mMMTab.gridLayout->setSpacing( QDialog::spacingHint() );
//TODO PORT QT5     mMMTab.gridLayout->setMargin( QDialog::marginHint() );
    mMMTab.mExcludeImportantFromExpiry->setWhatsThis(
        i18n(GlobalSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8()));

    connect(mMMTab.mEmptyFolderConfirmCheck, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mExcludeImportantFromExpiry, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mLoopOnGotoUnread, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mActionEnterFolder, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkTime, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)), mMMTab.mDelayedMarkTime, SLOT(setEnabled(bool)));
    connect(mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)), this , SLOT(slotEmitChanged()));
    connect(mMMTab.mShowPopupAfterDnD, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mOnStartupOpenFolder, &MailCommon::FolderRequester::folderChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mEmptyTrashCheck, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, mOnStartupOpenFolder, &MailCommon::FolderRequester::setEnabled);
}

void MiscPage::FolderTab::doLoadFromGlobalSettings()
{
    loadWidget(mMMTab.mExcludeImportantFromExpiry, GlobalSettings::self()->excludeImportantMailFromExpiryItem());
    // default = "Loop in current folder"
    loadWidget(mMMTab.mLoopOnGotoUnread, GlobalSettings::self()->loopOnGotoUnreadItem());
    loadWidget(mMMTab.mActionEnterFolder, GlobalSettings::self()->actionEnterFolderItem());
    loadWidget(mMMTab.mDelayedMarkAsRead, MessageViewer::GlobalSettings::self()->delayedMarkAsReadItem());
    loadWidget(mMMTab.mDelayedMarkTime, MessageViewer::GlobalSettings::self()->delayedMarkTimeItem());
    loadWidget(mMMTab.mShowPopupAfterDnD, GlobalSettings::self()->showPopupAfterDnDItem());
    loadWidget(mMMTab.mStartUpFolderCheck, GlobalSettings::self()->startSpecificFolderAtStartupItem());
    mOnStartupOpenFolder->setEnabled(GlobalSettings::self()->startSpecificFolderAtStartup());
    doLoadOther();
}

void MiscPage::FolderTab::doLoadOther()
{
    loadWidget(mMMTab.mEmptyTrashCheck, GlobalSettings::self()->emptyTrashOnExitItem());
    mOnStartupOpenFolder->setCollection(Akonadi::Collection(GlobalSettings::self()->startupFolder()));
    loadWidget(mMMTab.mEmptyFolderConfirmCheck, GlobalSettings::self()->confirmBeforeEmptyItem());
}

void MiscPage::FolderTab::save()
{
    saveCheckBox(mMMTab.mEmptyTrashCheck, GlobalSettings::self()->emptyTrashOnExitItem());
    saveCheckBox(mMMTab.mEmptyFolderConfirmCheck, GlobalSettings::self()->confirmBeforeEmptyItem());
    saveComboBox(mMMTab.mActionEnterFolder, GlobalSettings::self()->actionEnterFolderItem());
    GlobalSettings::self()->setStartupFolder(mOnStartupOpenFolder->collection().id());

    saveCheckBox(mMMTab.mDelayedMarkAsRead, MessageViewer::GlobalSettings::self()->delayedMarkAsReadItem());
    saveSpinBox(mMMTab.mDelayedMarkTime, MessageViewer::GlobalSettings::self()->delayedMarkTimeItem());
    saveComboBox(mMMTab.mLoopOnGotoUnread, GlobalSettings::self()->loopOnGotoUnreadItem());

    saveCheckBox(mMMTab.mExcludeImportantFromExpiry, GlobalSettings::self()->excludeImportantMailFromExpiryItem());
    saveCheckBox(mMMTab.mShowPopupAfterDnD, GlobalSettings::self()->showPopupAfterDnDItem());
    saveCheckBox(mMMTab.mStartUpFolderCheck, GlobalSettings::self()->startSpecificFolderAtStartupItem());
}

MiscPageAgentSettingsTab::MiscPageAgentSettingsTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0 , 0, 0, 0);
    mConfigureAgent = new ConfigureAgentsWidget;
    l->addWidget(mConfigureAgent);

    connect(mConfigureAgent, &ConfigureAgentsWidget::changed, this, &MiscPageAgentSettingsTab::slotEmitChanged);
}

void MiscPageAgentSettingsTab::doLoadFromGlobalSettings()
{
    mConfigureAgent->doLoadFromGlobalSettings();
}

void MiscPageAgentSettingsTab::save()
{
    mConfigureAgent->save();
}

void MiscPageAgentSettingsTab::doResetToDefaultsOther()
{
    mConfigureAgent->doResetToDefaultsOther();
}

QString MiscPageAgentSettingsTab::helpAnchor() const
{
    return mConfigureAgent->helpAnchor();
}

MiscPageInviteTab::MiscPageInviteTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mInvitationUi = new MessageViewer::InvitationSettings(this);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0 , 0, 0, 0);
    l->addWidget(mInvitationUi);
    connect(mInvitationUi, &MessageViewer::InvitationSettings::changed, this, &MiscPageInviteTab::slotEmitChanged);
}

void MiscPage::InviteTab::doLoadFromGlobalSettings()
{
    mInvitationUi->doLoadFromGlobalSettings();
}

void MiscPage::InviteTab::save()
{
    mInvitationUi->save();
}

void MiscPage::InviteTab::doResetToDefaultsOther()
{
    mInvitationUi->doResetToDefaultsOther();
}

MiscPageProxyTab::MiscPageProxyTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    KCModuleInfo proxyInfo(QLatin1String("proxy.desktop"));
    mProxyModule = new KCModuleProxy(proxyInfo, parent);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->addWidget(mProxyModule);
    connect(mProxyModule, SIGNAL(changed(bool)), this, SLOT(slotEmitChanged()));
}

void MiscPage::ProxyTab::save()
{
    mProxyModule->save();
}

MiscPagePrintingTab::MiscPagePrintingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mPrintingUi = new MessageViewer::PrintingSettings(this);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0 , 0, 0, 0);
    l->addWidget(mPrintingUi);
    connect(mPrintingUi, &MessageViewer::PrintingSettings::changed, this, &MiscPagePrintingTab::slotEmitChanged);
}

void MiscPagePrintingTab::doLoadFromGlobalSettings()
{
    mPrintingUi->doLoadFromGlobalSettings();
}

void MiscPagePrintingTab::doResetToDefaultsOther()
{
    mPrintingUi->doResetToDefaultsOther();
}

void MiscPagePrintingTab::save()
{
    mPrintingUi->save();
}

//----------------------------
