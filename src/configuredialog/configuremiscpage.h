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

class MiscPageFolderTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit MiscPageFolderTab(QWidget *parent = nullptr);

    void save() override;
    QString helpAnchor() const;

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
    explicit MiscPage(QWidget *parent = nullptr);
    QString helpAnchor() const override;

    typedef MiscPageFolderTab FolderTab;
    typedef MiscPageInviteTab InviteTab;
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

#endif // CONFIGUREMISCPAGE_H
