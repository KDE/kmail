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

namespace KMime {
  class Content;
}

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
  static bool filtering( KMime::Content* );
  static void setFiltering( KMime::Content*, bool filtering );
  /** The folder this message is to be moved into once
      filtering is finished, or null if the message is not
      scheduled to be moved */
  static KMFolder* filterFolder( quint32 );
  static void setFilterFolder( quint32, KMFolder* folder );
  static KMFolder* filterFolder( KMime::Content* );
  static void setFilterFolder( KMime::Content*, KMFolder* folder );
  /* Set the filterHandler for a message */
  static ActionScheduler* filterHandler( quint32 );
  static void setFilterHandler( quint32, ActionScheduler* filterHandler );
  static ActionScheduler* filterHandler( KMime::Content* );
  static void setFilterHandler( KMime::Content*, ActionScheduler* filterHandler );

  /* Caches the serial number for a message, or more correctly for a
     KMMsgBase based instance representing a message.
     This property becomes invalid when the message is destructed or
     assigned a new value */
  static void setSerialCache( KMime::Content*, quint32 );
  static quint32 serialCache( KMime::Content* );

  /**
   * Set the transferInProgress for a message.
   *
   * The number of calls to setTransferInProgress() with transfer = true is counted,
   * e.g. if you call setTransferInProgress() with transfer = true two times, and
   * then setTransferInProgress() with transfer = false once, transferInProgres()
   * will return true. transferInProgress() only returns false after an additional
   * call to setTransferInProgress() with transfer = false.
   *
   * This property becomes invalid when the message is destructed or
   * assigned a new value.
   */
  static void setTransferInProgress( KMime::Content*, bool, bool = false );
  static bool transferInProgress( KMime::Content* );
  static void setTransferInProgress( quint32, bool, bool = false );
  static bool transferInProgress( quint32 );

  /**
   * Set this property to true if you want to keep the serial number when moving
   * a message from a local folder to an online IMAP folder.
   * Setting this to true will cause the ImapJob to save the meta data, like the
   * serial number, of the message in a map, which is later read when the
   * message arrives in the new location. Then the serial number is restored.
   */
  static void setKeepSerialNumber( quint32 serialNumber, bool keepForMoving );
  static bool keepSerialNumber( quint32 serialNumber );

  /** Some properties, namely complete, transferInProgress, and
      serialCache must be forgotten when a message class instance is
      destructed or assigned a new value */
  static void forget( KMime::Content* );

private:

  // The folder a message is to be moved into once filtering is finished if any
  static QMap<quint32, QPointer<KMFolder> > sFolders;

  // Whether the serial number of a message should be kept when moving it from
  // a local folder to an online IMAP folder. This is currently only used by
  // the action scheduler (in ActionScheduler::moveMessage()), to make the IMAP
  // job aware that it should try to preserve the serial number when moving, see
  // ImapJob::init().
  static QMap<quint32, bool> sKeepSerialNumber;

  // The action scheduler currently processing a message if any
  static QMap<quint32, QPointer<ActionScheduler> > sHandlers;

  // The transferInProgres state of a message if any.
  static QMap<quint32, int > sTransfers;

  // The cached serial number of a message if any.
  static QMap<KMime::Content*, long> sSerialCache;
};

}

#endif /*messageproperty_h*/
