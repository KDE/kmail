// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include <config.h>
#include "kmaddrbook.h"
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qfile.h>
#include <qtextstream.h>
#include <assert.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kmessagebox.h>

#include "kmkernel.h" // for KabBridge
#include "kmmessage.h" // for KabBridge
#include "kmaddrbookdlg.h" // for kmaddrbookexternal
#include <krun.h> // for kmaddrbookexternal
#include "addtoaddressbook.h"

//-----------------------------------------------------------------------------
KMAddrBook::KMAddrBook(): KMAddrBookInherited()
{
  mModified = FALSE;
}


//-----------------------------------------------------------------------------
KMAddrBook::~KMAddrBook()
{
  if (mModified)
  {
    if(store() == IO_FatalError)
    {
      KMessageBox::sorry(0, i18n("The addressbook could not be stored!\n"));
    }
  }
  writeConfig(FALSE);
}


//-----------------------------------------------------------------------------
void KMAddrBook::insert(const QString aAddress)
{
  if (find((const char *)aAddress.local8Bit())<0)
  {
    inSort((const char *)aAddress.local8Bit());
    mModified=TRUE;
  }
}


//-----------------------------------------------------------------------------
void KMAddrBook::remove(const QString aAddress)
{
  remove(aAddress);
  mModified=TRUE;
}


//-----------------------------------------------------------------------------
void KMAddrBook::clear(void)
{
  KMAddrBookInherited::clear();
  mModified=TRUE;
}


//-----------------------------------------------------------------------------
void KMAddrBook::writeConfig(bool aWithSync)
{
  KConfig* config = kapp->config();

  config->setGroup("Addressbook");
  config->writeEntry("default", mDefaultFileName);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMAddrBook::readConfig(void)
{
  KConfig* config = kapp->config();
  config->setGroup("Addressbook");

  mDefaultFileName = config->readEntry("default");
  if (mDefaultFileName.isEmpty())
    mDefaultFileName = locateLocal("appdata", "addressbook");
}


//-----------------------------------------------------------------------------
int KMAddrBook::load(const QString &aFileName)
{
  QString line;
  QString fname = aFileName.isNull() ? mDefaultFileName : aFileName;
  QFile file(fname);
  int rc;

  if(!fname)
    return IO_FatalError;

  if (!file.open(IO_ReadOnly)) return file.status();
  clear();

  QTextStream t( &file );
  t.setEncoding(QTextStream::Locale);

  while ( !t.eof() )
  {
    line = t.readLine();
      if (line[0]!='#' && !line.isNull()) inSort((const char *)line.local8Bit());
  }
  rc = file.status();
  file.close();

  mModified = FALSE;
  return rc;
}


//-----------------------------------------------------------------------------
int KMAddrBook::store(const QString &aFileName)
{
  QString addr;
  QString fname = aFileName.isNull() ? mDefaultFileName : aFileName;
  QFile file(fname);

  if(fname.isNull())
    return IO_FatalError;

  if (!file.open(IO_ReadWrite|IO_Truncate)) return fileError(file.status());
  file.resetStatus(); // Work around suspected QT pre 2.2 snapshot bug
  QTextStream ts( &file );
  ts.setEncoding(QTextStream::Locale);

  addr = "# kmail addressbook file\n";
  ts << addr;
  if (file.status() != IO_Ok) return fileError(file.status());

  for (addr=QString::fromLocal8Bit(first()); addr; addr=QString::fromLocal8Bit(next()))
  {
    ts << addr << "\n";
    if (file.status() != IO_Ok) return fileError(file.status());
  }
  file.close();

  mModified = FALSE;
  return IO_Ok;
}


//-----------------------------------------------------------------------------
int KMAddrBook::fileError(int status) const
{
  QString msg;

  switch(status)
  {
  case IO_ReadError:
    msg = i18n("Could not read file:\n%1");
    break;
  case IO_OpenError:
    msg = i18n("Could not open file:\n%1");
    break;
  default:
    msg = i18n("Error while writing file:\n%1");
  }

  QString str = msg.arg(mDefaultFileName);
  KMessageBox::sorry(0, str, i18n("File I/O Error"));

  return status;
}


//-----------------------------------------------------------------------------
int KMAddrBook::compareItems(Item aItem1, Item aItem2)
{
  return strcasecmp((const char*)aItem1, (const char*)aItem2);
}




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
	else
	  addr = "\"" + entry.fn + "\" ";
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
    kdDebug() << "Error occurred trying to update database: operation insert.0" << endl;
    return false;
  }
  if (kernel->KABaddrBook()->addressbook()->save("", true) !=
      AddressBook::NoError) {
    kdDebug() << "Error occurred trying to update database: opeation insert.1" << endl;
    return false;
  }
  return true;
}

