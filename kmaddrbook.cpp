// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include <pwd.h>
#include <config.h>
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
#include "kmmessage.h" // for KabcBridge
#include "kmaddrbookdlg.h" // for kmaddrbookexternal
#include <krun.h> // for kmaddrbookexternal
#include <kprocess.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>


void KabcBridge::addresses(QStringList* result) // includes lists
{
  QString addr, email;
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
    for( unsigned int i = 0; i < emails.count(); ++i ) {
      if (!emails[i].isEmpty()) {
	if (n.isEmpty() || (emails[i].find( "<" ) != -1))
	  addr = "";
	else { /* do we really need quotes around this name ? */
	  if (n.find(QRegExp("[^ 0-9A-Za-z\\x0080-\\xFFFF]")) != -1)
	    addr = "\"" + n + "\" ";
	  else
	    addr = n + " ";
	}
	email = emails[i];
	if (!addr.isEmpty() && (email.find( "<" ) == -1)
	    && (email.find( ">" ) == -1)
	    && (email.find( "," ) == -1))
	  addr += "<" + email + ">";
	else
	  addr += email;
	addr.stripWhiteSpace();
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
    for ( jt = names.begin(); jt != names.end(); ++jt)
      if (receiver.lower() == (*jt).lower()) {
	QStringList el = manager.list( receiver )->emails();
	for ( QStringList::Iterator kt = el.begin(); kt != el.end(); ++kt ) {
	  if (!expRecipients.isEmpty())
	    expRecipients += ", ";
	  expRecipients += *kt;
	}
	break;
      }
    if ( jt == names.end() )
    {
      if (receiver.find('@') == -1)
      {
        KConfigGroup general( kapp->config(), "General" );
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
  
          passwd *pw;
          setpwent();
          while ((pw = getpwent()))
          {
            if (qstrcmp(pw->pw_name, username.local8Bit()) == 0)
            {
              QString fn = QString::fromLocal8Bit(pw->pw_gecos);
              if (fn.find(QRegExp("[^ 0-9A-Za-z\\x0080-\\xFFFF]")) != -1)
                receiver = "\"" + fn + "\" <" + receiver + ">";
              else
                receiver = fn + " <" + receiver + ">";
            }
          }
          endpwent();
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
void KMAddrBookExternal::addEmail(QString addr, QWidget *) {
  if (useKAddressbook())
  {
    KRun::runCommand( "kaddressbook -a \"" + addr.replace(QRegExp("\""), "")
      + "\"" );
    return;
  }
  
  // TODO: Start a simple add-to-addressbook-dialog, or just add the address
  // silently to kabc.
}

void KMAddrBookExternal::launch(QWidget *) {
// Reenable selection, when kab is ported to libkabc.
// It might be better to remove the useK* functions and to check the config
// file directly, as they aren't used anywhere else.
// It might also be better to write a string for identifying the addressbook
// instead of a number as it is done now.
    KRun::runCommand("kaddressbook");  
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
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 1);
  return (ab == 0);
}

bool KMAddrBookExternal::useKAddressbook()
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 1);
  return (ab == 1);
}


