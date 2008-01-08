/*  -*- c++ -*-
    vacation.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef __KMAIL_VACATION_H__
#define __KMAIL_VACATION_H__

#include <QObject>
//Added by qt3to4:
#include <QList>

#include <kurl.h>

class QString;
class QStringList;
template <typename T> class QList;
namespace KMail {
  class SieveJob;
  class VacationDialog;
}
namespace KMime {
  namespace Types {
    struct AddrSpec;
    typedef QList<AddrSpec> AddrSpecList;
  }
}

namespace KMail {

  class Vacation : public QObject {
    Q_OBJECT
  public:
    explicit Vacation( QObject * parent=0, bool checkonly = false, const char * name=0 );
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
    KUrl findURL() const;
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
    KUrl mUrl;
    // GUI:
    KMail::VacationDialog * mDialog;
    bool mWasActive;
    bool mCheckOnly;
  };

} // namespace KMail

#endif // __KMAIL_VACATION_H__
