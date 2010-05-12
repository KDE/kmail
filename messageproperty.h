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

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QPointer>
#include <QObject>
#include <QSet>

class KMFilter;

namespace KMime {
  class Content;
}

namespace KMail {

/* A place to store properties that some but not necessarily all messages
   have.

   These properties do not need to be stored persistantly on disk but
   rather only are required while KMail is running.

   Furthermore some properties, namely complete and
   serialCache should only exist during the lifetime of a particular
   KMMsgBase based instance.
 */
class MessageProperty : public QObject
{
  Q_OBJECT

public:
  /** If the message is being filtered  */
  static bool filtering( const Akonadi::Item &item );
  static void setFiltering( const Akonadi::Item &item, bool filtering );
  /** The folder this message is to be moved into once
      filtering is finished, or null if the message is not
      scheduled to be moved */
  static Akonadi::Collection filterFolder( const Akonadi::Item &item );
  static void setFilterFolder( const Akonadi::Item &item, const Akonadi::Collection &folder );

  /** Some properties, namely complete and
      serialCache must be forgotten when a message class instance is
      destructed or assigned a new value */
  static void forget( KMime::Content* );

private:

  // The folder a message is to be moved into once filtering is finished if any
  static QMap<Akonadi::Item::Id, Akonadi::Collection> sFolders;
};

}

#endif /*messageproperty_h*/
