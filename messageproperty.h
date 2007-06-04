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
#ifndef messageproperty_h
#define messageproperty_h

#include "kmfilteraction.h" // for KMFilterAction::ReturnCode
#include "kmfolder.h"

#include <QPointer>
#include <QObject>

class KMFilter;

namespace KMail {

class ActionScheduler;

/* A place to store properties that some but not necessarily all messages
   have.

   These properties do not need to be stored persistantly on disk but
   rather only are required while KMail is running.

   Furthermore some properties, namely complete, transferInProgress, and
   serialCache should only exist during the lifetime of a particular
   KMMsgBase based instance.
 */
class MessageProperty : public QObject
{
  Q_OBJECT

public:
  /** If the message is being filtered  */
  static bool filtering( quint32 );
  static void setFiltering( quint32, bool filtering );
  static bool filtering( const KMMsgBase* );
  static void setFiltering( const KMMsgBase*, bool filtering );
  /** The folder this message is to be moved into once
      filtering is finished, or null if the message is not
      scheduled to be moved */
  static KMFolder* filterFolder( quint32 );
  static void setFilterFolder( quint32, KMFolder* folder );
  static KMFolder* filterFolder( const KMMsgBase* );
  static void setFilterFolder( const KMMsgBase*, KMFolder* folder );
  /* Set the filterHandler for a message */
  static ActionScheduler* filterHandler( quint32 );
  static void setFilterHandler( quint32, ActionScheduler* filterHandler );
  static ActionScheduler* filterHandler( const KMMsgBase* );
  static void setFilterHandler( const KMMsgBase*, ActionScheduler* filterHandler );

  /* Caches the serial number for a message, or more correctly for a
     KMMsgBase based instance representing a message.
     This property becomes invalid when the message is destructed or
     assigned a new value */
  static void setSerialCache( const KMMsgBase*, quint32 );
  static quint32 serialCache( const KMMsgBase* );

  /* Set the transferInProgress for a message
     This property becomes invalid when the message is destructed or
     assigned a new value */
  static void setTransferInProgress( const KMMsgBase*, bool, bool = false );
  static bool transferInProgress( const KMMsgBase* );
  static void setTransferInProgress( quint32, bool, bool = false );
  static bool transferInProgress( quint32 );
  /** Some properties, namely complete, transferInProgress, and
      serialCache must be forgotten when a message class instance is
      destructed or assigned a new value */
  static void forget( const KMMsgBase* );

private:
  // The folder a message is to be moved into once filtering is finished if any
  static QMap<quint32, QPointer<KMFolder> > sFolders;
  // The action scheduler currently processing a message if any
  static QMap<quint32, QPointer<ActionScheduler> > sHandlers;
  // The transferInProgres state of a message if any.
  static QMap<quint32, int > sTransfers;
  // The cached serial number of a message if any.
  static QMap<const KMMsgBase*, long > sSerialCache;
};

}

#endif /*messageproperty_h*/
