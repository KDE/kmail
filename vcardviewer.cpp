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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vcardviewer.h"
#include "kmaddrbook.h"

#include <addresseeview.h>
using KPIM::AddresseeView;

#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>
using KABC::VCardConverter;
using KABC::Addressee;

#include <klocale.h>

#include <qstring.h>

KMail::VCardViewer::VCardViewer(QWidget *parent, const QString& vCard, const char* name)
  : KDialogBase( parent, name, false, i18n("VCard Viewer"), User1|User2|User3|Close, Close,
		 true, i18n("&Import"), i18n("&Next Card"), i18n("&Previous Card") )
{
  mAddresseeView = new AddresseeView(this);
  mAddresseeView->setVScrollBarMode(QScrollView::Auto);
  setMainWidget(mAddresseeView);

  VCardConverter vcc;
  mAddresseeList = vcc.parseVCards( vCard );
  if ( !mAddresseeList.empty() ) {
    itAddresseeList = mAddresseeList.begin();
    mAddresseeView->setAddressee( *itAddresseeList );
    if ( mAddresseeList.size() <= 1 ) {
      showButton(User2, false);
      showButton(User3, false);
    }
    else
      enableButton(User3, false);
  }
  else {
    mAddresseeView->setText(i18n("Failed to parse vCard."));
    enableButton(User1, false);
  }

  resize(300,400);
}

KMail::VCardViewer::~VCardViewer()
{
}

void KMail::VCardViewer::slotUser1()
{
  KMAddrBookExternal::addVCard( *itAddresseeList, this );
}

void KMail::VCardViewer::slotUser2()
{
  // next vcard
  mAddresseeView->setAddressee( *(++itAddresseeList) );
  if ( itAddresseeList == --(mAddresseeList.end()) )
    enableButton(User2, false);
  enableButton(User3, true);
}

void KMail::VCardViewer::slotUser3()
{
  // previous vcard
  mAddresseeView->setAddressee( *(--itAddresseeList) );
  if ( itAddresseeList == mAddresseeList.begin() )
    enableButton(User3, false);
  enableButton(User2, true);
}

#include "vcardviewer.moc"
