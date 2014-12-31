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

#include "manageserversidesubscriptionjob.h"
#include <KLocalizedString>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include "kmkernel.h"
#include <akonadi/dbusconnectionpool.h>
#include <KDebug>
#include <KMessageBox>

ManageServerSideSubscriptionJob::ManageServerSideSubscriptionJob(QObject *parent)
    : QObject(parent),
      mParentWidget(0)
{

}

ManageServerSideSubscriptionJob::~ManageServerSideSubscriptionJob()
{

}

void ManageServerSideSubscriptionJob::start()
{
    if (!mCurrentFolder) {
        qDebug()<<" currentFolder not defined";
        deleteLater();
        return;
    }
    bool isImapOnline = false;
    if ( kmkernel->isImapFolder( mCurrentFolder->collection(), isImapOnline ) ) {
        QDBusInterface iface(
                    QLatin1String( "org.freedesktop.Akonadi.Resource.")+mCurrentFolder->collection().resource(),
                    QLatin1String( "/" ), QLatin1String( "org.kde.Akonadi.ImapResourceBase" ),
                    Akonadi::DBusConnectionPool::threadConnection(), this );
        if ( !iface.isValid() ) {
            kDebug()<<"Cannot create imap dbus interface";
            deleteLater();
            return;
        }
        QDBusPendingCall call = iface.asyncCall( QLatin1String( "configureSubscription" ), (qlonglong)mParentWidget->winId() );
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(slotConfigureSubscriptionFinished(QDBusPendingCallWatcher*)));
    }
}

void ManageServerSideSubscriptionJob::slotConfigureSubscriptionFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<int> reply = *watcher;
    if ( reply.isValid() ) {
        if (reply == -2 ){
            KMessageBox::error(mParentWidget,i18n("IMAP server not configured yet. Please configure the server in the IMAP account before setting up server-side subscription."));
        } else if (reply == -1) {
            KMessageBox::error(mParentWidget,i18n("Log in failed, please configure the IMAP account before setting up server-side subscription."));
        }
    }
    watcher->deleteLater();
    watcher = 0;
    deleteLater();
}

void ManageServerSideSubscriptionJob::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

void ManageServerSideSubscriptionJob::setCurrentFolder(const QSharedPointer<MailCommon::FolderCollection> &currentFolder)
{
    mCurrentFolder = currentFolder;
}


