// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

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

#include "kmkernel.h" // for KabBridge
#include "kmmessage.h" // for KabBridge
#include "kmaddrbookdlg.h" // for kmaddrbookexternal
#include <krun.h> // for kmaddrbookexternal
#include <kprocess.h>
#include "addtoaddressbook.h"
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>


//-----------------------------------------------------------------------------
void KabBridge::addresses(QStringList* result, QValueList<KabKey> *keys)
{
  QString addr;
  QString email;
  KabKey key;
  AddressBook::Entry entry;
  if (keys)
    keys->clear();
  int num = kernel->KABaddrBook()->addressbook()->noOfEntries();

  for (int i = 0; i < num; ++i) {
    if (AddressBook::NoError !=
	kernel->KABaddrBook()->addressbook()->getKey( i, key ))
      continue;
    if (AddressBook::NoError !=
	kernel->KABaddrBook()->addressbook()->getEntry( key, entry ))
      continue;
    unsigned int emails_count;
    for( emails_count = 0; emails_count < entry.emails.count(); emails_count++ ) {
      if (!entry.emails[emails_count].isEmpty()) {
	if (entry.fn.isEmpty() || (entry.emails[0].find( "<" ) != -1))
	  addr = "";
	else { /* do we really need quotes around this name ? */
	  if (entry.fn.find(QRegExp("[^ 0-9A-Za-z\\x0080-\\xFFFF]")) != -1)
		addr = "\"" + entry.fn + "\" ";
	  else
		addr = entry.fn + " ";
	}
	email = entry.emails[emails_count];
	if (!addr.isEmpty() && (email.find( "<" ) == -1)
	    && (email.find( ">" ) == -1)
	    && (email.find( "," ) == -1))
	    addr += "<" + email + ">";
	else
	    addr += email;
	addr.stripWhiteSpace();
	result->append( addr );
	if (keys)
	  keys->append( key );
      }
    }
  }
}

QString KabBridge::fn(QString address)
{
  return KMMessage::stripEmailAddr( address );
}

QString KabBridge::email(QString address)
{
  int i = address.find( "<" );
  if (i < 0)
    return "";
  int j = address.find( ">", i );
  if (j < 0)
    return "";
  return address.mid( i + 1, j - i - 1 );
}

bool KabBridge::add(QString address, KabKey &kabkey)
{
  AddressBook::Entry entry;
  if (entry.emails.count() < 1)
    entry.emails.append( "" );
  entry.emails[0] = email(address);
  entry.fn = fn(address);

  if (kernel->KABaddrBook()->addressbook()->add( entry, kabkey, true ) !=
      AddressBook::NoError) {
    kdDebug(5006) << "Error occurred trying to update database: operation insert.0" << endl;
    return false;
  }
  if (kernel->KABaddrBook()->addressbook()->save("", true) !=
      AddressBook::NoError) {
    kdDebug(5006) << "Error occurred trying to update database: opeation insert.1" << endl;
    return false;
  }
  return true;
}

bool KabBridge::remove(KabKey kabKey)
{
  if (kernel->KABaddrBook()->addressbook()->remove( kabKey ) !=
      AddressBook::NoError) {
    kdDebug(5006) << "Error occurred trying to update database: operation remove.0" << endl;
    return false;
  }

  if (kernel->KABaddrBook()->addressbook()->save("", true) !=
      AddressBook::NoError) {
    kdDebug(5006) << "Error occurred trying to update database: operation remove.1" << endl;
    return false;
  }
  return true;
}

bool KabBridge::replace(QString address, KabKey kabKey)
{
  AddressBook::Entry old;
  if (AddressBook::NoError !=
      kernel->KABaddrBook()->addressbook()->getEntry( kabKey, old )) {
    kdDebug(5006) << "Error occurred trying to update database: operation replace.0" << endl;
    return false;
  }

  if (old.emails.count() < 1)
    old.emails.append( "" );
  old.emails[0] = email(address);
  old.fn = fn(address);

  if (kernel->KABaddrBook()->addressbook()->change( kabKey, old ) !=
      AddressBook::NoError) {
    kdDebug(5006) << "Error occurred trying to update database: operation replace.1" << endl;
    return false;
  }

  if (kernel->KABaddrBook()->addressbook()->save("", true) !=
      AddressBook::NoError) {
    kdDebug(5006) << "Error occurred trying to update database: operation replace.2" << endl;
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
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
      expRecipients += receiver;
  }
  return expRecipients;
}

//-----------------------------------------------------------------------------
void KMAddrBookExternal::addEmail(QString addr, QWidget *parent) {
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 3);
  if (ab == 3) {
    KRun::runCommand( "kaddressbook -a \"" + addr.replace(QRegExp("\""), "")
      + "\"" );
    return;
  }
  AddToKabDialog dialog(addr, kernel->KABaddrBook(), parent);
  dialog.exec();
}

void KMAddrBookExternal::launch(QWidget *) {
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 3);
  switch (ab)
  {
  case 0:
    KRun::runCommand("kab");
    break;
  default:
    KRun::runCommand("kaddressbook");
  }
}

bool KMAddrBookExternal::useKAB()
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 1);
  return (ab == 0);
}

bool KMAddrBookExternal::useKABC()
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  int ab = config->readNumEntry("addressbook", 1);
  return (ab == 1);
}


