/* -*- mode: C++; c-file-style: "gnu" -*-
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
#include <config.h>
#include <unistd.h>

#include "kmaddrbook.h"
#include "kcursorsaver.h"
#include "kmmessage.h"
#include "kmkernel.h" // for KabcBridge

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>
#include <kabc/vcardconverter.h>
#include <dcopref.h>

#include <qregexp.h>

void KabcBridge::addresses(QStringList& result) // includes lists
{
  KCursorSaver busy(KBusyPtr::busy()); // loading might take a while

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::AddressBook::ConstIterator it;
  for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
    const QStringList emails = (*it).emails();
    QString n = (*it).prefix() + " " +
		(*it).givenName() + " " +
		(*it).additionalName() + " " +
	        (*it).familyName() + " " +
		(*it).suffix();
    n = n.simplifyWhiteSpace();

    QRegExp needQuotes("[^ 0-9A-Za-z\\x0080-\\xFFFF]");
    QString endQuote = "\" ";
    QStringList::ConstIterator mit;
    QString addr, email;

    for ( mit = emails.begin(); mit != emails.end(); ++mit ) {
      email = *mit;
      if (!email.isEmpty()) {
	if (n.isEmpty() || (email.find( '<' ) != -1))
	  addr = QString::null;
	else { // do we really need quotes around this name ?
          if (n.find(needQuotes) != -1)
	    addr = '"' + n + endQuote;
	  else
	    addr = n + ' ';
	}

	if (!addr.isEmpty() && (email.find( '<' ) == -1)
	    && (email.find( '>' ) == -1)
	    && (email.find( ',' ) == -1))
	  addr += '<' + email + '>';
	else
	  addr += email;
	addr = addr.stripWhiteSpace();
	result.append( addr );
      }
    }
  }
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  result += manager.listNames();

  result.sort();
}

QStringList KabcBridge::addresses()
{
    QStringList entries;
    KABC::AddressBook::ConstIterator it;

    const KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
    for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
        entries += (*it).fullEmail();
    }
    return entries;
}

//-----------------------------------------------------------------------------
QString KabcBridge::expandNickName( const QString& nickName )
{
  if ( nickName.isEmpty() )
    return QString::null;

  const QString lowerNickName = nickName.lower();
  const KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  for( KABC::AddressBook::ConstIterator it = addressBook->begin();
       it != addressBook->end(); ++it ) {
    if ( (*it).nickName().lower() == lowerNickName )
      return (*it).fullEmail();
  }
  return QString::null;
}


//-----------------------------------------------------------------------------

QStringList KabcBridge::categories()
{
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::Addressee::List addresses = addressBook->allAddressees();
  QStringList allcategories, aux;

  for ( KABC::Addressee::List::Iterator it = addresses.begin();
        it != addresses.end(); ++it ) {
    aux = ( *it ).categories();
    for ( QStringList::ConstIterator itAux = aux.begin();
          itAux != aux.end(); ++itAux ) {
      // don't have duplicates in allcategories
      if ( allcategories.find( *itAux ) == allcategories.end() )
        allcategories += *itAux;
    }
  }
  return allcategories;
}
