/** -*- mode: C++ -*-
 * Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */


#include "accountcombobox.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "accountmanager.h"
#include <kdebug.h>
//Added by qt3to4:
#include <QList>

using namespace KMail;

AccountComboBox::AccountComboBox( QWidget* parent ) : QComboBox( parent )
{
  connect( kmkernel->acctMgr(), SIGNAL( accountAdded( KMAccount* ) ),
           this, SLOT( slotRefreshAccounts() ) );
  connect( kmkernel->acctMgr(), SIGNAL( accountRemoved( KMAccount* ) ),
           this, SLOT( slotRefreshAccounts() ) );
  slotRefreshAccounts();
}

void AccountComboBox::slotRefreshAccounts()
{
  KMAccount* curr = currentAccount();
  clear();
  // Note that this won't take into account newly-created-in-configuredialog accounts
  // until clicking OK or Apply. This would make this class much more complex
  // (this would have to be different depending on whether this combo is in the
  // configuration dialog or not...)
  QStringList accountNames;
  QList<KMAccount *> lst = applicableAccounts();
  QList<KMAccount *>::ConstIterator it = lst.begin();
  for ( ; it != lst.end() ; ++it )
    accountNames.append( (*it)->name() );
  kDebug(5006) << accountNames;
  addItems( accountNames );
  if ( curr )
    setCurrentAccount( curr );
}


void AccountComboBox::setCurrentAccount( KMAccount* account )
{
  int i = 0;
  QList<KMAccount *> lst = applicableAccounts();
  QList<KMAccount *>::ConstIterator it = lst.begin();
  for ( ; it != lst.end() ; ++it, ++i ) {
    if ( (*it) == account ) {
      setCurrentIndex( i );
      return;
    }
  }
}

KMAccount* AccountComboBox::currentAccount() const
{
  int i = 0;
  QList<KMAccount *> lst = applicableAccounts();
  QList<KMAccount *>::ConstIterator it = lst.begin();
  while ( it != lst.end() && i < currentIndex() ) {
    ++it;
    ++i;
  }
  if ( it != lst.end() )
    return *it;
  return 0;
}

QList<KMAccount *> KMail::AccountComboBox::applicableAccounts() const
{
  QList<KMAccount *> lst;
  for( KMAccount *a = kmkernel->acctMgr()->first(); a;
       a = kmkernel->acctMgr()->next() ) {
    if ( a && a->type() == KAccount::DImap ) { //// ## proko2 hack. Need a list of allowed account types as ctor param
      lst.append( a );
    }
  }
  return lst;
}

#include "accountcombobox.moc"
