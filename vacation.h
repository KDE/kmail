/*  -*- c++ -*-
    vacation.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
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

  protected:
    static QString composeScript( const QString & messageText,
				  int notificationInterval,
				  const KMime::Types::AddrSpecList & aliases);
    static bool parseScript( const QString & script, QString & messageText,
			     int & notificationInterval, QStringList & aliases );
    KURL findURL() const;

  signals:
    void result( bool success );

  protected slots:
    void slotDialogDefaults();
    void slotGetResult( KMail::SieveJob * job, bool success,
			const QString & script, bool active );
    void slotDialogOk();
    void slotDialogCancel();
    void slotPutResult( KMail::SieveJob * job, bool success,
			const QString &, bool );

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
