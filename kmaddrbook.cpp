// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include <config.h>
#include <assert.h>

#include <qregexp.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kprocess.h>

#include <dcopref.h>

#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>
#include <kabc/vcardconverter.h>

#include "kmaddrbook.h"
#include "kcursorsaver.h"
#include "kmmessage.h"
#include "kmkernel.h" // for KabcBridge

void KabcBridge::addresses(QStringList& result) // includes lists
{
  KCursorSaver busy(KBusyPtr::busy()); // loading might take a while

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::AddressBook::Iterator it;
  for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
    QStringList emails = (*it).emails();
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

  QStringList names = manager.listNames();
  QStringList::Iterator jt;
  for ( jt = names.begin(); jt != names.end(); ++jt)
    result.append( *jt );
  result.sort();
}

QStringList KabcBridge::addresses()
{
    QStringList entries;
    KABC::AddressBook::ConstIterator it;

    KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
    for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
        entries += (*it).fullEmail();
    }
    return entries;
}

//-----------------------------------------------------------------------------
QString KabcBridge::expandNickName( const QString& nickName )
{
  if ( nickName.isEmpty() )
    return QString();

  QString lowerNickName = nickName.lower();
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  for( KABC::AddressBook::ConstIterator it = addressBook->begin();
       it != addressBook->end(); ++it ) {
    if ( (*it).nickName().lower() == lowerNickName )
      return (*it).fullEmail();
  }
  return QString();
}

//-----------------------------------------------------------------------------
QString KabcBridge::expandDistributionList( const QString& listName )
{
  if ( listName.isEmpty() )
    return QString();

  QString lowerListName = listName.lower();
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  QStringList listNames = manager.listNames();

  for ( QStringList::Iterator it = listNames.begin();
        it != listNames.end(); ++it) {
    if ( (*it).lower() == lowerListName ) {
      QStringList addressList = manager.list( *it )->emails();
      return addressList.join( ", " );
    }
  }
  return QString();
}

//-----------------------------------------------------------------------------
void KMAddrBookExternal::openEmail( const QString &addr, QWidget *) {
  if (useKAddressbook()) {
    if ( checkForAddressBook() ) {
      QString email = KMMessage::getEmailAddr(addr);
      KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
      KABC::Addressee::List addresseeList = addressBook->findByEmail(email);
      if (!addresseeList.isEmpty())
      {
        QString error;
        kapp->startServiceByDesktopName("kaddressbook", QString::null, &error);
        sleep(2);

        DCOPRef call("kaddressbook", "KAddressBookIface");
        int succ = call.send("showContactEditor(QString)", addresseeList.first().uid() );
      }
    }
    return;
  }

  // TODO: Start a simple add-to-addressbook-dialog, or just add the address
  // silently to kabc.
}

//-----------------------------------------------------------------------------
void KMAddrBookExternal::addEmail( const QString& addr, QWidget *parent) {
  QString email;
  QString name;

  KABC::Addressee::parseEmailAddress( addr, name, email );

  KABC::AddressBook *ab = KABC::StdAddressBook::self();

  // force a reload of the address book file so that changes that were made
  // by other programs are loaded
  ab->load();

  KABC::Addressee::List addressees = ab->findByEmail( email );

  if ( addressees.isEmpty() ) {
    KABC::Addressee a;
    a.setNameFromString( name );
    a.insertEmail( email, true );

    ab->insertAddressee(a);

    if ( !KABC::StdAddressBook::save() ) {
      KMessageBox::error( parent, i18n("Can't save to addressbook.") );
    } else {
      QString text = i18n("<qt>The email address <b>%1</b> was added to your "
                          "addressbook. You can add more information to this "
                          "entry by opening the addressbook.</qt>").arg( addr );
      KMessageBox::information( parent, text, QString::null, "addedtokabc" );
    }
  } else {
    QString text = i18n("<qt>The email address <b>%1</b> is already in your "
                        "addressbook.</qt>").arg( addr );
    KMessageBox::information( parent, text );
  }
}

void KMAddrBookExternal::launch(QWidget *) {
// It might be better to remove the useK* functions and to check the config
// file directly, as they aren't used anywhere else.
// It might also be better to write a string for identifying the addressbook
// instead of a number as it is done now.
  if ( checkForAddressBook() ) {
    kapp->startServiceByDesktopName("kaddressbook");
  }
}

bool KMAddrBookExternal::useKab()
{
  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 1);
  return (ab == 0);
}

bool KMAddrBookExternal::useKAddressbook()
{
  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 1);
  return (ab == 1);
}

bool KMAddrBookExternal::checkForAddressBook()
{
  if ( KStandardDirs::findExe( "kaddressbook" ).isEmpty() ) {
    KMessageBox::information( 0,
        i18n("No external address book application found. You might want to "
             "install KAddressBook from the kdepim module.") );
    return false;
  } else {
    return true;
  }
}

void KMAddrBookExternal::addNewAddressee( QWidget* )
{
  if (useKAddressbook()) {
    if ( checkForAddressBook() ) {
        kapp->startServiceByDesktopName("kaddressbook");
        sleep(2);
        DCOPRef call("kaddressbook", "KAddressBookIface");
        call.send("newContact()");
    }
    return;
  }
  // TODO: Write a simple add-to-addressbook-dialog, or just add the address
  // silently to kabc.
}

bool KMAddrBookExternal::addVCard( const KABC::Addressee& addressee, QWidget *parent )
{
  KABC::AddressBook *ab = KABC::StdAddressBook::self();
  bool inserted = false;

  KABC::Addressee::List addressees =
      ab->findByEmail( addressee.preferredEmail() );

  if ( addressees.isEmpty() ) {
    ab->insertAddressee( addressee );
    if ( !KABC::StdAddressBook::save() ) {
      KMessageBox::error( parent, i18n("Can't save to addressbook.") );
      inserted = false;
    } else {
      QString text = i18n("The VCard was added to your addressbook. "
                          "You can add more information to this "
                          "entry by opening the addressbook.");
      KMessageBox::information( parent, text, QString::null, "addedtokabc" );
      inserted = true;
    }
  } else {
    QString text = i18n("The VCard's primary email address is already in "
                        "your addressbook. However you may save the VCard "
                        "into a file and import it into the addressbook "
                        "manually.");
    KMessageBox::information( parent, text );
    inserted = true;
  }

  return inserted;
}
