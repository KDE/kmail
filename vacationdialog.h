/*  -*- c++ -*-
    vacationdialog.h

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_VACATIONDIALOG_H__
#define __KMAIL_VACATIONDIALOG_H__

#include "kdialogbase.h"

class QString;
class QDate;
class QCheckBox;
class KDateWidget;
class KIntSpinBox;

namespace KMail {

  class VacationDialog : public KDialogBase {
    Q_OBJECT
  public:
    VacationDialog( const QString & caption, QWidget * parent=0,
		    const char * name=0, bool modal=true );
    virtual ~VacationDialog();

    bool activateVacation() const;
    virtual void setActivateVacation( bool activate );

    QDate returnDate() const;
    virtual void setReturnDate( const QDate & date );

    int notificationInterval() const;
    virtual void setNotificationInterval( int days );

  protected:
    QCheckBox   * mActiveCheck;
    KDateWidget * mDateWidget;
    KIntSpinBox * mIntervalSpin;
  };

}; // namespace KMail

#endif // __KMAIL_VACATIONDIALOG_H__
