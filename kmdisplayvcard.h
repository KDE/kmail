/**
 * kmdisplayvcard.h
 *
 * Copyright (c) 2000 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef KMDISPLAYVCARD_H
#define KMDISPLAYVCARD_H

#include <kapp.h>
#include <kdialog.h>
#include <ktabctl.h>
#include <qwidget.h>
#include <qlistbox.h>
#include <qlabel.h>

#include "vcard.h"

// I hope this is self explanatory.  You create an object of this type
// and pass it the VCard to display.  Pretty simple? <grin>


class KMDisplayVCard : public KTabCtl {
Q_OBJECT

 public:
  KMDisplayVCard(VCard *vc, QWidget *parent = 0, const char *name = 0);
  virtual ~KMDisplayVCard();

  void setVCard(VCard *vc);

 public slots:
  void slotChangeAddress();

 private slots:
  void urlClicked(const QString &url);
  void mailUrlClicked(const QString &url);

 private:
  VCard *_vc;
  QListBox *mAddrList;
  QLabel *mAddr_PO, *mAddr_addr1, *mAddr_addr2, *mAddr_city, *mAddr_prov,
         *mAddr_country, *mAddr_postal;
  QValueList< QValueList<QString> > mAddresses;

  void BuildInterface();
  QWidget *getFirstTab();
  QWidget *getSecondTab();
  QWidget *getThirdTab();


};


#endif
