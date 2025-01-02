/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "config-kmail.h"
#include "kmail_private_export.h"
#include <KActionMenu>
namespace MailTransport
{
class Transport;
}
#if KMAIL_HAVE_ACTIVITY_SUPPORT
class TransportActivities;
#endif
class KMAILTESTS_TESTS_EXPORT KActionMenuTransport : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuTransport(QObject *parent = nullptr);
    ~KActionMenuTransport() override;
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    void setTransportActivitiesAbstract(TransportActivities *activities);
#endif
Q_SIGNALS:
    void transportSelected(MailTransport::Transport *transport);

private:
    KMAIL_NO_EXPORT void updateTransportMenu();
    KMAIL_NO_EXPORT void slotCheckTransportMenu();
    KMAIL_NO_EXPORT void slotSelectTransport(QAction *act);
    KMAIL_NO_EXPORT void forceUpdateTransportMenu();

    bool mInitialized = false;
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    TransportActivities *mTransportActivities = nullptr;
#endif
};
