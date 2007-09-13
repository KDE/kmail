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

#include "vcardviewer.h"
#include "kmaddrbook.h"
#include <kaddrbook.h>

#include <addresseeview.h>
using KPIM::AddresseeView;

#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>
using KABC::VCardConverter;
using KABC::Addressee;

#include <klocale.h>

#include <QString>

KMail::VCardViewer::VCardViewer(QWidget *parent, const QByteArray& vCard)
  : KDialog( parent )
{
  setCaption( i18n("VCard Viewer") );
  setButtons( User1|User2|User3|Close );
  setModal( false );
  setDefaultButton( Close );
  setButtonGuiItem( User1, KGuiItem(i18n("&Import")) );
  setButtonGuiItem( User2, KGuiItem(i18n("&Next Card")) );
  setButtonGuiItem( User3, KGuiItem(i18n("&Previous Card")) );
  mAddresseeView = new AddresseeView(this);
  mAddresseeView->enableLinks( 0 );
  mAddresseeView->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
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
    mAddresseeView->setPlainText(i18n("Failed to parse vCard."));
    enableButton(User1, false);
  }
  connect( this, SIGNAL( user1clicked() ), SLOT( slotUser1() ) );
  connect( this, SIGNAL( user2clicked() ), SLOT( slotUser2() ) );
  connect( this, SIGNAL( user3clicked() ), SLOT( slotUser3() ) );

  resize(300,400);
}

KMail::VCardViewer::~VCardViewer()
{
}

void KMail::VCardViewer::slotUser1()
{
  KPIM::KAddrBookExternal::addVCard( *itAddresseeList, this );
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
