/*
   SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmlaunchexternalcomponent.h"
#include "kmail_debug.h"
#include "newmailnotifierinterface.h"
#include "util.h"
#include <Akonadi/AgentConfigurationDialog>
#include <Akonadi/AgentManager>
#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <MailCommon/FilterManager>
#include <PimCommon/PimUtil>
#include <QPointer>

#include <QProcess>
#include <QStandardPaths>

KMLaunchExternalComponent::KMLaunchExternalComponent(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , mParentWidget(parentWidget)
{
}

KMLaunchExternalComponent::~KMLaunchExternalComponent() = default;

void KMLaunchExternalComponent::createAgentConfigurationDialog(const QString &agentName, const QString &errorMessage)
{
    auto agent = Akonadi::AgentManager::self()->instance(agentName);
    if (agent.isValid()) {
        QPointer<Akonadi::AgentConfigurationDialog> dlg = new Akonadi::AgentConfigurationDialog(agent, mParentWidget);
        dlg->exec();
        delete dlg;
    } else {
        KMessageBox::error(mParentWidget, errorMessage);
    }
}

void KMLaunchExternalComponent::slotConfigureAutomaticArchiving()
{
    createAgentConfigurationDialog(QStringLiteral("akonadi_archivemail_agent"), i18n("Archive Mail Agent was not registered."));
}

void KMLaunchExternalComponent::slotConfigureSendLater()
{
    createAgentConfigurationDialog(QStringLiteral("akonadi_sendlater_agent"), i18n("Send Later Agent was not registered."));
}

void KMLaunchExternalComponent::slotConfigureMailMerge()
{
    createAgentConfigurationDialog(QStringLiteral("akonadi_mailmerge_agent"), i18n("Mail Merge Agent was not registered."));
}

void KMLaunchExternalComponent::slotConfigureFollowupReminder()
{
    createAgentConfigurationDialog(QStringLiteral("akonadi_followupreminder_agent"), i18n("Followup Reminder Agent was not registered."));
}

void KMLaunchExternalComponent::slotStartCertManager()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    const KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("org.kde.kleopatra"));
    if (service) {
        auto job = new KIO::ApplicationLauncherJob(service);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mParentWidget->window()));
        job->start();
    } else {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start certificate manager; "
                                "please make sure you have Kleopatra properly installed."),
                           i18nc("@title:window", "KMail Error"));
    }
#else
    const QString path = PimCommon::Util::findExecutable(QStringLiteral("kleopatra"));
    if (path.isEmpty() || !QProcess::startDetached(path)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start certificate manager; "
                                "please make sure you have Kleopatra properly installed."),
                           i18nc("@title:window", "KMail Error"));
    }
#endif
}

void KMLaunchExternalComponent::slotImportWizard()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    const KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("org.kde.akonadiimportwizard"));
    if (service) {
        auto job = new KIO::ApplicationLauncherJob(service);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mParentWidget->window()));
        job->start();
    } else {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start \"ImportWizard\" program. "
                                "Please check your installation."),
                           i18nc("@title:window", "Unable to start \"ImportWizard\" program"));
    }
#else
    const QString path = PimCommon::Util::findExecutable(QStringLiteral("akonadiimportwizard"));
    if (path.isEmpty() || !QProcess::startDetached(path)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start \"ImportWizard\" program. "
                                "Please check your installation."),
                           i18nc("@title:window", "Unable to start \"ImportWizard\" program"));
    }
#endif
}

void KMLaunchExternalComponent::slotExportData()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)

    const KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("org.kde.pimdataexporter"));
    if (service) {
        auto job = new KIO::ApplicationLauncherJob(service);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mParentWidget->window()));
        job->start();
    } else {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start \"PIM Data Exporter\" program. "
                                "Please check your installation."),
                           i18nc("@title:window", "Unable to start \"PIM Data Exporter\" program"));
    }
#else
    const QString path = PimCommon::Util::findExecutable(QStringLiteral("pimdataexporter"));
    if (path.isEmpty() || !QProcess::startDetached(path)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start \"PIM Data Exporter\" program. "
                                "Please check your installation."),
                           i18nc("@title:window", "Unable to start \"PIM Data Exporter\" program"));
    }
#endif
}

void KMLaunchExternalComponent::slotRunAddressBook()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kaddressbook"), {}, this);
    job->setDesktopName(QStringLiteral("org.kde.kaddressbook"));
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mParentWidget->window()));
    job->start();
#else
    const QString path = PimCommon::Util::findExecutable(QStringLiteral("kaddressbook"));
    if (path.isEmpty() || !QProcess::startDetached(path)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start \"KAddressbook\" program. "
                                "Please check your installation."),
                           i18nc("@title:window", "Unable to start \"KAddressbook\" program"));
    }
#endif
}

void KMLaunchExternalComponent::slotImport()
{
    const QStringList lst = {QStringLiteral("--mode"), QStringLiteral("manual")};
    const QString path = PimCommon::Util::findExecutable(QStringLiteral("akonadiimportwizard"));
    if (path.isEmpty() || !QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget,
                           i18n("Could not start the ImportWizard. "
                                "Please make sure you have ImportWizard properly installed."),
                           i18nc("@title:window", "Unable to start ImportWizard"));
    }
}

void KMLaunchExternalComponent::slotAccountWizard()
{
    KMail::Util::executeAccountWizard(mParentWidget);
}

void KMLaunchExternalComponent::slotFilterLogViewer()
{
    MailCommon::FilterManager::instance()->showFilterLogDialog(static_cast<qlonglong>(mParentWidget->winId()));
}

void KMLaunchExternalComponent::slotShowNotificationHistory()
{
    const auto service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_newmailnotifier_agent"));
    auto newMailNotifierInterface =
        new OrgFreedesktopAkonadiNewMailNotifierInterface(service, QStringLiteral("/NewMailNotifierAgent"), QDBusConnection::sessionBus(), this);
    if (!newMailNotifierInterface->isValid()) {
        qCDebug(KMAIL_LOG) << " org.freedesktop.Akonadi.NewMailNotifierAgent not found. Please verify your installation";
    } else {
        newMailNotifierInterface->showNotNotificationHistoryDialog(0); // TODO fix me windid
    }
    delete newMailNotifierInterface;
}

#include "moc_kmlaunchexternalcomponent.cpp"
