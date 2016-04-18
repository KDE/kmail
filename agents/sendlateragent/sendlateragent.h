/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

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

#ifndef SENDLATERAGENT_H
#define SENDLATERAGENT_H

#include <agentbase.h>
#include "sendlaterdialog.h"

class SendLaterManager;

class SendLaterAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit SendLaterAgent(const QString &id);
    ~SendLaterAgent();

    void showConfigureDialog(qlonglong windowId = 0);

    QString printDebugInfo();

    void setEnableAgent(bool b);
    bool enabledAgent() const;

Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void reload();
    void configure(WId windowId) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotSendNow(Akonadi::Item::Id id);

    void slotStartAgent();
protected:
    void itemsRemoved(const Akonadi::Item::List &item) Q_DECL_OVERRIDE;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection) Q_DECL_OVERRIDE;
    void doSetOnline(bool online) Q_DECL_OVERRIDE;

private:
    bool mAgentInitialized;
    SendLaterManager *mManager;
};

#endif // SENDLATERAGENT_H
