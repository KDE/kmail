/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>

#include <kdeversion.h>
#include <kabc/addressee.h>

class QWidget;

class KabcBridge {
public:
  static QStringList addresses();
  static void addresses(QStringList& result);
  static QString expandNickName( const QString& nickName );
   /**
    	Returns all categories found in the addressbook.
	@return A list of the categories
   */
    static QStringList categories();
};


#endif /*KMAddrBook_h*/