bool KabBridge::remove(KabKey kabKey)
{
  if (kernel->KABaddrBook()->addressbook()->remove( kabKey ) !=
      AddressBook::NoError) {
    kdDebug() << "Error occurred trying to update database: operation remove.0" << endl;
    return false;
  }

  if (kernel->KABaddrBook()->addressbook()->save("", true) !=
      AddressBook::NoError) {
    kdDebug() << "Error occurred trying to update database: operation remove.1" << endl;
    return false;
  }
  return true;
}

bool KabBridge::replace(QString address, KabKey kabKey)
{
  AddressBook::Entry old;
  if (AddressBook::NoError !=
      kernel->KABaddrBook()->addressbook()->getEntry( kabKey, old )) {
    kdDebug() << "Error occurred trying to update database: operation replace.0" << endl;
    return false;
  }

  if (old.emails.count() < 1)
    old.emails.append( "" );
  old.emails[0] = email(address);
  old.fn = fn(address);

  if (kernel->KABaddrBook()->addressbook()->change( kabKey, old ) !=
      AddressBook::NoError) {
    kdDebug() << "Error occurred trying to update database: operation replace.1" << endl;
    return false;
  }

  if (kernel->KABaddrBook()->addressbook()->save("", true) !=
      AddressBook::NoError) {
    kdDebug() << "Error occurred trying to update database: operation replace.2" << endl;
    return false;
  }
  return true;
}


//-----------------------------------------------------------------------------
void KMAddrBookExternal::addEmail(QString addr, QWidget *parent) {
  KConfig *config = kapp->config();
  config->setGroup("General");
  int ab = config->readNumEntry("addressbook", 1);
  if (ab == 3) {
    KRun::runCommand( "abbrowser -a \"" + addr.replace(QRegExp("\""), "")
      + "\"" );
    return;
  }
  if (!KMAddrBookExternal::useKAB()) {
    if (addr.isEmpty()) return;
    kernel->addrBook()->insert(addr);
    //    statusMsg(i18n("Address added to addressbook."));
  }
  else {
    AddToKabDialog dialog(addr, kernel->KABaddrBook(), parent);
    dialog.exec();
  }
}

void KMAddrBookExternal::launch(QWidget *parent) {
  KConfig *config = kapp->config();
  config->setGroup("General");
  int ab = config->readNumEntry("addressbook", 1);
  switch (ab)
  {
  case -1:
  case 0:
  case 1:
    {
    KMAddrBookEditDlg dlg( kernel->addrBook(), parent );
    dlg.exec();
    break;
    }
  case 2:
    KRun::runCommand("kab");
    break;
  case 3:
    KRun::runCommand("abbrowser");
    break;
  default:
    kdDebug() << "Unknown address book type" << endl;
  }
}

bool KMAddrBookExternal::useKAB()
{
  KConfig *config = kapp->config();
  config->setGroup("General");
  int ab = config->readNumEntry("addressbook", 1);
  if (ab <= 0)
    return false;
  return true;
}

