/*  -*- c++ -*-
    vacationdialog.h

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef __KMAIL_VACATIONDIALOG_H__
#define __KMAIL_VACATIONDIALOG_H__

#include "kdialogbase.h"

class QString;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class KDateWidget;
class KIntSpinBox;
template <typename T> class QValueList;

namespace KMime {
  namespace Types {
    struct AddrSpec;
    typedef QValueList<AddrSpec> AddrSpecList;
  }
}

namespace KMail {

  class VacationDialog : public KDialogBase {
    Q_OBJECT
  public:
    VacationDialog( const QString & caption, QWidget * parent=0,
		    const char * name=0, bool modal=true );
    virtual ~VacationDialog();

    virtual void enableDomainAndSendForSpam( bool enable = true );

    bool activateVacation() const;
    virtual void setActivateVacation( bool activate );

    QString messageText() const;
    virtual void setMessageText( const QString & text );

    int notificationInterval() const;
    virtual void setNotificationInterval( int days );

    KMime::Types::AddrSpecList mailAliases() const;
    virtual void setMailAliases( const KMime::Types::AddrSpecList & aliases );
    virtual void setMailAliases( const QString & aliases );
 
    QString domainName() const;
    virtual void setDomainName( const QString & domain );

    bool sendForSpam() const;
    virtual void setSendForSpam( bool enable );

    
  private slots:
    void slotIntervalSpinChanged( int value );

  protected:
    QCheckBox   * mActiveCheck;
    KIntSpinBox * mIntervalSpin;
    QLineEdit   * mMailAliasesEdit;
    QTextEdit   * mTextEdit;
    QCheckBox   * mSpamCheck;
    QCheckBox   * mDomainCheck;
    QLineEdit   * mDomainEdit;

  };

} // namespace KMail

#endif // __KMAIL_VACATIONDIALOG_H__
