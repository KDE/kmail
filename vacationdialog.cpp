/*  -*- c++ -*-
    vacationdialog.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "vacationdialog.h"

#include <kdatewidget.h>
#include <knuminput.h>
#include <klocale.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>

namespace KMail {

  VacationDialog::VacationDialog( const QString & caption, QWidget * parent,
				  const char * name, bool modal )
    : KDialogBase( Plain, caption, Ok|Cancel|Help, Ok, parent, name, modal )
  {
    int row = -1;

    QGridLayout * glay = new QGridLayout( plainPage(), 5, 2, 0, spacingHint() );
    glay->setRowStretch( 4, 1 );
    glay->setColStretch( 1, 1 );

    // explanation label:
    ++row;
    glay->addMultiCellWidget( new QLabel( i18n("Configure vacation "
					       "notifications to be sent:"),
					  plainPage() ), row, row, 0, 1 );

    // Activate checkbox:
    ++row;
    mActiveCheck = new QCheckBox( i18n("&Activate vacation notifications"), plainPage() );
    glay->addMultiCellWidget( mActiveCheck, row, row, 0, 1 );

    // Date of return lineedit and label:
    ++row;
    mDateWidget = new KDateWidget( plainPage(), "mDateWidget" );
    glay->addWidget( new QLabel( mDateWidget, i18n("&Date of return:"), plainPage() ), row, 0 );
    glay->addWidget( mDateWidget, row, 1 );

    // "Resent only after" lineedit and label:
    ++row;
    mIntervalSpin = new KIntSpinBox( 1, 356, 1, 7, 10, plainPage(), "mIntervalSpin" );
    mIntervalSpin->setSuffix( i18n(" days") );
    glay->addWidget( new QLabel( mIntervalSpin, i18n("&Resend notification only after:"), plainPage() ), row, 0 );
    glay->addWidget( mIntervalSpin, row, 1 );

    // row 4 is for stretch.

  }

  VacationDialog::~VacationDialog() {
    kdDebug(5006) << "~VacationDialog()" << endl;
  }

  bool VacationDialog::activateVacation() const {
    return mActiveCheck->isChecked();
  }

  void VacationDialog::setActivateVacation( bool activate ) {
    mActiveCheck->setChecked( activate );
  }

  QDate VacationDialog::returnDate() const {
    return mDateWidget->date();
  }

  void VacationDialog::setReturnDate( const QDate & date ) {
    mDateWidget->setDate( date );
  }

  int VacationDialog::notificationInterval() const {
    return mIntervalSpin->value();
  }

  void VacationDialog::setNotificationInterval( int days ) {
    mIntervalSpin->setValue( days );
  }

}; // namespace KMail

#include "vacationdialog.moc"
