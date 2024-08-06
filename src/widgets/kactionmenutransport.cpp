/*
   SPDX-FileCopyrightText: 2015-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kactionmenutransport.h"
#include "config-kmail.h"
#include <MailTransport/TransportManager>
#include <QMenu>
#if HAVE_ACTIVITY_SUPPORT
#include "activities/transportactivities.h"
#endif
KActionMenuTransport::KActionMenuTransport(QObject *parent)
    : KActionMenu(parent)
{
    setPopupMode(QToolButton::DelayedPopup);
    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportsChanged, this, &KActionMenuTransport::updateTransportMenu);
    connect(menu(), &QMenu::aboutToShow, this, &KActionMenuTransport::slotCheckTransportMenu);
    connect(menu(), &QMenu::triggered, this, &KActionMenuTransport::slotSelectTransport);
}

KActionMenuTransport::~KActionMenuTransport() = default;

void KActionMenuTransport::slotCheckTransportMenu()
{
    if (!mInitialized) {
        mInitialized = true;
        updateTransportMenu();
    }
}

#if HAVE_ACTIVITY_SUPPORT
void KActionMenuTransport::setIdentityActivitiesAbstract(TransportActivities *activities)
{
    mTransportActivities = activities;
}
#endif

void KActionMenuTransport::updateTransportMenu()
{
    if (mInitialized) {
        menu()->clear();
        const QList<MailTransport::Transport *> transports = MailTransport::TransportManager::self()->transports();
        QMap<QString, int> menuTransportLst;

        for (MailTransport::Transport *transport : transports) {
#if HAVE_ACTIVITY_SUPPORT
            if (mTransportActivities) {
                if (mTransportActivities->hasActivitySupport() && !mTransportActivities->filterAcceptsRow(transport->activities())) {
                    break;
                }
            }
#endif
            const QString name = transport->name().replace(QLatin1Char('&'), QStringLiteral("&&"));
            menuTransportLst.insert(name, transport->id());
        }
        QMapIterator<QString, int> i(menuTransportLst);
        while (i.hasNext()) {
            i.next();
            auto action = new QAction(i.key(), this);
            action->setData(i.value());
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

#include "moc_kactionmenutransport.cpp"
