// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include "kmlineeditspell.h"

#include "recentaddresses.h"
#include "kmkernel.h"
#include "globalsettings.h"

#include <libkdepim/kvcarddrag.h>
#include <libemailfunctions/email.h>

#include <kabc/vcardconverter.h>
#include <kio/netaccess.h>

#include <kmenu.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kcompletionbox.h>
#include <klocale.h>

#include <qevent.h>
#include <qfile.h>
#include <q3cstring.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QDropEvent>


KMLineEdit::KMLineEdit(bool useCompletion,
                       QWidget *parent, const char *name)
    : KPIM::AddresseeLineEdit(parent,useCompletion,name)
{
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
  QString vcards;
  KVCardDrag::decode( event, vcards );
  if ( !vcards.isEmpty() ) {
    KABC::VCardConverter converter;
    KABC::Addressee::List list = converter.parseVCards( vcards.toAscii() );
    KABC::Addressee::List::Iterator ait;
    for ( ait = list.begin(); ait != list.end(); ++ait ){
      insertEmails( (*ait).emails() );
    }
  } else {
    KURL::List urls = KURL::List::fromMimeData( event->mimeData() );
    if ( !urls.isEmpty() ) {
      //kdDebug(5006) << "urlList" << endl;
      KURL::List::Iterator it = urls.begin();
      KABC::VCardConverter converter;
      KABC::Addressee::List list;
      QString fileName;
      QString caption( i18n( "vCard Import Failed" ) );
      for ( it = urls.begin(); it != urls.end(); ++it ) {
        if ( KIO::NetAccess::download( *it, fileName, parentWidget() ) ) {
          QFile file( fileName );
          file.open( QIODevice::ReadOnly );
          QByteArray rawData = file.readAll();
          file.close();
          QString data = QString::fromUtf8( rawData.data(), rawData.size() + 1 );
          list += converter.parseVCards( data.toAscii() );
          KIO::NetAccess::removeTempFile( fileName );
        } else {
          QString text = i18n( "<qt>Unable to access <b>%1</b>.</qt>" );
          KMessageBox::error( parentWidget(), text.arg( (*it).url() ), caption );
        }
        KABC::Addressee::List::Iterator ait;
        for ( ait = list.begin(); ait != list.end(); ++ait )
          insertEmails((*ait).emails());
      }
    } else {
      KPIM::AddresseeLineEdit::dropEvent( event );
    }
  }
}

void KMLineEdit::contextMenuEvent( QContextMenuEvent*e )
{
   QMenu *popup = KLineEdit::createStandardContextMenu();
   popup->insertSeparator();
   popup->insertItem( i18n( "Edit Recent Addresses..." ),this, SLOT( editRecentAddresses() ) );
   popup->exec( e->globalPos() );
   delete popup;
}

void KMLineEdit::editRecentAddresses()
{
  KRecentAddress::RecentAddressDialog dlg( this );
  dlg.setAddresses( KRecentAddress::RecentAddresses::self( KMKernel::config() )->addresses() );
  if ( !dlg.exec() )
    return;
  KRecentAddress::RecentAddresses::self( KMKernel::config() )->clear();
  const QStringList addrList = dlg.addresses();
  for ( QStringList::const_iterator it = addrList.begin(), end = addrList.end() ; it != end ; ++it )
    KRecentAddress::RecentAddresses::self( KMKernel::config() )->add( *it );
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
        KRecentAddress::RecentAddresses::self( KMKernel::config() )->addresses();
      QStringList::Iterator it = recent.begin();
      QString name, email;
      int idx = addCompletionSource( i18n( "Recent Addresses" ) );
      for ( ; it != recent.end(); ++it ) {
        KABC::Addressee addr;
        KPIM::getNameAndMail(*it, name, email);
        addr.setNameFromString( KPIM::quoteNameIfNecessary( name ));
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
