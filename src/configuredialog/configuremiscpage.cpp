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

    mMMTab.mExcludeImportantFromExpiry->setWhatsThis(i18n(KMailSettings::self()->excludeImportantMailFromExpiryItem()->whatsThis().toUtf8().constData()));

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(mMMTab.mExcludeImportantFromExpiry, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#else
    connect(mMMTab.mExcludeImportantFromExpiry, &QCheckBox::checkStateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#endif
    connect(mMMTab.mLoopOnGotoUnread, &QComboBox::activated, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mActionEnterFolder, &QComboBox::activated, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkTime, &QSpinBox::valueChanged, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, mMMTab.mDelayedMarkTime, &QWidget::setEnabled);
    connect(mMMTab.mDelayedMarkAsRead, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(mMMTab.mShowPopupAfterDnD, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#else
    connect(mMMTab.mShowPopupAfterDnD, &QCheckBox::checkStateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#endif
    connect(mOnStartupOpenFolder, &MailCommon::FolderRequester::folderChanged, this, &MiscPageFolderTab::slotEmitChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(mMMTab.mEmptyTrashCheck, &QCheckBox::stateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#else
    connect(mMMTab.mEmptyTrashCheck, &QCheckBox::checkStateChanged, this, &MiscPageFolderTab::slotEmitChanged);
#endif
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mStartUpFolderCheck, &QCheckBox::toggled, mOnStartupOpenFolder, &MailCommon::FolderRequester::setEnabled);
    connect(mMMTab.mDeleteMessagesWithoutConfirmation, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
    connect(mMMTab.mEnableFolderDragAndDrop, &QCheckBox::toggled, this, &MiscPageFolderTab::slotEmitChanged);
}

void MiscPageFolderTab::doLoadFromGlobalSettings()
{
    loadWidget(mMMTab.mEnableFolderDragAndDrop, KMailSettings::self()->enableFolderDnDItem());
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

void MiscPageFolderTab::doLoadOther()
{
    loadWidget(mMMTab.mEmptyTrashCheck, KMailSettings::self()->emptyTrashOnExitItem());
    mOnStartupOpenFolder->setCollection(Akonadi::Collection(KMailSettings::self()->startupFolder()));
}

void MiscPageFolderTab::save()
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
    saveCheckBox(mMMTab.mEnableFolderDragAndDrop, KMailSettings::self()->enableFolderDnDItem());
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
