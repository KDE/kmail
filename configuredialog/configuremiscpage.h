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

#ifndef CONFIGUREMISCPAGE_H
#define CONFIGUREMISCPAGE_H

#include "kmail_export.h"
#include "ui_miscpagemaintab.h"
#include "configuredialog_p.h"

namespace MailCommon {
class FolderRequester;
}
namespace MessageViewer {
class InvitationSettings;
class PrintingSettings;
}
class KCModuleProxy;
class ConfigureAgentsWidget;

class MiscPageFolderTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit MiscPageFolderTab( QWidget * parent=0 );

    void save();
    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui_MiscMainTab mMMTab;
    MailCommon::FolderRequester *mOnStartupOpenFolder;
};

class MiscPageInviteTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPageInviteTab( QWidget * parent=0 );
    void save();
    void doResetToDefaultsOther();

private:
    void doLoadFromGlobalSettings();

private:
    MessageViewer::InvitationSettings *mInvitationUi;
};


class MiscPageProxyTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPageProxyTab( QWidget * parent=0 );
    void save();
private:
    KCModuleProxy *mProxyModule;
};


class MiscPageAgentSettingsTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPageAgentSettingsTab( QWidget * parent=0 );
    void save();
    void doResetToDefaultsOther();

    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings();

private:
    ConfigureAgentsWidget *mConfigureAgent;
};

class MiscPagePrintingTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPagePrintingTab( QWidget * parent=0 );
    void save();
    void doResetToDefaultsOther();

private:
    void doLoadFromGlobalSettings();

private:
    MessageViewer::PrintingSettings* mPrintingUi;
};


class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit MiscPage( QWidget *parent=0 );
    QString helpAnchor() const;

    typedef MiscPageFolderTab FolderTab;
    typedef MiscPageInviteTab InviteTab;
    typedef MiscPageProxyTab ProxyTab;

private:
    FolderTab * mFolderTab;
    InviteTab * mInviteTab;
    ProxyTab * mProxyTab;
    MiscPageAgentSettingsTab *mAgentSettingsTab;
    MiscPagePrintingTab *mPrintingTab;
};

#endif // CONFIGUREMISCPAGE_H
