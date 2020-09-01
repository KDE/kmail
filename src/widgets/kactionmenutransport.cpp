/*
   SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kactionmenutransport.h"
#include <QMenu>
#include <MailTransport/TransportManager>

KActionMenuTransport::KActionMenuTransport(QObject *parent)
    : KActionMenu(parent)
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
        QMap<QString, int> menuTransportLst;

        for (MailTransport::Transport *transport : transports) {
            const QString name = transport->name().replace(QLatin1Char('&'), QStringLiteral("&&"));
            menuTransportLst.insert(name, transport->id());
        }
        QMapIterator<QString, int> i(menuTransportLst);
        while (i.hasNext()) {
            i.next();
            QAction *action = new QAction(i.key(), this);
            action->setData(i.value());
            menu()->addAction(action);
        }
    }
}

void KActionMenuTransport::slotSelectTransport(QAction *act)
{
    const QVector<int> availTransports = MailTransport::TransportManager::self()->transportIds();
    const int transportId = act->data().toInt();
    if (availTransports.contains(transportId)) {
        MailTransport::Transport *transport = MailTransport::TransportManager::self()->transportById(transportId);
        Q_EMIT transportSelected(transport);
    }
}
