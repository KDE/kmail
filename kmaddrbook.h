/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>
#include <qvaluelist.h>  // for KabBridge
#include <kabapi.h> // for KabBridge

class KabBridge {
public:
  static void addresses(QStringList* result, QValueList<KabKey> *keys=0);
  static QString fn(QString address);
  static QString email(QString address);
  static bool add(QString address, KabKey &kabkey);
  static bool remove(KabKey);
  static bool replace(QString address, KabKey);
};

class KabcBridge {
public:
  static void addresses(QStringList* result);
  static QString expandDistributionLists(QString recipients);
};

class KMAddrBookExternal {
public:
  static void addEmail(QString addr, QWidget *parent);
  static void launch(QWidget *parent);
  static bool useKAB();
  static bool useKABC();
};

#endif /*KMAddrBook_h*/
