/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmagentinstance.h"
#include <akonadi/agentinstance.h>
#include <agentprogressmonitor.h>
#include <KLocale>
#include <KDebug>
#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;

KMAgentInstance::KMAgentInstance( QObject *parent, const Akonadi::AgentInstance & inst )
  :QObject( parent ), mAgentInstance( inst ), mProgressItem( 0 )
{
  kDebug()<<" KMAgentInstance::KMAgentInstance";
// TODO implement me!
//  ProgressItem *item = ProgressManager::createProgressItem( mAgentInstance.name() );
//  mProgressItem = ProgressManager::createProgressItem(item, "MailCheck"+mAgentInstance.name(),i18n("Preparing transmission from \"%1\"...", mAgentInstance.name() ) );
}

KMAgentInstance::~KMAgentInstance()
{
  //kDebug()<<" KMAgentInstance::~KMAgentInstance";
}

#include "kmagentinstance.moc"

