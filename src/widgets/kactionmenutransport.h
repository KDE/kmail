/*
   SPDX-FileCopyrightText: 2015-2024 Laurent Montel <montel@kde.org>

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
#if HAVE_ACTIVITY_SUPPORT
class TransportActivities;
#endif
class KMAILTESTS_TESTS_EXPORT KActionMenuTransport : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuTransport(QObject *parent = nullptr);
    ~KActionMenuTransport() override;
#if HAVE_ACTIVITY_SUPPORT
    void setIdentityActivitiesAbstract(TransportActivities *activities);
#endif
Q_SIGNALS:
    void transportSelected(MailTransport::Transport *transport);

private:
    KMAIL_NO_EXPORT void updateTransportMenu();
    KMAIL_NO_EXPORT void slotCheckTransportMenu();
    KMAIL_NO_EXPORT void slotSelectTransport(QAction *act);

    bool mInitialized = false;
#if HAVE_ACTIVITY_SUPPORT
    TransportActivities *mTransportActivities = nullptr;
#endif
};
