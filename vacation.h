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
class QDate;
namespace KMail {
  class SieveJob;
  class VacationDialog;
};

namespace KMail {

  class Vacation : public QObject {
    Q_OBJECT
  public:
    Vacation( QObject * parent=0, const char * name=0 );
    virtual ~Vacation();

    bool isUsable() const { return !mUrl.isEmpty(); }

    static QDate defaultReturnDate();
    static int defaultNotificationInterval();

  protected:
    static QString composeScript( const QDate & returnData,
				  int notificationInterval );
    static bool parseScript( const QString & script, QDate & returnDate,
			     int & notificationInterval );
    KURL findURL() const;

  signals:
    void result( bool success );

  protected slots:
    void slotGetResult( KMail::SieveJob * job, bool success,
			const QString & scipt, bool active );
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

}; // namespace KMail

#endif // __KMAIL_VACATION_H__
