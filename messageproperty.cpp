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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "messageproperty.h"
using namespace KMail;

QMap<Q_UINT32, QGuardedPtr<KMFolder> > MessageProperty::sFolders;
QMap<Q_UINT32, QGuardedPtr<ActionScheduler> > MessageProperty::sHandlers;
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
  QMap<Q_UINT32, QGuardedPtr<KMFolder> >::const_iterator it = sFolders.find( serNum );
  return it == sFolders.constEnd() ? 0 : (*it).operator->();
}

void MessageProperty::setFilterFolder( Q_UINT32 serNum, KMFolder* folder )
{
  sFolders.insert(serNum, QGuardedPtr<KMFolder>(folder) );
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
  QMap<Q_UINT32, QGuardedPtr<ActionScheduler> >::const_iterator it = sHandlers.find( serNum );
  return it == sHandlers.constEnd() ? 0 : (*it).operator->();
}

void MessageProperty::setFilterHandler( Q_UINT32 serNum, ActionScheduler* handler )
{
  if (handler)
    sHandlers.insert( serNum, QGuardedPtr<ActionScheduler>(handler) );
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

bool MessageProperty::transferInProgress( Q_UINT32 serNum )
{
  QMap<Q_UINT32, int >::const_iterator it = sTransfers.find( serNum );
  return it == sTransfers.constEnd() ? false : *it;
}

void MessageProperty::setTransferInProgress( Q_UINT32 serNum, bool transfer, bool force )
{
  int transferInProgress = 0;
  QMap<Q_UINT32, int >::const_iterator it = sTransfers.find( serNum );
  if (it != sTransfers.constEnd())
    transferInProgress = *it;
  if ( force && !transfer )
    transferInProgress = 0;
  else
    transfer ? ++transferInProgress : --transferInProgress;
  if ( transferInProgress < 0 )
    transferInProgress = 0;
  if (transferInProgress)
    sTransfers.insert( serNum, transferInProgress );
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
  QMap<const KMMsgBase*, long >::const_iterator it = sSerialCache.find( msgBase );
  return it == sSerialCache.constEnd() ? 0 : *it;
}

void MessageProperty::setSerialCache( const KMMsgBase *msgBase, Q_UINT32 serNum )
{
  if (serNum)
    sSerialCache.insert( msgBase, serNum );
  else
    sSerialCache.remove( msgBase );
}

void MessageProperty::forget( const KMMsgBase *msgBase )
{
  Q_UINT32 serNum = serialCache( msgBase );
  if (serNum) {
    Q_ASSERT( !transferInProgress( serNum ) );
    sTransfers.remove( serNum );
    sSerialCache.remove( msgBase );
  }
}

#include "messageproperty.moc"
