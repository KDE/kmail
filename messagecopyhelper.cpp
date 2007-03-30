/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "messagecopyhelper.h"

#include "kmcommands.h"
#include "kmfolder.h"
#include "kmmsgdict.h"

using namespace KMail;
using namespace KPIM;

MessageCopyHelper::MessageCopyHelper( const QValueList< Q_UINT32 > & msgs,
                                      KMFolder * dest, bool move, QObject * parent ) :
    QObject( parent )
{
  if ( msgs.isEmpty() || !dest )
    return;

  KMFolder *f = 0;
  int index;
  QPtrList<KMMsgBase> list;

  for ( QValueList<Q_UINT32>::ConstIterator it = msgs.constBegin(); it != msgs.constEnd(); ++it ) {
    KMMsgDict::instance()->getLocation( *it, &f, &index );
    if ( !f ) // not found
      continue;
    if ( f == dest )
      continue; // already there
    if ( !mOpenFolders.contains( f ) ) {// not yet opened
      f->open("messagecopyhelper");
      mOpenFolders.insert( f, 0 );
    }
    KMMsgBase *msgBase = f->getMsgBase( index );
    if ( msgBase )
      list.append( msgBase );
  }

  if ( list.isEmpty() )
    return; // nothing to do

  KMCommand *command;
  if ( move ) {
    command = new KMMoveCommand( dest, list );
  } else {
    command = new KMCopyCommand( dest, list );
  }

  connect( command, SIGNAL(completed(KMCommand*)), SLOT(copyCompleted(KMCommand*)) );
  command->start();
}

void MessageCopyHelper::copyCompleted(KMCommand * cmd)
{
  Q_UNUSED( cmd );

  // close all folders we opened
  for ( QMap<QGuardedPtr<KMFolder>, int>::ConstIterator it = mOpenFolders.constBegin();
        it != mOpenFolders.constEnd(); ++it ) {
    it.key()->close("messagecopyhelper");
  }
  mOpenFolders.clear();
  deleteLater();
}

QValueList< Q_UINT32 > MessageCopyHelper::serNumListFromMailList(const KPIM::MailList & list)
{
  QValueList<Q_UINT32> rv;
  for ( MailList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
    rv.append( (*it).serialNumber() );
  return rv;
}

QValueList< Q_UINT32 > MessageCopyHelper::serNumListFromMsgList(QPtrList< KMMsgBase > list)
{
  QValueList<Q_UINT32> rv;
  KMMsgBase* msg = list.first();
  while( msg ) {
    rv.append( msg->getMsgSerNum() );
    msg = list.next();
  }
  return rv;
}

#include "messagecopyhelper.moc"
