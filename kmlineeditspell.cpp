// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include "kmlineeditspell.h"

#include "recentaddresses.h"
#include "kmkernel.h"
#include "globalsettings.h"
#include "stringutil.h"

#include <libkdepim/kvcarddrag.h>
#include <libemailfunctions/email.h>

#include <kabc/vcardconverter.h>
#include <kio/netaccess.h>

#include <kpopupmenu.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kmessagebox.h>
#include <kcompletionbox.h>
#include <klocale.h>

#include <qevent.h>
#include <qfile.h>
#include <qcstring.h>
#include <qcursor.h>


KMLineEdit::KMLineEdit(bool useCompletion,
                       QWidget *parent, const char *name)
    : KPIM::AddresseeLineEdit(parent,useCompletion,name)
{
   allowSemiColonAsSeparator( GlobalSettings::allowSemicolonAsAddressSeparator() );
}


//-----------------------------------------------------------------------------
void KMLineEdit::keyPressEvent(QKeyEvent *e)
{
  if ((e->key() == Key_Enter || e->key() == Key_Return) &&
      !completionBox()->isVisible())
  {
    emit focusDown();
    AddresseeLineEdit::keyPressEvent(e);
    return;
  }
  if (e->key() == Key_Up)
  {
    emit focusUp();
    return;
  }
  if (e->key() == Key_Down)
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
  KPopupMenu menu( this, "Addresschooser" );
  for ( QStringList::const_iterator it = emails.begin(), end = emails.end() ; it != end; ++it )
    menu.insertItem( *it );
  const int result = menu.exec( QCursor::pos() );
  if ( result == -1 )
    return;
  setText( contents + menu.text( result ) );
}

void KMLineEdit::dropEvent( QDropEvent *event )
{
  KURL::List urls;

  // Case one: The user dropped a text/directory (i.e. vcard), so decode its
  //           contents
  if ( KVCardDrag::canDecode( event ) ) {
   KABC::Addressee::List list;
    KVCardDrag::decode( event, list );

    KABC::Addressee::List::Iterator ait;
    for ( ait = list.begin(); ait != list.end(); ++ait ){
      insertEmails( (*ait).emails() );
    }
  }

  // Case two: The user dropped a list or Urls.
  // Iterate over that list. For mailto: Urls, just add the addressee to the list,
  // and for other Urls, download the Url and assume it points to a vCard
  else if ( KURLDrag::decode( event, urls ) ) {
    KURL::List::Iterator it = urls.begin();
    KABC::Addressee::List list;
    for ( it = urls.begin(); it != urls.end(); ++it ) {

      // First, let's deal with mailto Urls. The path() part contains the
      // email-address.
      if ( (*it).protocol() == "mailto" ) {
        KABC::Addressee addressee;
        addressee.insertEmail( KMail::StringUtil::decodeMailtoUrl( (*it).path() ), true /* preferred */ );
        list += addressee;
      }
      // Otherwise, download the vCard to which the Url points
      else {
        KABC::VCardConverter converter;
        QString fileName;
        if ( KIO::NetAccess::download( (*it), fileName, parentWidget() ) ) {
          QFile file( fileName );
          file.open( IO_ReadOnly );
          const QByteArray data = file.readAll();
          file.close();
#if defined(KABC_VCARD_ENCODING_FIX)
          list += converter.parseVCardsRaw( data.data() );
#else
          list += converter.parseVCards( data );
#endif
          KIO::NetAccess::removeTempFile( fileName );
        } else {
          QString caption( i18n( "vCard Import Failed" ) );
          QString text = i18n( "<qt>Unable to access <b>%1</b>.</qt>" ).arg( (*it).url() );
          KMessageBox::error( parentWidget(), text, caption );
        }
      }
      // Now, let the user choose which addressee to add.
      KABC::Addressee::List::Iterator ait;
      for ( ait = list.begin(); ait != list.end(); ++ait )
        insertEmails( (*ait).emails() );
    }
  }

  // Case three: Let AddresseeLineEdit deal with the rest
  else {
    KPIM::AddresseeLineEdit::dropEvent( event );
  }
}

QPopupMenu *KMLineEdit::createPopupMenu()
{
    QPopupMenu *menu = KPIM::AddresseeLineEdit::createPopupMenu();
    if ( !menu )
        return 0;

    menu->insertSeparator();
    menu->insertItem( i18n( "Edit Recent Addresses..." ),
                      this, SLOT( editRecentAddresses() ) );

    return menu;
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
  AddresseeLineEdit::loadContacts();

  if ( GlobalSettings::self()->showRecentAddressesInComposer() ){
    if ( KMKernel::self() ) {
      QStringList recent =
        KRecentAddress::RecentAddresses::self( KMKernel::config() )->addresses();
      QStringList::Iterator it = recent.begin();
      QString name, email;

      KConfig config( "kpimcompletionorder" );
      config.setGroup( "CompletionWeights" );
      int weight = config.readEntry( "Recent Addresses", "10" ).toInt();
      int idx = addCompletionSource( i18n( "Recent Addresses" ), weight );
      for ( ; it != recent.end(); ++it ) {
        KABC::Addressee addr;
        KPIM::getNameAndMail(*it, name, email);
        name = KPIM::quoteNameIfNecessary( name );
        if ( ( name[0] == '"' ) && ( name[name.length() - 1] == '"' ) ) {
          name.remove( 0, 1 );
          name.truncate( name.length() - 1 );
        }
        addr.setNameFromString( name );
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
