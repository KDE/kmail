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
    OrgFreedesktopAkonadiArchiveMailAgentInterface archiveMailInterface(QStringLiteral("org.freedesktop.Akonadi.ArchiveMailAgent"), QStringLiteral("/ArchiveMailAgent"), QDBusConnection::sessionBus(), this);
    if (archiveMailInterface.isValid()) {
        archiveMailInterface.showConfigureDialog((qlonglong)mParentWidget->winId());
    } else {
        KMessageBox::error(mParentWidget, i18n("Archive Mail Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotConfigureSendLater()
{
    OrgFreedesktopAkonadiSendLaterAgentInterface sendLaterInterface(QStringLiteral("org.freedesktop.Akonadi.SendLaterAgent"), QStringLiteral("/SendLaterAgent"), QDBusConnection::sessionBus(), this);
    if (sendLaterInterface.isValid()) {
        sendLaterInterface.showConfigureDialog((qlonglong)mParentWidget->winId());
    } else {
        KMessageBox::error(mParentWidget, i18n("Send Later Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotConfigureFollowupReminder()
{
    OrgFreedesktopAkonadiFollowUpReminderAgentInterface followUpInterface(QStringLiteral("org.freedesktop.Akonadi.FollowUpReminder"), QStringLiteral("/FollowUpReminder"), QDBusConnection::sessionBus(), this);
    if (followUpInterface.isValid()) {
        followUpInterface.showConfigureDialog((qlonglong)mParentWidget->winId());
    } else {
        KMessageBox::error(mParentWidget, i18n("Followup Reminder Agent was not registered."));
    }
}

void KMLaunchExternalComponent::slotStartCertManager()
{
    if (!QProcess::startDetached(QStringLiteral("kleopatra"))) {
        KMessageBox::error(mParentWidget, i18n("Could not start certificate manager; "
                                               "please check your installation."),
                           i18n("KMail Error"));
    }
}

void KMLaunchExternalComponent::slotStartWatchGnuPG()
{
    if (!QProcess::startDetached(QStringLiteral("kwatchgnupg"))) {
        KMessageBox::error(mParentWidget, i18n("Could not start GnuPG LogViewer (kwatchgnupg); "
                                               "please check your installation."),
                           i18n("KMail Error"));
    }
}

void KMLaunchExternalComponent::slotImportWizard()
{
    const QString path = QStandardPaths::findExecutable(QStringLiteral("importwizard"));
    if (!QProcess::startDetached(path)) {
        KMessageBox::error(mParentWidget, i18n("Could not start the import wizard. "
                                               "Please check your installation."),
                           i18n("Unable to start import wizard"));
    }
}

void KMLaunchExternalComponent::slotExportData()
{
    const QString path = QStandardPaths::findExecutable(QStringLiteral("pimsettingexporter"));
    if (!QProcess::startDetached(path)) {
        KMessageBox::error(mParentWidget, i18n("Could not start \"PIM Setting Exporter\" program. "
                                               "Please check your installation."),
                           i18n("Unable to start \"PIM Setting Exporter\" program"));
    }
}

void KMLaunchExternalComponent::slotAddrBook()
{
    KRun::runCommand(QStringLiteral("kaddressbook"), mParentWidget->window());
}

void KMLaunchExternalComponent::slotImport()
{
    QStringList lst;
    lst.append(QStringLiteral("--mode"));
    lst.append(QStringLiteral("manual"));
    const QString path = QStandardPaths::findExecutable(QStringLiteral("importwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget, i18n("Could not start the ImportWizard. "
                                               "Please check your installation."),
                           i18n("Unable to start ImportWizard"));
    }
}

void KMLaunchExternalComponent::slotAccountWizard()
{
    QStringList lst;
    lst.append(QStringLiteral("--type"));
    lst.append(QStringLiteral("message/rfc822"));

    const QString path = QStandardPaths::findExecutable(QStringLiteral("accountwizard"));
    if (!QProcess::startDetached(path, lst)) {
        KMessageBox::error(mParentWidget, i18n("Could not start the account wizard. "
                                               "Please check your installation."),
                           i18n("Unable to start account wizard"));
    }
}

void KMLaunchExternalComponent::slotAntiSpamWizard()
{
    KMail::AntiSpamWizard wiz(KMail::AntiSpamWizard::AntiSpam, mParentWidget);
    wiz.exec();
}

void KMLaunchExternalComponent::slotAntiVirusWizard()
{
    KMail::AntiSpamWizard wiz(KMail::AntiSpamWizard::AntiVirus, mParentWidget);
    wiz.exec();
}

void KMLaunchExternalComponent::slotFilterLogViewer()
{
    MailCommon::FilterManager::instance()->showFilterLogDialog((qlonglong)mParentWidget->winId());
}

