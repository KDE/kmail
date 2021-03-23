/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <KActionMenu>
namespace MailTransport
{
class Transport;
}

class KMAILTESTS_TESTS_EXPORT KActionMenuTransport : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuTransport(QObject *parent = nullptr);
    ~KActionMenuTransport() override;

Q_SIGNALS:
    void transportSelected(MailTransport::Transport *transport);

private:
    void updateTransportMenu();
    void slotCheckTransportMenu();
    void slotSelectTransport(QAction *act);

    bool mInitialized = false;
};

