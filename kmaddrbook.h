/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstrlist.h>

#define KMAddrBookInherited QStrList
class KMAddrBook: protected QStrList
{
public:
  KMAddrBook();
  virtual ~KMAddrBook();

  /** Insert given address to the addressbook. Sorted. Duplicate
    addresses are not inserted. */
  virtual void insert(const QString address);

  /** Remove given address from the addressbook. */
  virtual void remove(const QString address);

  /** Returns first address in addressbook or NULL if addressbook is empty. */
  virtual const char* first(void) { return KMAddrBookInherited::first(); }

  /** Returns next address in addressbook or NULL. */
  virtual const char* next(void) { return KMAddrBookInherited::next(); }

  /** Clear addressbook (remove the contents). */
  virtual void clear(void);

  /** Open addressbook file and read contents. The default addressbook
    file is used if no filename is given.
    Returns IO_Ok on success and an IO device status on failure -- see
    QIODevice::status(). */
  virtual int load(const char* fileName=NULL);

  /** Store addressbook in file or in same file of last load() call
    if no filename is given. Returns IO_Ok on success and an IO device
    status on failure -- see QIODevice::status(). */
  virtual int store(const char* fileName=NULL);

  /** Read/write configuration options. Uses the group "Addressbook"
    in the app's config file. */
  virtual void readConfig(void);
  virtual void writeConfig(bool withSync=TRUE);

  /** Test if the addressbook has unsaved changes. */
  virtual bool modified(void) const { return mModified; }

protected:
  virtual int compareItems(GCI item1, GCI item2);

  /** Displays a detailed message box and returns 'status' */
  virtual int KMAddrBook::fileError(int status) const;

  QString mDefaultFileName;
  bool mModified;
};

#endif /*KMAddrBook_h*/
