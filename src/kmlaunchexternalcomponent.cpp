/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmlaunchexternalcomponent.h"
#include <AkonadiCore/AgentManager>
#include <KLocalizedString>
#include <KMessageBox>

#include "archivemailagentinterface.h"
#include "followupreminderinterface.h"
#include "mailmergeagentinterface.h"
#include "sendlateragentinterface.h"
#include "util.h"
#include <MailCommon/FilterManager>

#include <KDialogJobUiDelegate>
#include <KIO/CommandLauncherJob>

#include "kmail_debug.h"
#include <QProcess>
#include <QStandardPaths>

KMLaunchExternalComponent::KMLaunchExternalComponent(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , mParentWidget(parentWidget)
{
}

KMLaunchExternalComponent::~KMLaunchExternalComponent() = default;

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

void KMLaunchExternalComponent::slotConfigureMailMerge()
{
    auto agent = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_mailmerge_agent"));
    if (agent.isValid()) {
        agent.configure(mParentWidget);
    } else {
        KMessageBox::error(mParentWidget, i18n("Mail Merge Agent was not registered."));
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
        KMessageBox::error(mParentWidget,
                           i18n("Could not start certificate manager; "
                                "please make sure you have Kleopatra properly installed."),
                           i18n("KMail Error"));
    }
}

void KMLaunchExternalComponent::slotImportWizard()
{
    const QString path = QStandardPaths::findExecutable(QStringLiteral("akonadiimportwizard"));
    if (!QProcess::startDetached(path, QStringList())) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start the import wizard. "
                                "Please make sure you have ImportWizard properly installed."),
                           i18n("Unable to start import wizard"));
    }
}

void KMLaunchExternalComponent::slotExportData()
{
    const QString path = QStandardPaths::findExecutable(QStringLiteral("pimdataexporter"));
    if (!QProcess::startDetached(path, QStringList())) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start \"PIM Data Exporter\" program. "
                                "Please check your installation."),
                           i18n("Unable to start \"PIM Data Exporter\" program"));
    }
}

void KMLaunchExternalComponent::slotRunAddressBook()
{
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kaddressbook"), {}, this);
    job->setDesktopName(QStringLiteral("org.kde.kaddressbook"));
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mParentWidget->window()));
    job->start();
}

void KMLaunchExternalComponent::slotImport()
{
    const QStringList lst = {QStringLiteral("--mode"), QStringLiteral("manual")};
    const QString path = QStandardPaths::findExecutable(QStringLiteral("akonadiimportwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start the ImportWizard. "
                                "Please make sure you have ImportWizard properly installed."),
                           i18n("Unable to start ImportWizard"));
    }
}

void KMLaunchExternalComponent::slotAccountWizard()
{
    const QStringList lst = {QStringLiteral("--type"), QStringLiteral("message/rfc822")};

    const QString path = QStandardPaths::findExecutable(QStringLiteral("accountwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start the account wizard. "
                                "Please make sure you have AccountWizard properly installed."),
                           i18n("Unable to start account wizard"));
    }
}

void KMLaunchExternalComponent::slotFilterLogViewer()
{
    MailCommon::FilterManager::instance()->showFilterLogDialog(static_cast<qlonglong>(mParentWidget->winId()));
}
