/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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
#include "configureagentswidget.h"
#include "settings/globalsettings.h"

#include <mailcommon/folder/folderrequester.h>
#include "messageviewer/widgets/invitationsettings.h"
#include "messageviewer/widgets/printingsettings.h"
#include "messageviewer/settings/globalsettings.h"

#include <KCModuleProxy>
#include <KCModuleInfo>
#include <KLocale>
using namespace MailCommon;
QString MiscPage::helpAnchor() const
{
    return QString::fromLatin1("configure-misc");
}

MiscPage::MiscPage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    mFolderTab = new FolderTab();
    addTab( mFolderTab, i18n("Folders") );

    mInviteTab = new InviteTab();
    addTab( mInviteTab, i18n("Invitations" ) );

    mProxyTab = new ProxyTab();
    addTab( mProxyTab, i18n("Proxy" ) );

    mAgentSettingsTab = new MiscPageAgentSettingsTab();
    addTab( mAgentSettingsTab, i18n("Agent Settings" ) );

    mPrintingTab = new MiscPagePrintingTab();
    addTab( mPrintingTab, i18n("Printing" ) );
}

QString MiscPageFolderTab::helpAnchor() const
{
    return QString::fromLatin1("Sconfigure-misc-folders");
}

MiscPageFolderTab::MiscPageFolderTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mMMTab.setupUi( this );
    //replace QWidget with FolderRequester. Promote to doesn't work due to the custom constructor
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    mMMTab.mOnStartupOpenFolder->setLayout( layout );
    mOnStartupOpenFolder = new FolderRequester( mMMTab.mOnStartupOpenFolder );
    layout->addWidget( mOnStartupOpenFolder );

    mMMTab.gridLayout->setSpacing( KDialog::spacingHint() );
    mMMTab.gridLayout->setMargin( KDialog::marginHint() );
    mMMTab.mExcludeImportantFromExpiry->setWhatsThis(
                i18n( GlobalSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8() ) );

    connect( mMMTab.mEmptyFolderConfirmCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mExcludeImportantFromExpiry, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mLoopOnGotoUnread, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mActionEnterFolder, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mDelayedMarkTime, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)),
             mMMTab.mDelayedMarkTime, SLOT(setEnabled(bool)));
    connect( mMMTab.mDelayedMarkAsRead, SIGNAL(toggled(bool)),
             this , SLOT(slotEmitChanged()) );
    connect( mMMTab.mShowPopupAfterDnD, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mOnStartupOpenFolder, SIGNAL(folderChanged(Akonadi::Collection)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mEmptyTrashCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mStartUpFolderCheck, SIGNAL(toggled(bool)),
             this, SLOT(slotEmitChanged()) );
    connect( mMMTab.mStartUpFolderCheck, SIGNAL(toggled(bool)),
             mOnStartupOpenFolder, SLOT(setEnabled(bool)) );
}

void MiscPage::FolderTab::doLoadFromGlobalSettings()
{
    mMMTab.mExcludeImportantFromExpiry->setChecked( GlobalSettings::self()->excludeImportantMailFromExpiry() );
    // default = "Loop in current folder"
    mMMTab.mLoopOnGotoUnread->setCurrentIndex( GlobalSettings::self()->loopOnGotoUnread() );
    mMMTab.mActionEnterFolder->setCurrentIndex( GlobalSettings::self()->actionEnterFolder() );
    mMMTab.mDelayedMarkAsRead->setChecked( MessageViewer::GlobalSettings::self()->delayedMarkAsRead() );
    mMMTab.mDelayedMarkTime->setValue( MessageViewer::GlobalSettings::self()->delayedMarkTime() );
    mMMTab.mShowPopupAfterDnD->setChecked( GlobalSettings::self()->showPopupAfterDnD() );
    mMMTab.mStartUpFolderCheck->setChecked( GlobalSettings::self()->startSpecificFolderAtStartup() );
    mOnStartupOpenFolder->setEnabled(GlobalSettings::self()->startSpecificFolderAtStartup());
    doLoadOther();
}

void MiscPage::FolderTab::doLoadOther()
{
    mMMTab.mEmptyTrashCheck->setChecked( GlobalSettings::self()->emptyTrashOnExit() );
    mOnStartupOpenFolder->setCollection( Akonadi::Collection( GlobalSettings::self()->startupFolder() ) );
    mMMTab.mEmptyFolderConfirmCheck->setChecked( GlobalSettings::self()->confirmBeforeEmpty() );
}

void MiscPage::FolderTab::save()
{
    GlobalSettings::self()->setEmptyTrashOnExit( mMMTab.mEmptyTrashCheck->isChecked() );
    GlobalSettings::self()->setConfirmBeforeEmpty( mMMTab.mEmptyFolderConfirmCheck->isChecked() );
    GlobalSettings::self()->setStartupFolder( mOnStartupOpenFolder->collection().id() );

    MessageViewer::GlobalSettings::self()->setDelayedMarkAsRead( mMMTab.mDelayedMarkAsRead->isChecked() );
    MessageViewer::GlobalSettings::self()->setDelayedMarkTime( mMMTab.mDelayedMarkTime->value() );
    GlobalSettings::self()->setActionEnterFolder( mMMTab.mActionEnterFolder->currentIndex() );
    GlobalSettings::self()->setLoopOnGotoUnread( mMMTab.mLoopOnGotoUnread->currentIndex() );
    GlobalSettings::self()->setShowPopupAfterDnD( mMMTab.mShowPopupAfterDnD->isChecked() );
    GlobalSettings::self()->setExcludeImportantMailFromExpiry(
                mMMTab.mExcludeImportantFromExpiry->isChecked() );
    GlobalSettings::self()->setStartSpecificFolderAtStartup(mMMTab.mStartUpFolderCheck->isChecked() );
}

MiscPageAgentSettingsTab::MiscPageAgentSettingsTab( QWidget* parent )
    : ConfigModuleTab( parent )
{
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setContentsMargins( 0 , 0, 0, 0 );
    mConfigureAgent = new ConfigureAgentsWidget;
    l->addWidget( mConfigureAgent );

    connect( mConfigureAgent, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
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

MiscPageInviteTab::MiscPageInviteTab( QWidget* parent )
    : ConfigModuleTab( parent )
{
    mInvitationUi = new MessageViewer::InvitationSettings( this );
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setContentsMargins( 0 , 0, 0, 0 );
    l->addWidget( mInvitationUi );
    connect( mInvitationUi, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
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


MiscPageProxyTab::MiscPageProxyTab( QWidget* parent )
    : ConfigModuleTab( parent )
{
    KCModuleInfo proxyInfo(QLatin1String("proxy.desktop"));
    mProxyModule = new KCModuleProxy(proxyInfo, parent);
    QHBoxLayout *l = new QHBoxLayout( this );
    l->addWidget( mProxyModule );
    connect(mProxyModule,SIGNAL(changed(bool)), this, SLOT(slotEmitChanged()));
}

void MiscPage::ProxyTab::save()
{
    mProxyModule->save();
}

MiscPagePrintingTab::MiscPagePrintingTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mPrintingUi = new MessageViewer::PrintingSettings( this );
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setContentsMargins( 0 , 0, 0, 0 );
    l->addWidget( mPrintingUi );
    connect( mPrintingUi, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );
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
