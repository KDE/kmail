/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>
#include <qvaluelist.h>  // for KabBridge
#include <kabapi.h> // for KabBridge


#define KMAddrBookInherited QStringList
class KMAddrBook: protected QStringList
{
public:
  KMAddrBook();
  virtual ~KMAddrBook();

  /** Insert given address to the addressbook. Sorted. Duplicate
    addresses are not inserted. */
  virtual void insert(const QString& address);

  /** Remove given address from the addressbook. */
  virtual void remove(const QString& address);

  /** Returns an iterator pointing to the first address or to @ref end() */
  QStringList::ConstIterator begin() const { 
    return KMAddrBookInherited::begin(); 
  }

  /** Returns an iterator pointing to the end of the list */
  QStringList::ConstIterator end() const { 
    return KMAddrBookInherited::end(); 
  }

  /** Clear addressbook (remove the contents). */
  virtual void clear(void);

  /** Open addressbook file and read contents. The default addressbook
    file is used if no filename is given.
    Returns IO_Ok on success and an IO device status on failure -- see
    QIODevice::status(). */
  virtual int load(const QString &fileName=QString::null);

  /** Store addressbook in file or in same file of last load() call
    if no filename is given. Returns IO_Ok on success and an IO device
    status on failure -- see QIODevice::status(). */
  virtual int store(const QString &fileName=QString::null);

  /** Read/write configuration options. Uses the group "Addressbook"
    in the app's config file. */
  virtual void readConfig(void);
  virtual void writeConfig(bool withSync=TRUE);

  /** Test if the addressbook has unsaved changes. */
  virtual bool modified(void) const { return mModified; }

protected:
  /** Displays a detailed message box and returns 'status' */
  virtual int fileError(int status) const;

  /** inserts @p entry alphabetically sorted into the addressbook 
      Does NOT check for duplicates! */
  void inSort(const QString& entry);

  QString mDefaultFileName;
  bool mModified;
};

class KabBridge {
public:
  static void addresses(QStringList* result, QValueList<KabKey> *keys=0);
  static QString fn(QString address);
  static QString email(QString address);
  static bool add(QString address, KabKey &kabkey);
  static bool remove(KabKey);
  static bool replace(QString address, KabKey);
};

class KMAddrBookExternal {
public:
  static void addEmail(QString addr, QWidget *parent);
  static void launch(QWidget *parent);
  static bool useKAB();
};

#endif /*KMAddrBook_h*/
