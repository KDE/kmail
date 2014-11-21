/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "kmconfigureagent.h"
#include <KMessageBox>
#include <KLocalizedString>

#include "archivemailagentinterface.h"
#include "sendlateragentinterface.h"
#include "followupreminderinterface.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

KMConfigureAgent::KMConfigureAgent(QWidget *parentWidget, QObject *parent)
    : QObject(parent),
      mParentWidget(parentWidget)
{

}

KMConfigureAgent::~KMConfigureAgent()
{

}

void KMConfigureAgent::slotConfigureAutomaticArchiving()
{
    OrgFreedesktopAkonadiArchiveMailAgentInterface archiveMailInterface(QLatin1String("org.freedesktop.Akonadi.ArchiveMailAgent"), QLatin1String("/ArchiveMailAgent"),QDBusConnection::sessionBus(), this);
    if (archiveMailInterface.isValid()) {
        archiveMailInterface.showConfigureDialog( (qlonglong)mParentWidget->winId() );
    } else {
        KMessageBox::error(mParentWidget,i18n("Archive Mail Agent was not registered."));
    }
}

void KMConfigureAgent::slotConfigureSendLater()
{
    OrgFreedesktopAkonadiSendLaterAgentInterface sendLaterInterface(QLatin1String("org.freedesktop.Akonadi.SendLaterAgent"), QLatin1String("/SendLaterAgent"),QDBusConnection::sessionBus(), this);
    if (sendLaterInterface.isValid()) {
        sendLaterInterface.showConfigureDialog( (qlonglong)mParentWidget->winId() );
    } else {
        KMessageBox::error(mParentWidget,i18n("Send Later Agent was not registered."));
    }
}

void KMConfigureAgent::slotConfigureFollowupReminder()
{
    OrgFreedesktopAkonadiFollowUpReminderAgentInterface followUpInterface(QLatin1String("org.freedesktop.Akonadi.FollowUpReminder"), QLatin1String("/FollowUpReminder"),QDBusConnection::sessionBus(), this);
    if (followUpInterface.isValid()) {
        followUpInterface.showConfigureDialog( (qlonglong)mParentWidget->winId() );
    } else {
        KMessageBox::error(mParentWidget,i18n("Followup Reminder Agent was not registered."));
    }
}
