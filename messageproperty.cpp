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

#include "messageproperty.h"
#include "actionscheduler.h"

#include <kmime/kmime_content.h>
using namespace KMail;

QMap<Akonadi::Item::Id, Akonadi::Collection> MessageProperty::sFolders;
QMap<quint32, bool> MessageProperty::sKeepSerialNumber;
QMap<quint32, QPointer<ActionScheduler> > MessageProperty::sHandlers;
QMap<quint32, int > MessageProperty::sTransfers;
QMap<KMime::Content*, long > MessageProperty::sSerialCache;

bool MessageProperty::filtering( const Akonadi::Item &item )
{
  return sFolders.contains( item.id() );
}

void MessageProperty::setFiltering( const Akonadi::Item &item, bool filter )
{
  if ( filter )
    sFolders.insert( item.id(), Akonadi::Collection() );
  else
    sFolders.remove( item.id() );
}


void MessageProperty::setFilterFolder( const Akonadi::Item &item, const Akonadi::Collection &folder)
{
  sFolders.insert( item.id(), folder );
}

Akonadi::Collection MessageProperty::filterFolder( const Akonadi::Item &item )
{
  return sFolders.value( item.id() );
}

ActionScheduler* MessageProperty::filterHandler( quint32 serNum )
{
  QMap<quint32, QPointer<ActionScheduler> >::ConstIterator it = sHandlers.constFind( serNum );
  return it == sHandlers.constEnd() ? 0 : (*it).operator->();
}

void MessageProperty::setFilterHandler( quint32 serNum, ActionScheduler* handler )
{
  if (handler)
    sHandlers.insert( serNum, QPointer<ActionScheduler>(handler) );
  else
    sHandlers.remove( serNum );
}

ActionScheduler* MessageProperty::filterHandler( KMime::Content *msgBase )
{
#if 0 //TODO port to akonadi
  return filterHandler( msgBase->getMsgSerNum() );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return 0;
#endif
}

void MessageProperty::setFilterHandler( KMime::Content *msgBase, ActionScheduler* handler )
{
#if 0 //TODO port to akonadi
  setFilterHandler( msgBase->getMsgSerNum(), handler );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

bool MessageProperty::transferInProgress( quint32 serNum )
{
  QMap<quint32, int >::ConstIterator it = sTransfers.constFind( serNum );
  return it == sTransfers.constEnd() ? false : *it;
}

void MessageProperty::setTransferInProgress( quint32 serNum, bool transfer, bool force )
{
  int transferInProgress = 0;
  QMap<quint32, int >::ConstIterator it = sTransfers.constFind( serNum );
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

bool MessageProperty::transferInProgress( KMime::Content *msgBase )
{
#if 0 //TODO port to akonadi
  return transferInProgress( msgBase->getMsgSerNum() );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}

void MessageProperty::setTransferInProgress( KMime::Content *msgBase, bool transfer, bool force )
{
#if 0 //TODO port to akonadi
  setTransferInProgress( msgBase->getMsgSerNum(), transfer, force );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

}

quint32 MessageProperty::serialCache( KMime::Content *msgBase )
{
  QMap<KMime::Content*, long >::ConstIterator it = sSerialCache.constFind( msgBase );
  return it == sSerialCache.constEnd() ? 0 : *it;
}

void MessageProperty::setSerialCache( KMime::Content *msgBase, quint32 serNum )
{
  if (serNum)
    sSerialCache.insert( msgBase, serNum );
  else
    sSerialCache.remove( msgBase );
}

void MessageProperty::setKeepSerialNumber( quint32 serialNumber, bool keepForMoving )
{
  if ( serialNumber ) {
    if ( sKeepSerialNumber.contains( serialNumber ) )
      sKeepSerialNumber[ serialNumber ] = keepForMoving;
    else
      sKeepSerialNumber.insert( serialNumber, keepForMoving );
  }
}

bool MessageProperty::keepSerialNumber( quint32 serialNumber )
{
  if ( sKeepSerialNumber.contains( serialNumber ) )
    return sKeepSerialNumber[ serialNumber ];
  else
    return false;
}
 
void MessageProperty::forget( KMime::Content *msgBase )
{
  quint32 serNum = serialCache( msgBase );
  if (serNum) {
    Q_ASSERT( !transferInProgress( serNum ) );
    sTransfers.remove( serNum );
    sSerialCache.remove( msgBase );
    sKeepSerialNumber.remove( serNum );
  }
}

#include "messageproperty.moc"
