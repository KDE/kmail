/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "kmlaunchexternalcomponent.h"
#include <KMessageBox>
#include <KLocalizedString>
#include <KRun>

#include "util.h"
#include "archivemailagentinterface.h"
#include "sendlateragentinterface.h"
#include "followupreminderinterface.h"
#include "mailcommon/filter/filtermanager.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QProcess>
#include "kmail_debug.h"
#include <QStandardPaths>
#include <antispam-virus/antispamwizard.h>

KMLaunchExternalComponent::KMLaunchExternalComponent(QWidget *parentWidget, QObject *parent)
    : QObject(parent),
      mParentWidget(parentWidget)
{

}

KMLaunchExternalComponent::~KMLaunchExternalComponent()
{

}

void KMLaunchExternalComponent::slotConfigureAutomaticArchiving()
{
    OrgFreedesktopAkonadiArchiveMailAgentInterface archiveMailInterface(QLatin1String("org.freedesktop.Akonadi.ArchiveMailAgent"), QLatin1String("/ArchiveMailAgent"), QDBusConnection::sessionBus(), this);
    if (archiveMailInterface.isValid()) {
        archiveMailInterface.showConfigureDialog((qlonglong)mParentWidget->winId());
    } else {
        KMessageBox::error(mParentWidget, i18n("Archive Mail Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotConfigureSendLater()
{
    OrgFreedesktopAkonadiSendLaterAgentInterface sendLaterInterface(QLatin1String("org.freedesktop.Akonadi.SendLaterAgent"), QLatin1String("/SendLaterAgent"), QDBusConnection::sessionBus(), this);
    if (sendLaterInterface.isValid()) {
        sendLaterInterface.showConfigureDialog((qlonglong)mParentWidget->winId());
    } else {
        KMessageBox::error(mParentWidget, i18n("Send Later Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotConfigureFollowupReminder()
{
    OrgFreedesktopAkonadiFollowUpReminderAgentInterface followUpInterface(QLatin1String("org.freedesktop.Akonadi.FollowUpReminder"), QLatin1String("/FollowUpReminder"), QDBusConnection::sessionBus(), this);
    if (followUpInterface.isValid()) {
        followUpInterface.showConfigureDialog((qlonglong)mParentWidget->winId());
    } else {
        KMessageBox::error(mParentWidget, i18n("Followup Reminder Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotStartCertManager()
{
    if (!QProcess::startDetached(QLatin1String("kleopatra")))
        KMessageBox::error(mParentWidget, i18n("Could not start certificate manager; "
                                               "please check your installation."),
                           i18n("KMail Error"));
    else {
        qCDebug(KMAIL_LOG) << "slotStartCertManager(): certificate manager started.";
    }
}

void KMLaunchExternalComponent::slotStartWatchGnuPG()
{
    if (!QProcess::startDetached(QLatin1String("kwatchgnupg")))
        KMessageBox::error(mParentWidget, i18n("Could not start GnuPG LogViewer (kwatchgnupg); "
                                               "please check your installation."),
                           i18n("KMail Error"));
}

void KMLaunchExternalComponent::slotImportWizard()
{
    const QString path = QStandardPaths::findExecutable(QLatin1String("importwizard"));
    if (!QProcess::startDetached(path))
        KMessageBox::error(mParentWidget, i18n("Could not start the import wizard. "
                                               "Please check your installation."),
                           i18n("Unable to start import wizard"));
}

void KMLaunchExternalComponent::slotExportData()
{
    const QString path = QStandardPaths::findExecutable(QLatin1String("pimsettingexporter"));
    if (!QProcess::startDetached(path))
        KMessageBox::error(mParentWidget, i18n("Could not start \"PIM Setting Exporter\" program. "
                                               "Please check your installation."),
                           i18n("Unable to start \"PIM Setting Exporter\" program"));
}

void KMLaunchExternalComponent::slotAddrBook()
{
    KRun::runCommand(QLatin1String("kaddressbook"), mParentWidget->window());
}

void KMLaunchExternalComponent::slotImport()
{
    KRun::runCommand(QLatin1String("kmailcvt"), mParentWidget->window());
}

void KMLaunchExternalComponent::slotAccountWizard()
{
    QStringList lst;
    lst.append(QLatin1String("--type"));
    lst.append(QLatin1String("message/rfc822"));

    const QString path = QStandardPaths::findExecutable(QLatin1String("accountwizard"));
    if (!QProcess::startDetached(path, lst))
        KMessageBox::error(mParentWidget, i18n("Could not start the account wizard. "
                                               "Please check your installation."),
                           i18n("Unable to start account wizard"));
}

void KMLaunchExternalComponent::slotAntiSpamWizard()
{
    KMail::AntiSpamWizard wiz( KMail::AntiSpamWizard::AntiSpam, mParentWidget );
    wiz.exec();
}

void KMLaunchExternalComponent::slotAntiVirusWizard()
{
    KMail::AntiSpamWizard wiz( KMail::AntiSpamWizard::AntiVirus, mParentWidget);
    wiz.exec();
}

void KMLaunchExternalComponent::slotFilterLogViewer()
{
    MailCommon::FilterManager::instance()->showFilterLogDialog( (qlonglong)mParentWidget->winId() );
}
