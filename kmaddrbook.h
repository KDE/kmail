/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>
#include <qwidget.h>

#include <kdeversion.h>
#include <kabc/addressee.h>

class KabcBridge {
public:
  static QStringList addresses();
  static void addresses(QStringList& result);
  static QString expandNickName( const QString& nickName );
  static QString expandDistributionList( const QString& listName );
};

class KMAddrBookExternal {
public:
  static void addEmail( const QString &addr, QWidget *parent );
  static void addNewAddressee( QWidget* );
  static void openEmail( const QString &addr, QWidget *parent );
  static void launch( QWidget *parent );
  static bool useKab();
  static bool useKAddressbook();
  static bool checkForAddressBook();
  static bool addVCard( const KABC::Addressee& addressee, QWidget *parent );
};

#endif /*KMAddrBook_h*/
