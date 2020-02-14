/*
   Copyright (C) 2014-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kmlaunchexternalcomponent.h"
#include <KMessageBox>
#include <KLocalizedString>
#include <KRun>
#include <AkonadiCore/AgentManager>

#include "util.h"
#include "archivemailagentinterface.h"
#include "sendlateragentinterface.h"
#include "followupreminderinterface.h"
#include "MailCommon/FilterManager"

#include <QProcess>
#include "kmail_debug.h"
#include <QStandardPaths>

KMLaunchExternalComponent::KMLaunchExternalComponent(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , mParentWidget(parentWidget)
{
}

KMLaunchExternalComponent::~KMLaunchExternalComponent()
{
}

void KMLaunchExternalComponent::slotConfigureAutomaticArchiving()
{
    auto agent = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_archivemail_agent"));
    if (agent.isValid()) {
        agent.configure(mParentWidget);
    } else {
        KMessageBox::error(mParentWidget, i18n("Archive Mail Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotConfigureSendLater()
{
    auto agent = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_sendlater_agent"));
    if (agent.isValid()) {
        agent.configure(mParentWidget);
    } else {
        KMessageBox::error(mParentWidget, i18n("Send Later Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotConfigureFollowupReminder()
{
    auto agent = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_followupreminder_agent"));
    if (agent.isValid()) {
        agent.configure(mParentWidget);
    } else {
        KMessageBox::error(mParentWidget, i18n("Followup Reminder Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotStartCertManager()
{
    if (!QProcess::startDetached(QStringLiteral("kleopatra"), QStringList())) {
        KMessageBox::error(mParentWidget, i18n("Could not start certificate manager; "
                                               "please make sure you have Kleopatra properly installed."),
                           i18n("KMail Error"));
    }
}

void KMLaunchExternalComponent::slotImportWizard()
{
    const QString path = QStandardPaths::findExecutable(QStringLiteral("akonadiimportwizard"));
    if (!QProcess::startDetached(path, QStringList())) {
        KMessageBox::error(mParentWidget, i18n("Could not start the import wizard. "
                                               "Please make sure you have ImportWizard properly installed."),
                           i18n("Unable to start import wizard"));
    }
}

void KMLaunchExternalComponent::slotExportData()
{
    const QString path = QStandardPaths::findExecutable(QStringLiteral("pimdataexporter"));
    if (!QProcess::startDetached(path, QStringList())) {
        KMessageBox::error(mParentWidget, i18n("Could not start \"PIM Data Exporter\" program. "
                                               "Please check your installation."),
                           i18n("Unable to start \"PIM Data Exporter\" program"));
    }
}

void KMLaunchExternalComponent::slotRunAddressBook()
{
    KRun::runCommand(QStringLiteral("kaddressbook"), mParentWidget->window());
}

void KMLaunchExternalComponent::slotImport()
{
    const QStringList lst = {QStringLiteral("--mode"), QStringLiteral("manual")};
    const QString path = QStandardPaths::findExecutable(QStringLiteral("akonadiimportwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget, i18n("Could not start the ImportWizard. "
                                               "Please make sure you have ImportWizard properly installed."),
                           i18n("Unable to start ImportWizard"));
    }
}

void KMLaunchExternalComponent::slotAccountWizard()
{
    const QStringList lst = {QStringLiteral("--type"), QStringLiteral("message/rfc822") };

    const QString path = QStandardPaths::findExecutable(QStringLiteral("accountwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget, i18n("Could not start the account wizard. "
                                               "Please make sure you have AccountWizard properly installed."),
                           i18n("Unable to start account wizard"));
    }
}

void KMLaunchExternalComponent::slotFilterLogViewer()
{
    MailCommon::FilterManager::instance()->showFilterLogDialog(static_cast<qlonglong>(mParentWidget->winId()));
}
