/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "configuredialog_p.h"
#include "kmail_export.h"
#include "ui_miscpagemaintab.h"

namespace MailCommon
{
class FolderRequester;
}
namespace MessageViewer
{
class InvitationSettings;
class PrintingSettings;
}

class MiscPageFolderTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPageFolderTab(QWidget *parent = nullptr);

    void save() override;
    Q_REQUIRED_RESULT QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;

private:
    Ui_MiscMainTab mMMTab;
    MailCommon::FolderRequester *mOnStartupOpenFolder = nullptr;
};

class MiscPageInviteTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPageInviteTab(QWidget *parent = nullptr);
    void save() override;
    void doResetToDefaultsOther() override;

private:
    void doLoadFromGlobalSettings() override;

private:
    MessageViewer::InvitationSettings *mInvitationUi = nullptr;
};

class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit MiscPage(QWidget *parent = nullptr, const QVariantList &args = {});
    QString helpAnchor() const override;

    using FolderTab = MiscPageFolderTab;
    using InviteTab = MiscPageInviteTab;
};

class MiscPagePrintingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPagePrintingTab(QWidget *parent = nullptr);
    void save() override;
    void doResetToDefaultsOther() override;

private:
    void doLoadFromGlobalSettings() override;

private:
    MessageViewer::PrintingSettings *mPrintingUi = nullptr;
};

#ifdef WITH_KUSERFEEDBACK
namespace KUserFeedback
{
class FeedbackConfigWidget;
}
class KuserFeedBackPageTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit KuserFeedBackPageTab(QWidget *parent = nullptr);
    void save() override;
    void doResetToDefaultsOther() override;

private:
    void doLoadFromGlobalSettings() override;
    KUserFeedback::FeedbackConfigWidget *mUserFeedbackWidget = nullptr;
};
#endif

