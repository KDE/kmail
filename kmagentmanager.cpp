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

#include "kmagentmanager.h"
#include <QDebug>
#include <QStringList>

KMAgentManager::KMAgentManager( QObject *parent )
  : QObject( parent )
  , mAgentManager( Akonadi::AgentManager::self() )
{
  init();
}

void KMAgentManager::init()
{
  Akonadi::AgentInstance::List lst = mAgentManager->instances();
  foreach ( const Akonadi::AgentInstance& type, lst )
  {
    if ( type.type().mimeTypes().contains(  "message/rfc822" ) ) {
      mListInstance << type;
    }
  }
}

Akonadi::AgentInstance::List KMAgentManager::instanceList() const
{
  return mListInstance;
}

Akonadi::AgentInstance KMAgentManager::instance( const QString &name)
{
  return mAgentManager->instance( name );
}

bool KMAgentManager::isEmpty() const
{
  return mListInstance.isEmpty();
}

void KMAgentManager::slotInstanceAdded( const Akonadi::AgentInstance & instance)
{
  qDebug()<<" KMAgentManager::slotInstanceAdded :"<<instance.name();
  if ( instance.type().mimeTypes().contains( "message/rfc822" ) )
    mListInstance << instance;
}

void KMAgentManager::slotInstanceRemoved( const Akonadi::AgentInstance &instance )
{
  qDebug()<<" KMAgentManager::slotInstanceRemoved :"<<instance.name();
  if ( mListInstance.contains( instance ) )
    mListInstance.removeAll( instance );
}

#include "kmagentmanager.moc"

