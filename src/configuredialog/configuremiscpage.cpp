/*
  SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configuremiscpage.h"
#include <PimCommon/ConfigureImmutableWidgetUtils>
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "kmkernel.h"
#include "settings/kmailsettings.h"
#include <MailCommon/FolderRequester>
#include <MessageViewer/InvitationSettings>
#include <MessageViewer/MessageViewerSettings>
#include <MessageViewer/PrintingSettings>

#include <KLocalizedString>
#include <QHBoxLayout>

#ifdef WITH_KUSERFEEDBACK
#include <KUserFeedback/FeedbackConfigWidget>
#endif

using namespace MailCommon;
QString MiscPage::helpAnchor() const
{
    return QStringLiteral("configure-misc");
}

MiscPage::MiscPage(QObject *parent, const KPluginMetaData &data)
    : ConfigModuleWithTabs(parent, data)
{
    auto folderTab = new MiscPageFolderTab();
    addTab(folderTab, i18n("Folders"));

    auto inviteTab = new MiscPageInviteTab();
    addTab(inviteTab, i18n("Invitations"));

    auto printingTab = new MiscPagePrintingTab();
    addTab(printingTab, i18n("Printing"));
#ifdef WITH_KUSERFEEDBACK
    auto userFeedBackTab = new KuserFeedBackPageTab();
    addTab(userFeedBackTab, i18n("User Feedback"));
#endif
}

QString MiscPageFolderTab::helpAnchor() const
{
    return QStringLiteral("configure-misc-folders");
}

MiscPageFolderTab::MiscPageFolderTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mMMTab.setupUi(this);
    // replace QWidget with FolderRequester. Promote to doesn't work due to the custom constructor
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    mMMTab.mOnStartupOpenFolder->setLayout(layout);
    mOnStartupOpenFolder = new FolderRequester(mMMTab.mOnStartupOpenFolder);
    layout->addWidget(mOnStartupOpenFolder);

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(mMMTab.kcfg_ExcludeImportantFromExpiry, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#else
    connect(mMMTab.kcfg_ExcludeImportantMailFromExpiry, &QCheckBox::checkStateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#endif
    connect(mMMTab.kcfg_LoopOnGotoUnread, &QComboBox::activated, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.kcfg_ActionEnterFolder, &QComboBox::activated, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkTime, &QSpinBox::valueChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, mMMTab.mDelayedMarkTime, &QWidget::setEnabled);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(mMMTab.kcfg_ShowPopupAfterDnD, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#else
    connect(mMMTab.kcfg_ShowPopupAfterDnD, &QCheckBox::checkStateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#endif
    connect(mOnStartupOpenFolder, &MailCommon::FolderRequester::folderChanged, this, &MiscPageFolderTab::slotEmitChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(mMMTab.kcfg_EmptyTrashOnExit, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#else
    connect(mMMTab.kcfg_EmptyTrashOnExit, &QCheckBox::checkStateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#endif
    connect(mMMTab.kcfg_StartSpecificFolderAtStartup, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.kcfg_StartSpecificFolderAtStartup, &QCheckBox::toggled, mOnStartupOpenFolder, &MailCommon::FolderRequester::setEnabled);
    connect(mMMTab.kcfg_DeleteMessageWithoutConfirmation, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.kcfg_EnableFolderDnD, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
}

void MiscPageFolderTab::doLoadFromGlobalSettings()
{
    loadWidget(mMMTab.mDelayedMarkAsRead, MessageViewer::MessageViewerSettings::self()->delayedMarkAsReadItem());
    loadWidget(mMMTab.mDelayedMarkTime, MessageViewer::MessageViewerSettings::self()->delayedMarkTimeItem());
    mOnStartupOpenFolder->setEnabled(KMailSettings::self()->startSpecificFolderAtStartup());

    doLoadOther();
}

void MiscPageFolderTab::doLoadOther()
{
    mOnStartupOpenFolder->setCollection(Akonadi::Collection(KMailSettings::self()->startupFolder()));
}

void MiscPageFolderTab::save()
{
    KMailSettings::self()->setStartupFolder(mOnStartupOpenFolder->collection().id());

    saveCheckBox(mMMTab.mDelayedMarkAsRead, MessageViewer::MessageViewerSettings::self()->delayedMarkAsReadItem());
    saveSpinBox(mMMTab.mDelayedMarkTime, MessageViewer::MessageViewerSettings::self()->delayedMarkTimeItem());
}

MiscPageInviteTab::MiscPageInviteTab(QWidget *parent)
    : ConfigModuleTab(parent)
    , mInvitationUi(new MessageViewer::InvitationSettings(this))
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins({});
    l->addWidget(mInvitationUi);
    connect(mInvitationUi, &MessageViewer::InvitationSettings::changed, this, &MiscPageInviteTab::slotEmitChanged);
}

void MiscPageInviteTab::doLoadFromGlobalSettings()
{
    mInvitationUi->doLoadFromGlobalSettings();
}

void MiscPageInviteTab::save()
{
    mInvitationUi->save();
}

void MiscPageInviteTab::doResetToDefaultsOther()
{
    mInvitationUi->doResetToDefaultsOther();
}

MiscPagePrintingTab::MiscPagePrintingTab(QWidget *parent)
    : ConfigModuleTab(parent)
    , mPrintingUi(new MessageViewer::PrintingSettings(this))
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins({});
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

#ifdef WITH_KUSERFEEDBACK
KuserFeedBackPageTab::KuserFeedBackPageTab(QWidget *parent)
    : ConfigModuleTab(parent)
    , mUserFeedbackWidget(new KUserFeedback::FeedbackConfigWidget(this))
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins({});
    l->addWidget(mUserFeedbackWidget);
    connect(mUserFeedbackWidget, &KUserFeedback::FeedbackConfigWidget::configurationChanged, this, &KuserFeedBackPageTab::slotEmitChanged);

    if (KMKernel::self()) {
        mUserFeedbackWidget->setFeedbackProvider(KMKernel::self()->userFeedbackProvider());
    }
}

void KuserFeedBackPageTab::save()
{
    if (KMKernel::self()) {
        // set current active mode + write back the config for future starts
        KMKernel::self()->userFeedbackProvider()->setTelemetryMode(mUserFeedbackWidget->telemetryMode());
        KMKernel::self()->userFeedbackProvider()->setSurveyInterval(mUserFeedbackWidget->surveyInterval());
    }
}

void KuserFeedBackPageTab::doResetToDefaultsOther()
{
    if (KMKernel::self()) {
        mUserFeedbackWidget->setFeedbackProvider(KMKernel::self()->userFeedbackProvider());
    }
}

void KuserFeedBackPageTab::doLoadFromGlobalSettings()
{
    if (KMKernel::self()) {
        mUserFeedbackWidget->setFeedbackProvider(KMKernel::self()->userFeedbackProvider());
    }
}

#endif

#include "moc_configuremiscpage.cpp"
