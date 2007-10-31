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
  if ( KPIM::KVCardDrag::canDecode( md ) ) {
    KABC::Addressee::List list;
    KPIM::KVCardDrag::fromMimeData( md, list );

    KABC::Addressee::List::Iterator ait;
    for ( ait = list.begin(); ait != list.end(); ++ait ){
      insertEmails( (*ait).emails() );
    }
  } else if ( KUrl::List::canDecode( md ) ) {
    KUrl::List urls = KUrl::List::fromMimeData( md );
    //kDebug(5006) <<"urlList";
    KUrl::List::Iterator it = urls.begin();
    KABC::VCardConverter converter;
    KABC::Addressee::List list;
    QString fileName;
    QString caption( i18n( "vCard Import Failed" ) );
    for ( it = urls.begin(); it != urls.end(); ++it ) {
      if ( KIO::NetAccess::download( *it, fileName, parentWidget() ) ) {
        QFile file( fileName );
        file.open( QIODevice::ReadOnly );
        const QByteArray data = file.readAll();
        file.close();
        list += converter.parseVCards( data );
        KIO::NetAccess::removeTempFile( fileName );
      } else {
        QString text = i18n( "<qt>Unable to access <b>%1</b>.</qt>", (*it).url() );
        KMessageBox::error( parentWidget(), text, caption );
      }
      KABC::Addressee::List::Iterator ait;
      for ( ait = list.begin(); ait != list.end(); ++ait )
        insertEmails((*ait).emails());
    }
  } else {
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
  // was: KABC::AddressLineEdit::loadAddresses()
  AddresseeLineEdit::loadContacts();

  if ( GlobalSettings::self()->showRecentAddressesInComposer() ){
    if ( KMKernel::self() ) {
      QStringList recent =
        KPIM::RecentAddresses::self( KMKernel::config() )->addresses();
      QStringList::Iterator it = recent.begin();
      QString name, email;
      int idx = addCompletionSource( i18n( "Recent Addresses" ) );
      for ( ; it != recent.end(); ++it ) {
        KABC::Addressee addr;
        KPIMUtils::extractEmailAddressAndName(*it, email, name);
        addr.setNameFromString( KPIMUtils::quoteNameIfNecessary( name ));
        addr.insertEmail( email, true );
        addContact( addr, 120, idx ); // more weight than kabc entries and more than ldap results
      }
    }
  }
}


KMLineEditSpell::KMLineEditSpell(bool useCompletion,
                       QWidget *parent, const char *name)
    : KMLineEdit(useCompletion,parent,name)
{
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
