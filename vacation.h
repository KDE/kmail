/*  -*- c++ -*-
    vacation.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>
    Copyright (c) 2005 Klarälvdalens Datakonsult AB

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KMAIL_VACATION_H__
#define __KMAIL_VACATION_H__

#include <qobject.h>

#include <kurl.h>

class QString;
class QStringList;
template <typename T> class QValueList;
namespace KMail {
  class SieveJob;
  class VacationDialog;
}
namespace KMime {
  namespace Types {
    struct AddrSpec;
    typedef QValueList<AddrSpec> AddrSpecList;
  }
}

namespace KMail {

  class Vacation : public QObject {
    Q_OBJECT
  public:
    Vacation( QObject * parent=0, const char * name=0 );
    virtual ~Vacation();

    bool isUsable() const { return !mUrl.isEmpty(); }

    static QString defaultMessageText();
    static int defaultNotificationInterval();
    static QStringList defaultMailAliases();
    static bool defaultSendForSpam();
    static QString defaultDomainName();

  protected:
    static QString composeScript( const QString & messageText,
				  int notificationInterval,
				  const KMime::Types::AddrSpecList & aliases,
                                  bool sendForSpam, const QString & excludeDomain );
    static bool parseScript( const QString & script, QString & messageText,
			     int & notificationInterval, QStringList & aliases,
                             bool & sendForSpam, QString & domainName );
    KURL findURL() const;
    void handlePutResult( KMail::SieveJob * job, bool success, bool );


  signals:
    void result( bool success );

  protected slots:
    void slotDialogDefaults();
    void slotGetResult( KMail::SieveJob * job, bool success,
			const QString & script, bool active );
    void slotDialogOk();
    void slotDialogCancel();
    void slotPutActiveResult( KMail::SieveJob *, bool );
    void slotPutInactiveResult( KMail::SieveJob *, bool );
  protected:
    // IO:
    KMail::SieveJob * mSieveJob;
    KURL mUrl;
    // GUI:
    KMail::VacationDialog * mDialog;
    bool mWasActive;
  };

} // namespace KMail

#endif // __KMAIL_VACATION_H__
