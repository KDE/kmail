/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, <espen@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code
 *   by Stefan Taferner <taferner@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qradiobutton.h>

#include <klocale.h>

#include "kmacctseldlg.moc"

KMAcctSelDlg::KMAcctSelDlg( QWidget *parent, const char *name, bool modal )
  : KDialogBase( parent, name, modal, i18n("Add Account"), Ok|Cancel, Ok )
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  QButtonGroup *group = new QButtonGroup( i18n("Account Type"), page );
  connect(group, SIGNAL(clicked(int)), SLOT(buttonClicked(int)) );

  topLayout->addWidget( group, 10 );
  QVBoxLayout *vlay = new QVBoxLayout( group, spacingHint()*2, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );

  QRadioButton *radioButton1 = new QRadioButton( i18n("&Local mailbox"), group );
  vlay->addWidget( radioButton1 );
  QRadioButton *radioButton2 = new QRadioButton( i18n("&POP3"), group );
  vlay->addWidget( radioButton2 );
  QRadioButton *radioButton3 = new QRadioButton( i18n("&IMAP"), group );
  vlay->addWidget( radioButton3 );
  QRadioButton *radioButton4 = new QRadioButton( i18n("&Disconnected IMAP"), group );
  vlay->addWidget( radioButton4 );
  QRadioButton *radioButton5 = new QRadioButton( i18n("&Maildir mailbox"), group );
  vlay->addWidget( radioButton5 );

  vlay->addStretch( 10 );

  radioButton2->setChecked(true); // Pop is most common ?
  buttonClicked(1);
}


void KMAcctSelDlg::buttonClicked( int id )
{
  mSelectedButton = id;
}


int KMAcctSelDlg::selected( void ) const
{
  return mSelectedButton;
}


