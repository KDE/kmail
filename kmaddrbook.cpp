// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include <config.h>
#include <pwd.h>
#include "kmaddrbook.h"
#include <kapplication.h>
#include <kdebug.h>

#include <qfile.h>
#include <qregexp.h>
#include <assert.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>

#include "kmkernel.h" // for KabcBridge
#include "kbusyptr.h"
#include "kmmessage.h" // for KabcBridge
#include "kmaddrbookdlg.h" // for kmaddrbookexternal
#include <krun.h> // for kmaddrbookexternal
#include <kprocess.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>

void KabcBridge::addresses(QStringList* result) // includes lists
{
  kernel->kbp()->busy(); // loading might take a while

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
    QString empty = "";
    QStringList::ConstIterator mit;
    QString addr, email;

    for ( mit = emails.begin(); mit != emails.end(); ++mit ) {
      email = *mit;
      if (!email.isEmpty()) {
	if (n.isEmpty() || (email.find( '<' ) != -1))
	  addr = empty;
	else { /* do we really need quotes around this name ? */
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
	result->append( addr );
      }
    }
  }
  KABC::DistributionListManager manager( addressBook );
  manager.load();

  QStringList names = manager.listNames();
  QStringList::Iterator jt;
  for ( jt = names.begin(); jt != names.end(); ++jt)
    result->append( *jt );
  result->sort();

  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
QString KabcBridge::expandDistributionLists(QString recipients)
{
  if (recipients.isEmpty())
    return "";
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  QStringList recpList, names = manager.listNames();
  QStringList::Iterator it, jt;
  QString receiver, expRecipients;
  unsigned int begin = 0, count = 0, quoteDepth = 0;
  for (; begin + count < recipients.length(); ++count) {
    if (recipients[begin + count] == '"')
      ++quoteDepth;
    if ((recipients[begin + count] == ',') && (quoteDepth % 2 == 0)) {
      recpList.append( recipients.mid( begin, count ) );
      begin += count + 1;
      count = 0;
    }
  }
  recpList.append( recipients.mid( begin ));

  for ( it = recpList.begin(); it != recpList.end(); ++it ) {
    if (!expRecipients.isEmpty())
      expRecipients += ", ";
    receiver = (*it).stripWhiteSpace();
    for ( jt = names.begin(); jt != names.end(); ++jt) {
      if (receiver.lower() == (*jt).lower()) {
	QStringList el = manager.list( receiver )->emails();
	for ( QStringList::Iterator kt = el.begin(); kt != el.end(); ++kt ) {
	  if (!expRecipients.isEmpty())
	    expRecipients += ", ";
	  expRecipients += *kt;
	}
	break;
      }
    }
    if ( jt == names.end() )
    {
      if (receiver.find('@') == -1)
      {
        KConfigGroup general( KMKernel::config(), "General" );
        QString defaultdomain = general.readEntry( "Default domain", "" );
        if( !defaultdomain.isEmpty() )
        {
          receiver += "@" + defaultdomain;
        }
        else
        {
          char hostname[100];
          gethostname(hostname, 100);
          QString username = receiver;
          receiver += "@";
          receiver += QCString(hostname, 100);

          passwd *pw = getpwnam(username.local8Bit());
          if (pw)
          {
            QString fn = QString::fromLocal8Bit(pw->pw_gecos);
            int first_comma = fn.find(',');
            if (first_comma > 0) {
              fn = fn.left(first_comma);
            }
            if (fn.find(QRegExp("[^ 0-9A-Za-z\\x0080-\\xFFFF]")) != -1)
              receiver = "\"" + fn + "\" <" + receiver + ">";
            else
              receiver = fn + " <" + receiver + ">";
          }
        }
        expRecipients += receiver;
      }
      else
      {
        expRecipients += receiver;
      }
    }
  }
  return expRecipients;
}

//-----------------------------------------------------------------------------
void KMAddrBookExternal::openEmail( const QString &addr, QWidget *) {
  if (useKAddressbook()) {
    if ( checkForAddressBook() ) {
      KRun::runCommand( "kaddressbook -a " + KProcess::quote(addr) );
    }
    return;
  }

  // TODO: Start a simple add-to-addressbook-dialog, or just add the address
  // silently to kabc.
}

//-----------------------------------------------------------------------------
void KMAddrBookExternal::addEmail( const QString &addr, QWidget *parent) {
  QString email;
  QString name;

  KABC::Addressee::parseEmailAddress( addr, name, email );

  KABC::AddressBook *ab = KABC::StdAddressBook::self();

  KABC::Addressee::List addressees = ab->findByEmail( email );

  if ( addressees.isEmpty() ) {
    KABC::Addressee a;
    a.setNameFromString( name );
    a.insertEmail( email, true );

    ab->insertAddressee(a);

    if ( !KABC::StdAddressBook::save() ) {
      KMessageBox::error( parent, i18n("Can't save to addressbook.") );
    } else {
      QString text = i18n("The email address '%1' was added to your "
                          "addressbook. You can add more information to this "
                          "entry by opening the addressbook.").arg( addr );
      KMessageBox::information( parent, text, QString::null, "addedtokabc" );
    }
  } else {
    QString text = i18n("The email address '%1' is already in your "
                        "addressbook.").arg( addr );
    KMessageBox::information( parent, text );
  }
}

void KMAddrBookExternal::launch(QWidget *) {
// Reenable selection, when kab is ported to libkabc.
// It might be better to remove the useK* functions and to check the config
// file directly, as they aren't used anywhere else.
// It might also be better to write a string for identifying the addressbook
// instead of a number as it is done now.
  if ( checkForAddressBook() ) {
    KRun::runCommand("kaddressbook");
  }
#if 0
  if ( useKab() ) {
    KRun::runCommand("kab");
  } else if ( useKAddressbook() ) {
    KRun::runCommand("kaddressbook");
  } else {
    // TODO: some default action, e.g. a simple address book dialog.
  }
#endif
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

