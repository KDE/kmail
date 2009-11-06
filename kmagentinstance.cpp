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
#include <QDebug>
#include <KLocale>
#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;

KMAgentInstance::KMAgentInstance( QObject *parent, const Akonadi::AgentInstance & inst )
  :QObject( parent ), mAgentInstance( inst ), mProgressItem( 0 )
{
  qDebug()<<" KMAgentInstance::KMAgentInstance";
}

KMAgentInstance::~KMAgentInstance()
{
  qDebug()<<" KMAgentInstance::~KMAgentInstance";
}

void KMAgentInstance::progressChanged(int progress)
{
  if ( !mProgressItem ) {
    mProgressItem = ProgressManager::createProgressItem ("MailCheck"+mAgentInstance.name(),i18n("Preparing transmission from \"%1\"...", mAgentInstance.name() ) );
    connect( mProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ), this, SLOT( slotCanceled() ) ) ;
  }
  mProgressItem->setProgress( progress );
  if ( progress == 100 ) {
    mProgressItem->setComplete();
    mProgressItem = 0;
  }
  qDebug()<<" progress changed :"<<progress;
}

void KMAgentInstance::slotCanceled()
{
  mAgentInstance.abortCurrentTask();
  mProgressItem->reset();
  mProgressItem = 0;
}

#include "kmagentinstance.moc"

