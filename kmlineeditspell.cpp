/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "kmlineeditspell.h"

#include "recentaddresses.h"
#include "kmkernel.h"
#include "globalsettings.h"
#include "kmmessage.h"

#include <libkdepim/kvcarddrag.h>
#include <kpimutils/email.h>

#include <kabc/vcardconverter.h>
#include <kio/netaccess.h>

#include <kmenu.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kcompletionbox.h>
#include <klocale.h>

#include <QEvent>
#include <QFile>
#include <QCursor>
#include <QKeyEvent>
#include <QDropEvent>


KMLineEdit::KMLineEdit(bool useCompletion,
                       QWidget *parent, const char *name)
    : KPIM::AddresseeLineEdit(parent,useCompletion)
{
  setObjectName( name );
}


//-----------------------------------------------------------------------------
void KMLineEdit::keyPressEvent(QKeyEvent *e)
{
  if ((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) &&
      !completionBox()->isVisible())
  {
    emit focusDown();
    AddresseeLineEdit::keyPressEvent(e);
    return;
  }
  if (e->key() == Qt::Key_Up)
  {
    emit focusUp();
    return;
  }
  if (e->key() == Qt::Key_Down)
  {
    emit focusDown();
    return;
  }
  AddresseeLineEdit::keyPressEvent(e);
}


void KMLineEdit::insertEmails( const QStringList & emails )
{
  if ( emails.empty() )
    return;

  QString contents = text();
  if ( !contents.isEmpty() )
    contents += ',';
  // only one address, don't need kpopup to choose
  if ( emails.size() == 1 ) {
    setText( contents + emails.front() );
    return;
  }
  //multiple emails, let the user choose one
  KMenu menu( this );
  menu.setObjectName( "Addresschooser" );
  for ( QStringList::const_iterator it = emails.begin(), end = emails.end() ; it != end; ++it )
    menu.addAction( *it );
  const QAction *result = menu.exec( QCursor::pos() );
  if ( !result )
    return;
  setText( contents + result->text() );
}

void KMLineEdit::dropEvent(QDropEvent *event)
{
  const QMimeData *md = event->mimeData();

  // Case one: The user dropped a text/directory (i.e. vcard), so decode its
  //           contents
  if ( KPIM::KVCardDrag::canDecode( md ) ) {
    KABC::Addressee::List list;
    KPIM::KVCardDrag::fromMimeData( md, list );

    KABC::Addressee::List::Iterator ait;
    for ( ait = list.begin(); ait != list.end(); ++ait ){
      insertEmails( (*ait).emails() );
    }
  }

  // Case two: The user dropped a list or Urls.
  // Iterate over that list. For mailto: Urls, just add the addressee to the list,
  // and for other Urls, download the Url and assume it points to a vCard
  else if ( KUrl::List::canDecode( md ) ) {
    KUrl::List urls = KUrl::List::fromMimeData( md );
    KABC::Addressee::List list;

    foreach ( KUrl url, urls ) {

      // First, let's deal with mailto Urls. The path() part contains the
      // email-address.
      if ( url.protocol() == "mailto" ) {
        KABC::Addressee addressee;
        addressee.insertEmail( KMMessage::decodeMailtoUrl( url.path() ), true /* preferred */ );
        list += addressee;
      }

      // Otherwise, download the vCard to which the Url points
      else {
        KABC::VCardConverter converter;
        QString fileName;
        if ( KIO::NetAccess::download( url, fileName, parentWidget() ) ) {
          QFile file( fileName );
          file.open( QIODevice::ReadOnly );
          const QByteArray data = file.readAll();
          file.close();
          list += converter.parseVCards( data );
          KIO::NetAccess::removeTempFile( fileName );
        } else {
          QString caption( i18n( "vCard Import Failed" ) );
          QString text = i18n( "<qt>Unable to access <b>%1</b>.</qt>", url.url() );
          KMessageBox::error( parentWidget(), text, caption );
        }
      }
      // Now, let the user choose which addressee to add.
      KABC::Addressee::List::Iterator ait;
      foreach( const KABC::Addressee& addressee, list )
        insertEmails( addressee.emails() );
    }
  }

  // Case three: Let AddresseeLineEdit deal with the rest
  else {
    KPIM::AddresseeLineEdit::dropEvent( event );
  }
}

void KMLineEdit::contextMenuEvent( QContextMenuEvent*e )
{
   QMenu *popup = createStandardContextMenu();
   popup->addSeparator();
   QAction* act = popup->addAction( i18n( "Edit Recent Addresses..." ));
   connect(act,SIGNAL(triggered(bool)), SLOT( editRecentAddresses() ) );
   popup->exec( e->globalPos() );
   delete popup;
}

void KMLineEdit::editRecentAddresses()
{
  KPIM::RecentAddressDialog dlg( this );
  dlg.setAddresses( KPIM::RecentAddresses::self( KMKernel::config() )->addresses() );
  if ( !dlg.exec() )
    return;
  KPIM::RecentAddresses::self( KMKernel::config() )->clear();
  const QStringList addrList = dlg.addresses();
  for ( QStringList::const_iterator it = addrList.begin(), end = addrList.end() ; it != end ; ++it )
    KPIM::RecentAddresses::self( KMKernel::config() )->add( *it );
  loadContacts();
}


//-----------------------------------------------------------------------------
void KMLineEdit::loadContacts()
{
  AddresseeLineEdit::loadContacts();

  if ( GlobalSettings::self()->showRecentAddressesInComposer() ){
    if ( KMKernel::self() ) {
      QStringList recent =
        KPIM::RecentAddresses::self( KMKernel::config() )->addresses();
      QStringList::Iterator it = recent.begin();
      QString name, email;

      KConfig config( "kpimcompletionorder" );
      KConfigGroup group( &config, "CompletionWeights" );
      int weight = group.readEntry( "Recent Addresses", 10 );
      int idx = addCompletionSource( i18n( "Recent Addresses" ), weight );
      for ( ; it != recent.end(); ++it ) {
        KABC::Addressee addr;
        KPIMUtils::extractEmailAddressAndName(*it, email, name);
        addr.setNameFromString( KPIMUtils::quoteNameIfNecessary( name ));
        addr.insertEmail( email, true );
        addContact( addr, weight, idx );
      }
    }
  }
}


KMLineEditSpell::KMLineEditSpell(bool useCompletion,
                                 QWidget *parent, const char *name)
  : KMLineEdit(useCompletion,parent,name)
{
  allowSemiColonAsSeparator( GlobalSettings::allowSemicolonAsAddressSeparator() );
}


void KMLineEditSpell::highLightWord( unsigned int length, unsigned int pos )
{
    setSelection ( pos, length );
}

void KMLineEditSpell::spellCheckDone( const QString &s )
{
    if( s != text() )
        setText( s );
}

void KMLineEditSpell::spellCheckerMisspelling( const QString &_text, const QStringList&, unsigned int pos)
{
     highLightWord( _text.length(),pos );
}

void KMLineEditSpell::spellCheckerCorrected( const QString &old, const QString &corr, unsigned int pos)
{
    if( old!= corr )
    {
        setSelection ( pos, old.length() );
        insert( corr );
        setSelection ( pos, corr.length() );
        emit subjectTextSpellChecked();
    }
}


#include "kmlineeditspell.moc"
