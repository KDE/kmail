// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include "kmaddrbook.h"
#include <kapp.h>
#include <kconfig.h>
#include <qfile.h>
#include <qtextstream.h>
#include <assert.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kmessagebox.h>

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
  if (find(aAddress)<0)
  {
    inSort(aAddress);
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

  while ( !t.eof() )
  {
    line = t.readLine();
    debug( QString("load ") + line );
      if (line[0]!='#' && !line.isNull()) inSort(line);
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
  QTextStream ts( &file );

  addr = "# kmail addressbook file\n";
  ts << addr;
  if (file.status() != IO_Ok) return fileError(file.status());

  for (addr=first(); addr; addr=next())
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
