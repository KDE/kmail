/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>
#include <qwidget.h>

class KabcBridge {
public:
  static void addresses(QStringList* result);
  static QString expandDistributionLists(QString recipients);
};

class KMAddrBookExternal {
public:
  static void addEmail(QString addr, QWidget *parent);
  static void launch(QWidget *parent);
  static bool useKab();
  static bool useKAddressbook();
};

#endif /*KMAddrBook_h*/
