/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>
#include <qwidget.h>

#include <kdeversion.h>

class KabcBridge {
public:
  static void addresses(QStringList* result);
  static QString expandDistributionLists(QString recipients);
};

class KMAddrBookExternal {
public:
  static void addEmail( const QString &addr, QWidget *parent );
  static void openEmail( const QString &addr, QWidget *parent );
  static void launch(QWidget *parent);
  static bool useKab();
  static bool useKAddressbook();
  static bool checkForAddressBook();

#if KDE_VERSION < 305
private:
  // FIXME: In version 3.1 this function is in kdelibs. Remove it from here
  // when KMail doesn't need to be compilable with 3.0 libs anymore.
  static void parseEmailAddress( const QString &rawEmail, QString &fullName,
                                 QString &email );
#endif
};

#endif /*KMAddrBook_h*/
