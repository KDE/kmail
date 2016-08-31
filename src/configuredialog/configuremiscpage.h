/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

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

#ifndef CONFIGUREMISCPAGE_H
#define CONFIGUREMISCPAGE_H

#include "kmail_export.h"
#include "ui_miscpagemaintab.h"
#include "configuredialog_p.h"
#include "config-kmail.h"

namespace MailCommon
{
class FolderRequester;
}
namespace MessageViewer
{
class InvitationSettings;
class PrintingSettings;
}
class ConfigureAgentsWidget;

namespace WebEngineViewer
{
class NetworkPluginUrlInterceptorConfigureWidget;
}

class MiscPageFolderTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPageFolderTab(QWidget *parent = Q_NULLPTR);

    void save() Q_DECL_OVERRIDE;
    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;
    void doLoadOther() Q_DECL_OVERRIDE;

private:
    Ui_MiscMainTab mMMTab;
    MailCommon::FolderRequester *mOnStartupOpenFolder;
};

class MiscPageInviteTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPageInviteTab(QWidget *parent = Q_NULLPTR);
    void save() Q_DECL_OVERRIDE;
    void doResetToDefaultsOther() Q_DECL_OVERRIDE;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;

private:
    MessageViewer::InvitationSettings *mInvitationUi;
};

class MiscPageAgentSettingsTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPageAgentSettingsTab(QWidget *parent = Q_NULLPTR);
    void save() Q_DECL_OVERRIDE;
    void doResetToDefaultsOther() Q_DECL_OVERRIDE;

    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;

private:
    ConfigureAgentsWidget *mConfigureAgent;
};

class AddonsPluginTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AddonsPluginTab(WebEngineViewer::NetworkPluginUrlInterceptorConfigureWidget *configureWidget, QWidget *parent = Q_NULLPTR);
    ~AddonsPluginTab();

    void save() Q_DECL_OVERRIDE;
    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;
    void doLoadOther() Q_DECL_OVERRIDE;
    void doResetToDefaultsOther() Q_DECL_OVERRIDE;

private:
    WebEngineViewer::NetworkPluginUrlInterceptorConfigureWidget *mConfigureWidget;
};

class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit MiscPage(QWidget *parent = Q_NULLPTR);
    QString helpAnchor() const Q_DECL_OVERRIDE;

    typedef MiscPageFolderTab FolderTab;
    typedef MiscPageInviteTab InviteTab;
};

#ifdef WEBENGINEVIEWER_PRINT_SUPPORT
class MiscPagePrintingTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPagePrintingTab(QWidget *parent = Q_NULLPTR);
    void save() Q_DECL_OVERRIDE;
    void doResetToDefaultsOther() Q_DECL_OVERRIDE;

private:
    void doLoadFromGlobalSettings() Q_DECL_OVERRIDE;

private:
    MessageViewer::PrintingSettings *mPrintingUi;
};
#endif

#endif // CONFIGUREMISCPAGE_H
