/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kactionmenuaccount.h"
#include "kmail_debug.h"
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include <MailCommon/MailUtil>
#include <QMenu>

KActionMenuAccount::KActionMenuAccount(QObject *parent)
    : KActionMenu(parent)
{
    setPopupMode(QToolButton::DelayedPopup);
    connect(menu(), &QMenu::aboutToShow, this, &KActionMenuAccount::slotCheckTransportMenu);
    connect(menu(), &QMenu::triggered, this, &KActionMenuAccount::slotSelectAccount);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceNameChanged, this, &KActionMenuAccount::updateAccountMenu);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &KActionMenuAccount::updateAccountMenu);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceAdded, this, &KActionMenuAccount::updateAccountMenu);
}

KActionMenuAccount::~KActionMenuAccount() = default;

void KActionMenuAccount::setAccountOrder(const QStringList &identifier)
{
    if (mOrderIdentifier != identifier) {
        mOrderIdentifier = identifier;
        mInitialized = false;
    }
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

bool orderAgentIdentifier(const AgentIdentifier &lhs, const AgentIdentifier &rhs)
{
    if ((lhs.mIndex == -1) && (rhs.mIndex == -1)) {
        return lhs.mName < rhs.mName;
    }
    if (lhs.mIndex != rhs.mIndex) {
        return lhs.mIndex < rhs.mIndex;
    }
    // we can't have same index but fallback
    return true;
}

void KActionMenuAccount::updateAccountMenu()
{
    if (mInitialized) {
        menu()->clear();
        const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
        QVector<AgentIdentifier> agentIdentifierList;
        agentIdentifierList.reserve(lst.count());

        for (const Akonadi::AgentInstance &type : lst) {
            // Explicitly make a copy, as we're not changing values of the list but only
            // the local copy which is passed to action.
            const QString identifierName = type.identifier();
            const int index = mOrderIdentifier.indexOf(identifierName);
            const AgentIdentifier id(identifierName, QString(type.name()).replace(QLatin1Char('&'), QStringLiteral("&&")), index);
            agentIdentifierList << id;
        }
        std::sort(agentIdentifierList.begin(), agentIdentifierList.end(), orderAgentIdentifier);
        const int numberOfAccount(agentIdentifierList.size());
        for (int i = 0; i < numberOfAccount; ++i) {
            const AgentIdentifier id = agentIdentifierList.at(i);
            QAction *action = menu()->addAction(id.mName);
            action->setData(id.mIdentifier);
        }
    }
}
