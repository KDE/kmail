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
#include <QStringList>
#include "kmagentinstance.h"

KMAgentManager::KMAgentManager( QObject *parent )
  : QObject( parent )
  , mAgentManager( Akonadi::AgentManager::self() )
{
  init();
}

KMAgentManager::~KMAgentManager()
{
  //kDebug()<<" KMAgentManager::~KMAgentManager";
  QMapIterator<QString, KMAgentInstance*> i(lstAgentInstance);
  while (i.hasNext()) {
    i.next();
    delete i.value();
  }
  lstAgentInstance.clear();
}

void KMAgentManager::init()
{
  Akonadi::AgentInstance::List lst = mAgentManager->instances();
  foreach ( const Akonadi::AgentInstance& type, lst )
  {
    if ( type.type().mimeTypes().contains(  "message/rfc822" ) && type.type().capabilities().contains( "Resource" )) {
      mListInstance << type;
      KMAgentInstance *agent = new KMAgentInstance( this, type );
      lstAgentInstance.insert( type.identifier(), agent );

    }
  }
  connect( mAgentManager, SIGNAL( instanceAdded( const Akonadi::AgentInstance & ) ), this, SLOT( slotInstanceAdded( const Akonadi::AgentInstance& ) ) );
  connect( mAgentManager, SIGNAL( instanceRemoved( const Akonadi::AgentInstance & ) ), this, SLOT( slotInstanceRemoved( const Akonadi::AgentInstance& ) ) );

}

void KMAgentManager::slotInstanceAdded( const Akonadi::AgentInstance & instance)
{
  //kDebug()<<" KMAgentManager::slotInstanceAdded :"<<instance.name();
  if ( instance.type().mimeTypes().contains( "message/rfc822" ) ) {
    mListInstance << instance;
    KMAgentInstance *agent = new KMAgentInstance( this, instance );
    lstAgentInstance.insert( instance.identifier(), agent );
  }
}

void KMAgentManager::slotInstanceRemoved( const Akonadi::AgentInstance &instance )
{
  //kDebug()<<" KMAgentManager::slotInstanceRemoved :"<<instance.name();
  if ( mListInstance.contains( instance ) ) {
    mListInstance.removeAll( instance );
    lstAgentInstance.remove( instance.identifier() );
  }
}

#include "kmagentmanager.moc"

