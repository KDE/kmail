/* This file is part of the KDE project
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef VCARDVIEWER_H
#define VCARDVIEWER_H

#include <kdialogbase.h>
#include <kabc/addressee.h>

#include <qvaluelist.h>

class QString;

namespace KPIM {
  class AddresseeView;
}

namespace KMail {

  class VCardViewer : public KDialogBase
  {
     Q_OBJECT
     public:
       VCardViewer(QWidget *parent, const QString& vCard, const char* name);
       virtual ~VCardViewer();

     protected:
       virtual void slotUser1();
       virtual void slotUser2();
       virtual void slotUser3();

     private:
       KPIM::AddresseeView *  mAddresseeView;
       KABC::Addressee::List  mAddresseeList;

       QValueListIterator<KABC::Addressee> itAddresseeList;
  };

}

#endif // VCARDVIEWER_H

