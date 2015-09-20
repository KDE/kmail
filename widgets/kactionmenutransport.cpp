/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "kactionmenutransport.h"
#include <QMenu>
#include <mailtransport/transportmanager.h>

KActionMenuTransport::KActionMenuTransport(QObject *parent)
    : KActionMenu(parent),
      mInitialized(false)
{
    setDelayed(true);
    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportsChanged, this, &KActionMenuTransport::updateTransportMenu);
    connect(menu(), &QMenu::aboutToShow, this, &KActionMenuTransport::slotCheckTransportMenu);
    connect(menu(), &QMenu::triggered, this, &KActionMenuTransport::slotSelectTransport);
}

KActionMenuTransport::~KActionMenuTransport()
{

}

void KActionMenuTransport::slotCheckTransportMenu()
{
    if (!mInitialized) {
        mInitialized = true;
        updateTransportMenu();
    }
}

void KActionMenuTransport::updateTransportMenu()
{
    if (mInitialized) {
        menu()->clear();
        const QList<MailTransport::Transport *> transports = MailTransport::TransportManager::self()->transports();
        Q_FOREACH (MailTransport::Transport *transport, transports) {
            const QString name = transport->name().replace(QLatin1Char('&'), QStringLiteral("&&"));
            QAction *action = new QAction(name, this);
            action->setData(transport->id());
            menu()->addAction(action);
        }
    }
}

void KActionMenuTransport::slotSelectTransport(QAction *act)
{
    const QList<int> availTransports = MailTransport::TransportManager::self()->transportIds();
    const int transportId = act->data().toInt();
    if (availTransports.contains(transportId)) {
        MailTransport::Transport *transport = MailTransport::TransportManager::self()->transportById(transportId);
        Q_EMIT transportSelected(transport);
    }
}
