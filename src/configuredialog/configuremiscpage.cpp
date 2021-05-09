/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

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

MiscPage::MiscPage(QWidget *parent, const QVariantList &args)
    : ConfigModuleWithTabs(parent, args)
{
    auto folderTab = new FolderTab();
    addTab(folderTab, i18n("Folders"));

    auto inviteTab = new InviteTab();
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

    mMMTab.mExcludeImportantFromExpiry->setWhatsThis(i18n(KMailSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8().constData()));

    connect(mMMTab.mExcludeImportantFromExpiry, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mLoopOnGotoUnread, qOverload<int>(&QComboBox::activated), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mActionEnterFolder, qOverload<int>(&QComboBox::activated), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkTime, qOverload<int>(&QSpinBox::valueChanged), this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, mMMTab.mDelayedMarkTime, &QWidget::setEnabled);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
    connect(mMMTab.mShowPopupAfterDnD, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mOnStartupOpenFolder, &MailCommon::FolderRequester::folderChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mEmptyTrashCheck, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, mOnStartupOpenFolder, &MailCommon::FolderRequester::setEnabled);
    connect(mMMTab.mDeleteMessagesWithoutConfirmation, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
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

    loadWidget(mMMTab.mDeleteMessagesWithoutConfirmation, KMailSettings::self()->deleteMessageWithoutConfirmationItem());

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
    saveCheckBox(mMMTab.mDeleteMessagesWithoutConfirmation, KMailSettings::self()->deleteMessageWithoutConfirmationItem());
}

MiscPageInviteTab::MiscPageInviteTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mInvitationUi = new MessageViewer::InvitationSettings(this);
    auto l = new QHBoxLayout(this);
    l->setContentsMargins({});
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
{
    mUserFeedbackWidget = new KUserFeedback::FeedbackConfigWidget(this);
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
