// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include "kmaddrbook.h"
#include <kapp.h>
#include <kapp.h>
#include <kmsgbox.h>
#include <qfile.h>
#include <assert.h>
#include <klocale.h>

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
	KMsgBox::message(0,i18n("KMail Error"),
			     i18n("Storing the addressbook failed!\n"));
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
  KConfig* config = kapp->getConfig();

  config->setGroup("Addressbook");
  config->writeEntry("default", mDefaultFileName);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMAddrBook::readConfig(void)
{
  KConfig* config = kapp->getConfig();
  config->setGroup("Addressbook");

  mDefaultFileName = config->readEntry("default");
  if (mDefaultFileName.isEmpty())
    mDefaultFileName = kapp->localkdedir()+"/share/apps/kmail/addressbook";
}


//-----------------------------------------------------------------------------
int KMAddrBook::load(const char* aFileName)
{
  char line[256];
  const char* fname = (aFileName ? aFileName : (const char*)mDefaultFileName);
  QFile file(fname);
  int rc;

  //assert(fname != NULL);
  if(!fname)
    return IO_FatalError;

  if (!file.open(IO_ReadOnly)) return file.status();
  clear();

  while (!file.atEnd())
  {
    if (file.readLine(line,255) > 0)
    {
      if (line[strlen(line)-1] < ' ') line[strlen(line)-1] = '\0';
      if (line[0]!='#' && line[0]!='\0') inSort(line);
    }
  }
  rc = file.status();
  file.close();

  mModified = FALSE;
  return rc;
}


//-----------------------------------------------------------------------------
int KMAddrBook::store(const char* aFileName)
{
  const char* addr;
  const char* fname = (aFileName ? aFileName : (const char*)mDefaultFileName);
  QFile file(fname);


  //assert(fname != NULL);
  if(!fname)
    return IO_FatalError;

  if (!file.open(IO_ReadWrite|IO_Truncate)) return fileError(file.status());

  addr = "# kmail addressbook file\n";
  if (file.writeBlock(addr,strlen(addr)) < 0) return fileError(file.status());

  for (addr=first(); addr; addr=next())
  {
    if (file.writeBlock(addr,strlen(addr)) < 0) return fileError(file.status());
    file.writeBlock("\n",1);
  }
  file.close();

  mModified = FALSE;
  return IO_Ok;
}


//-----------------------------------------------------------------------------
int KMAddrBook::fileError(int status) const
{
  QString msg, str;

  switch(status)
  {
  case IO_ReadError:
    msg = i18n("Could not read file:\n%s");
    break;
  case IO_OpenError:
    msg = i18n("Could not open file:\n%s");
    break;
  default:
    msg = i18n("Error while writing file:\n%s");
  }

  str.sprintf(msg, mDefaultFileName.data());
  KMsgBox::message(NULL, i18n("File I/O Error"), str,
		   KMsgBox::STOP, i18n("OK"));

  return status;
}


//-----------------------------------------------------------------------------
int KMAddrBook::compareItems(Item aItem1, Item aItem2)
{
  return strcasecmp((const char*)aItem1, (const char*)aItem2);
}
