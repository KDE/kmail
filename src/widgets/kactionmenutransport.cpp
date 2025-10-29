/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kactionmenutransport.h"
#include <MailTransport/TransportManager>
#include <QMenu>
#if KMAIL_HAVE_ACTIVITY_SUPPORT
#include "activities/transportactivities.h"
#endif
KActionMenuTransport::KActionMenuTransport(QObject *parent)
    : KActionMenu(parent)
{
    setPopupMode(QToolButton::DelayedPopup);
    connect(MailTransport::TransportManager::self(),
            &MailTransport::TransportManager::transportsChanged,
            this,
            &KActionMenuTransport::forceUpdateTransportMenu);
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

#if KMAIL_HAVE_ACTIVITY_SUPPORT
void KActionMenuTransport::setTransportActivitiesAbstract(TransportActivities *activities)
{
    mTransportActivities = activities;
    connect(mTransportActivities, &TransportActivities::activitiesChanged, this, &KActionMenuTransport::forceUpdateTransportMenu);
}
#endif

void KActionMenuTransport::forceUpdateTransportMenu()
{
    mInitialized = false;
}

void KActionMenuTransport::updateTransportMenu()
{
    if (mInitialized) {
        menu()->clear();
        const QList<MailTransport::Transport *> transports = MailTransport::TransportManager::self()->transports();
        QMap<QString, int> menuTransportLst;

        for (MailTransport::Transport *transport : transports) {
#if KMAIL_HAVE_ACTIVITY_SUPPORT
            if (mTransportActivities) {
                if (mTransportActivities->hasActivitySupport() && !mTransportActivities->filterAcceptsRow(transport->activities())) {
                    continue;
                }
            }
#endif
            const QString name = transport->name().replace(QLatin1Char('&'), QStringLiteral("&&"));
            menuTransportLst.insert(name, transport->id());
        }
        for (const auto &[key, value] : menuTransportLst.asKeyValueRange()) {
            auto action = new QAction(key, this);
            action->setData(value);
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
