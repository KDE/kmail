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
  static QString expandNickName( const QString& nickName );
   /**
    	Returns all categories found in the addressbook.
	@return A list of the categories
   */
    static QStringList categories();
};


#endif /*KMAddrBook_h*/
