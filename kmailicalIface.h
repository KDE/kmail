/*
    This file is part of KMail.
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

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
#ifndef KMAILICALIFACE_H
#define KMAILICALIFACE_H

#include <dcopobject.h>
#include <qstringlist.h>

class KMailICalIface : virtual public DCOPObject
{
  K_DCOP
k_dcop:
  virtual bool addIncidence( const QString& type, const QString& folder,
                             const QString& uid, const QString& ical ) = 0;
  virtual bool deleteIncidence( const QString& type, const QString& folder,
                                const QString& uid ) = 0;
  virtual QStringList incidences( const QString& type,
                                  const QString& folder ) = 0;
  virtual QStringList subresources( const QString& type ) = 0;
  virtual bool isWritableFolder( const QString& type,
                                 const QString& resource ) = 0;

  // This saves the iCals/vCards in the entries in the folder.
  // The format in the string list is uid, entry, uid, entry...
  virtual bool update( const QString& type, const QString& folder,
                       const QStringList& entries ) = 0;

  // Update a single entry in the storage layer
  virtual bool update( const QString& type, const QString& folder,
                       const QString& uid, const QString& entry ) = 0;

k_dcop_signals:
  void incidenceAdded( const QString& type, const QString& folder,
                       const QString& entry );
  void incidenceDeleted( const QString& type, const QString& folder,
                         const QString& uid );
  void signalRefresh( const QString& type, const QString& folder );
  void subresourceAdded( const QString& type, const QString& resource );
  void subresourceDeleted( const QString& type, const QString& resource );
  void asyncLoadResult( const QStringList& list, const QString& type,
                        const QString& folder );
};

#endif
