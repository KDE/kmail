/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KMAddrBook_h
#define KMAddrBook_h

#include <qstringlist.h>

#include <kdeversion.h>
#include <kabc/addressee.h>

class QWidget;

class KabcBridge {
public:
  static QStringList addresses();
  static void addresses(QStringList& result);
  static QString expandNickName( const QString& nickName );
  static QString expandDistributionList( const QString& listName );
   /**
    	Returns all categories found in the addressbook.
	@return A list of the categories
   */
    static QStringList categories();
};

class KMAddrBookExternal {
public:
  static void addEmail( const QString &addr, QWidget *parent );
  static void addNewAddressee( QWidget* );
  static void openEmail( const QString &addr, QWidget *parent );
  static void openAddressBook( QWidget *parent );

  static bool addVCard( const KABC::Addressee& addressee, QWidget *parent );

private:
  static bool addAddressee( const KABC::Addressee& addressee );
};

#endif /*KMAddrBook_h*/
