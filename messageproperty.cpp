/*  Message Property
    
    This file is part of KMail, the KDE mail client.
    Copyright (c) Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "messageproperty.h"
using namespace KMail;

QMap<Q_UINT32, QGuardedPtr<KMFolder> > MessageProperty::sFolders;
QMap<Q_UINT32, QGuardedPtr<ActionScheduler> > MessageProperty::sHandlers;
QMap<Q_UINT32, bool > MessageProperty::sCompletes;
QMap<Q_UINT32, int > MessageProperty::sTransfers;
QMap<const KMMsgBase*, long > MessageProperty::sSerialCache;

bool MessageProperty::filtering( Q_UINT32 serNum )
{
  return sFolders.contains( serNum );
}

void MessageProperty::setFiltering( Q_UINT32 serNum, bool filter )
{
  assert(!filtering(serNum) || !filter);
  if (filter && !filtering(serNum))
    sFolders.replace(serNum, QGuardedPtr<KMFolder>(0) );
  else if (!filter)
    sFolders.remove(serNum);
}

bool MessageProperty::filtering( const KMMsgBase *msgBase )
{
  return filtering( msgBase->getMsgSerNum() );
}

void MessageProperty::setFiltering( const KMMsgBase *msgBase, bool filter )
{
  setFiltering( msgBase->getMsgSerNum(), filter );
}

KMFolder* MessageProperty::filterFolder( Q_UINT32 serNum )
{
  if (sFolders.contains(serNum))
    return sFolders[serNum].operator->();
  return 0;
}

void MessageProperty::setFilterFolder( Q_UINT32 serNum, KMFolder* folder )
{
  sFolders.replace(serNum, QGuardedPtr<KMFolder>(folder) );
}

KMFolder* MessageProperty::filterFolder( const KMMsgBase *msgBase )
{
  return filterFolder( msgBase->getMsgSerNum() );
}

void MessageProperty::setFilterFolder( const KMMsgBase *msgBase, KMFolder* folder )
{
  setFilterFolder( msgBase->getMsgSerNum(), folder );
}

ActionScheduler* MessageProperty::filterHandler( Q_UINT32 serNum )
{
  if ( sHandlers.contains( serNum ))
    return sHandlers[serNum].operator->();
  return 0;
}

void MessageProperty::setFilterHandler( Q_UINT32 serNum, ActionScheduler* handler )
{
  if (handler)
    sHandlers.replace( serNum, QGuardedPtr<ActionScheduler>(handler) );
  else
    sHandlers.remove( serNum );
}

ActionScheduler* MessageProperty::filterHandler( const KMMsgBase *msgBase )
{
  return filterHandler( msgBase->getMsgSerNum() );
}

void MessageProperty::setFilterHandler( const KMMsgBase *msgBase, ActionScheduler* handler )
{
  setFilterHandler( msgBase->getMsgSerNum(), handler );
}

bool MessageProperty::complete( Q_UINT32 serNum )
{
  if (sCompletes.contains( serNum ))
    return sCompletes[serNum];
  return false;
}

void MessageProperty::setComplete( Q_UINT32 serNum, bool complete )
{
  if (complete)
    sCompletes.replace( serNum, complete );
  else
    sCompletes.remove( serNum);
}

bool MessageProperty::complete( const KMMsgBase *msgBase )
{
  return complete( msgBase->getMsgSerNum() );
}

void MessageProperty::setComplete( const KMMsgBase *msgBase, bool complete )
{
  setComplete( msgBase->getMsgSerNum(), complete );
}

bool MessageProperty::transferInProgress( Q_UINT32 serNum )
{
  if (sTransfers.contains(serNum))
    return sTransfers[serNum];
  return false;
}

void MessageProperty::setTransferInProgress( Q_UINT32 serNum, bool transfer, bool force )
{
  int transferInProgress = 0;
  if (sTransfers.contains(serNum))
    transferInProgress = sTransfers[serNum];
  if ( force && !transfer )
    transferInProgress = 0;
  else
    transfer ? ++transferInProgress : --transferInProgress;
  if ( transferInProgress < 0 )
    transferInProgress = 0;
  if (transferInProgress)
    sTransfers.replace( serNum, transferInProgress );
  else
    sTransfers.remove( serNum );
}

bool MessageProperty::transferInProgress( const KMMsgBase *msgBase )
{
  return transferInProgress( msgBase->getMsgSerNum() );
}

void MessageProperty::setTransferInProgress( const KMMsgBase *msgBase, bool transfer, bool force )
{
  setTransferInProgress( msgBase->getMsgSerNum(), transfer, force );
}

Q_UINT32 MessageProperty::serialCache( const KMMsgBase *msgBase )
{
  if (sSerialCache.contains( msgBase ))
    return sSerialCache[msgBase];
  return 0;
}

void MessageProperty::setSerialCache( const KMMsgBase *msgBase, Q_UINT32 serNum )
{
  if (serNum)
    sSerialCache.replace( msgBase, serNum );
  else
    sSerialCache.remove( msgBase );
}

void MessageProperty::forget( const KMMsgBase *msgBase )
{
  Q_UINT32 serNum = serialCache( msgBase );
  if (serNum) {
    Q_ASSERT( !transferInProgress( serNum ) );
    sCompletes.remove( serNum );
    sTransfers.remove( serNum );
    sSerialCache.remove( msgBase );
  }
}

#include "messageproperty.moc"
