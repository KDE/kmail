/*
  Copyright (c) 2013-2017 Montel Laurent <montel@kde.org>

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
#include "PimCommon/ConfigureImmutableWidgetUtils"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "settings/kmailsettings.h"

#include <MailCommon/FolderRequester>
#include "MessageViewer/InvitationSettings"
#include "MessageViewer/PrintingSettings"
#include "messageviewer/messageviewersettings.h"

#include "MessageViewer/PrintingSettings"

#include <KLocalizedString>
#include <QHBoxLayout>

#include <WebEngineViewer/NetworkPluginUrlInterceptorConfigureWidget>
#include <WebEngineViewer/NetworkUrlInterceptorPluginManager>
#include <WebEngineViewer/NetworkPluginUrlInterceptor>

using namespace MailCommon;
QString MiscPage::helpAnchor() const
{
    return QStringLiteral("configure-misc");
}

MiscPage::MiscPage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    FolderTab *folderTab = new FolderTab();
    addTab(folderTab, i18n("Folders"));

    InviteTab *inviteTab = new InviteTab();
    addTab(inviteTab, i18n("Invitations"));

    MiscPagePrintingTab *printingTab = new MiscPagePrintingTab();
    addTab(printingTab, i18n("Printing"));
}

QString MiscPageFolderTab::helpAnchor() const
{
    return QStringLiteral("configure-misc-folders");
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

    mMMTab.mExcludeImportantFromExpiry->setWhatsThis(
        i18n(KMailSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8()));

    connect(mMMTab.mExcludeImportantFromExpiry, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mLoopOnGotoUnread, QOverload<int>::of(&KComboBox::activated), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mActionEnterFolder, QOverload<int>::of(&KComboBox::activated), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, mMMTab.mDelayedMarkTime, &QWidget::setEnabled);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
    connect(mMMTab.mShowPopupAfterDnD, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mOnStartupOpenFolder, &MailCommon::FolderRequester::folderChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mEmptyTrashCheck, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, mOnStartupOpenFolder, &MailCommon::FolderRequester::setEnabled);
}

void MiscPage::FolderTab::doLoadFromGlobalSettings()
{
    loadWidget(mMMTab.mExcludeImportantFromExpiry, KMailSettings::self()->excludeImportantMailFromExpiryItem());
    // default = "Loop in current folder"
    loadWidget(mMMTab.mLoopOnGotoUnread, KMailSettings::self()->loopOnGotoUnreadItem());
    loadWidget(mMMTab.mActionEnterFolder, KMailSettings::self()->actionEnterFolderItem());
    loadWidget(mMMTab.mDelayedMarkAsRead, MessageViewer::MessageViewerSettings::self()->delayedMarkAsReadItem());
    loadWidget(mMMTab.mDelayedMarkTime, MessageViewer::MessageViewerSettings::self()->delayedMarkTimeItem());
    loadWidget(mMMTab.mShowPopupAfterDnD, KMailSettings::self()->showPopupAfterDnDItem());
    loadWidget(mMMTab.mStartUpFolderCheck, KMailSettings::self()->startSpecificFolderAtStartupItem());
    mOnStartupOpenFolder->setEnabled(KMailSettings::self()->startSpecificFolderAtStartup());
    doLoadOther();
}

void MiscPage::FolderTab::doLoadOther()
{
    loadWidget(mMMTab.mEmptyTrashCheck, KMailSettings::self()->emptyTrashOnExitItem());
    mOnStartupOpenFolder->setCollection(Akonadi::Collection(KMailSettings::self()->startupFolder()));
}

void MiscPage::FolderTab::save()
{
    saveCheckBox(mMMTab.mEmptyTrashCheck, KMailSettings::self()->emptyTrashOnExitItem());
    saveComboBox(mMMTab.mActionEnterFolder, KMailSettings::self()->actionEnterFolderItem());
    KMailSettings::self()->setStartupFolder(mOnStartupOpenFolder->collection().id());

    saveCheckBox(mMMTab.mDelayedMarkAsRead, MessageViewer::MessageViewerSettings::self()->delayedMarkAsReadItem());
    saveSpinBox(mMMTab.mDelayedMarkTime, MessageViewer::MessageViewerSettings::self()->delayedMarkTimeItem());
    saveComboBox(mMMTab.mLoopOnGotoUnread, KMailSettings::self()->loopOnGotoUnreadItem());

    saveCheckBox(mMMTab.mExcludeImportantFromExpiry, KMailSettings::self()->excludeImportantMailFromExpiryItem());
    saveCheckBox(mMMTab.mShowPopupAfterDnD, KMailSettings::self()->showPopupAfterDnDItem());
    saveCheckBox(mMMTab.mStartUpFolderCheck, KMailSettings::self()->startSpecificFolderAtStartupItem());
}

MiscPageInviteTab::MiscPageInviteTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mInvitationUi = new MessageViewer::InvitationSettings(this);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
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

MiscPagePrintingTab::MiscPagePrintingTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mPrintingUi = new MessageViewer::PrintingSettings(this);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
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
