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

#include "kactionmenuaccount.h"
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include "mailcommon/util/mailutil.h"
#include "kmail_debug.h"
#include <QMenu>

KActionMenuAccount::KActionMenuAccount(QObject *parent)
    : KActionMenu(parent),
      mInitialized(false)
{
    setDelayed(true);
    connect(menu(), &QMenu::aboutToShow, this, &KActionMenuAccount::slotCheckTransportMenu);
    connect(menu(), &QMenu::triggered, this, &KActionMenuAccount::slotSelectAccount);
    connect(Akonadi::AgentManager::self(), SIGNAL(instanceNameChanged(Akonadi::AgentInstance)), this, SLOT(updateAccountMenu()));
    connect(Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)), this, SLOT(updateAccountMenu()));
    connect(Akonadi::AgentManager::self(), SIGNAL(instanceAdded(Akonadi::AgentInstance)), this, SLOT(updateAccountMenu()));
}

KActionMenuAccount::~KActionMenuAccount()
{

}

void KActionMenuAccount::slotSelectAccount(QAction *act)
{
    if (!act) {
        return;
    }

    Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(act->data().toString());
    if (agent.isValid()) {
        if (!agent.isOnline()) {
            agent.setIsOnline(true);
        }
        agent.synchronize();
    } else {
        qCDebug(KMAIL_LOG) << "account with identifier" << act->data().toString() << "not found";
    }
}

void KActionMenuAccount::slotCheckTransportMenu()
{
    if (!mInitialized) {
        mInitialized = true;
        updateAccountMenu();
    }
}

void KActionMenuAccount::updateAccountMenu()
{
    if (mInitialized) {
        menu()->clear();
        const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
        Q_FOREACH (const Akonadi::AgentInstance &type, lst) {
            // Explicitly make a copy, as we're not changing values of the list but only
            // the local copy which is passed to action.
            QAction *action = menu()->addAction(QString(type.name()).replace(QLatin1Char('&'), QStringLiteral("&&")));
            action->setData(type.identifier());
        }
    }
}
