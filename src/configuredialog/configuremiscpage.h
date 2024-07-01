/*
  SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

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
    [[nodiscard]] QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;
    void doResetToDefaultsOther() override;

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
    MessageViewer::InvitationSettings *const mInvitationUi;
};

class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit MiscPage(QObject *parent, const KPluginMetaData &data);
    [[nodiscard]] QString helpAnchor() const override;
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
    MessageViewer::PrintingSettings *const mPrintingUi;
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
    KUserFeedback::FeedbackConfigWidget *const mUserFeedbackWidget;
};
#endif
