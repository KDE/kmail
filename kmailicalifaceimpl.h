/*
    This file is part of KMail.
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>

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
*/

#ifndef KMAILICALIFACEIMPL_H
#define KMAILICALIFACEIMPL_H

#include "kmailicalIface.h"

class KMGroupware;

class KMailICalIfaceImpl : public QObject, virtual public KMailICalIface {
  Q_OBJECT
public:
  KMailICalIfaceImpl( KMGroupware* gw );
  
  virtual bool addIncidence( const QString& folder, const QString& uid, 
			     const QString& ical );
  virtual bool deleteIncidence( const QString& folder, const QString& uid );
  virtual QStringList incidences( const QString& folder );
public slots:
  void slotIncidenceAdded( const QString& folder, const QString& ical );
  void slotIncidenceDeleted( const QString& folder, const QString& uid );
  void slotRefresh( const QString& type);

private:
  KMGroupware* mGroupware;
};

#endif // KMAILICALIFACEIMPL_H
