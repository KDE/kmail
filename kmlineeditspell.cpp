// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include "config.h"

#include "kmlineeditspell.h"

#include "recentaddresses.h"
#include "kmkernel.h"
#include "globalsettings.h"

#include <libkdepim/kvcarddrag.h>
#include <emailfunctions/email.h>

#include <kabc/vcardconverter.h>
#include <kio/netaccess.h>

#include <kmenu.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kcompletionbox.h>
#include <klocale.h>

#include <QEvent>
#include <QFile>
#include <q3cstring.h>
#include <QCursor>
//Added by qt3to4:
#include <QKeyEvent>
#include <QDropEvent>


KMLineEdit::KMLineEdit(bool useCompletion,
                       QWidget *parent, const char *name)
    : KPIM::AddresseeLineEdit(parent,useCompletion)
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
  const QMimeData *md = event->mimeData();
  if ( KVCardDrag::canDecode( md ) ) {
    KABC::Addressee::List list;
    KVCardDrag::fromMimeData( md, list );

    KABC::Addressee::List::Iterator ait;
    for ( ait = list.begin(); ait != list.end(); ++ait ){
      insertEmails( (*ait).emails() );
    }
  } else if ( KUrl::List::canDecode( md ) ) {
    KUrl::List urls = KUrl::List::fromMimeData( md );
    //kDebug(5006) << "urlList" << endl;
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
        EmailAddressTools::extractEmailAddressAndName(*it, email, name);
        addr.setNameFromString( EmailAddressTools::quoteNameIfNecessary( name ));
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
