// kmaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include "kmaddrbook.h"
#include <kapp.h>
#include <qfile.h>
#include <assert.h>

//-----------------------------------------------------------------------------
KMAddrBook::KMAddrBook(): KMAddrBookInherited()
{
  mModified = FALSE;
}


//-----------------------------------------------------------------------------
KMAddrBook::~KMAddrBook()
{
  if (mModified) store();
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

  debug("addressbook: %s", mDefaultFileName.data());
}


//-----------------------------------------------------------------------------
int KMAddrBook::load(const char* aFileName)
{
  char line[256];
  const char* fname = (aFileName ? aFileName : (const char*)mDefaultFileName);
  QFile file(fname);

  assert(fname != NULL);

  if (!file.open(IO_ReadOnly)) return file.status();
  clear();

  while (!file.atEnd())
  {
    if (file.readLine(line,255) < 0) return file.status();
    if (line[strlen(line)-1] < ' ') line[strlen(line)-1] = '\0';
    if (line[0]!='#' && line[0]!='\0') inSort(line);
  }
  file.close();

  mModified = FALSE;
  return IO_Ok;
}


//-----------------------------------------------------------------------------
int KMAddrBook::store(const char* aFileName)
{
  const char* addr;
  const char* fname = (aFileName ? aFileName : (const char*)mDefaultFileName);
  QFile file(fname);

  assert(fname != NULL);

  if (!file.open(IO_ReadWrite|IO_Truncate)) return file.status();

  addr = "# kmail addressbook file\n";
  if (file.writeBlock(addr,strlen(addr)) < 0) return file.status();

  for (addr=first(); addr; addr=next())
  {
    if (file.writeBlock(addr,strlen(addr)) < 0) return file.status();
    file.writeBlock("\n",1);
  }
  file.close();

  mModified = FALSE;
  return IO_Ok;
}


//-----------------------------------------------------------------------------
int KMAddrBook::compareItems(GCI aItem1, GCI aItem2)
{
  return strcasecmp((const char*)aItem1, (const char*)aItem2);
}
