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

MessageCopyHelper::MessageCopyHelper( const QList<quint32> & msgs,
                                      KMFolder * dest, bool move, QObject * parent ) :
    QObject( parent )
{
  if ( msgs.isEmpty() || !dest )
    return;

  KMFolder *f = 0;
  int index;
  QList<KMMsgBase*> list;

  for ( QList<quint32>::ConstIterator it = msgs.constBegin(); it != msgs.constEnd(); ++it ) {
    KMMsgDict::instance()->getLocation( *it, &f, &index );
    if ( !f ) // not found
      continue;
    if ( f == dest )
      continue; // already there
    if ( !mOpenFolders.contains( f ) ) {// not yet opened
      f->open( "messagecopy" );
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
  for ( QMap<QPointer<KMFolder>, int>::ConstIterator it = mOpenFolders.constBegin();
        it != mOpenFolders.constEnd(); ++it ) {
    it.key()->close( "messagecopy" );
  }
  mOpenFolders.clear();
  deleteLater();
}

QList<quint32> MessageCopyHelper::serNumListFromMailList(const KPIM::MailList & list)
{
  QList<quint32> rv;
  for ( MailList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
    rv.append( (*it).serialNumber() );
  return rv;
}

QList<quint32> MessageCopyHelper::serNumListFromMsgList(QList<KMMsgBase*> list)
{
  QList<quint32> rv;
  foreach ( KMMsgBase *msg, list )
    if ( msg )
      rv.append( msg->getMsgSerNum() );
  return rv;
}

bool MessageCopyHelper::inReadOnlyFolder(const QList< quint32 > & sernums)
{
  KMFolder *f = 0;
  int index;
  for ( QList<quint32>::ConstIterator it = sernums.begin(); it != sernums.end(); ++it ) {
    KMMsgDict::instance()->getLocation( *it, &f, &index );
    if ( !f ) // not found
      continue;
    if ( f->isReadOnly() )
      return true;
  }
  return false;
}

#include "messagecopyhelper.moc"
