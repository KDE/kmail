/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, <espen@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code
 *   by Stefan Taferner <taferner@alpin.or.at>
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qradiobutton.h>

#include <klocale.h>

#include "kmacctseldlg.h"
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
  
  QRadioButton *radioButton1 = new QRadioButton(i18n("Local Mailbox"), group );
  vlay->addWidget( radioButton1 );
  QRadioButton *radioButton2  = new QRadioButton(i18n("Pop3"),  group );
  vlay->addWidget( radioButton2 );
  QRadioButton *radioButton3  = 
    new QRadioButton(i18n("Advanced Pop3"), group );
  vlay->addWidget( radioButton3 );

  vlay->addStretch( 10 );

  radioButton3->setChecked(true); // Advanced Pop is most common ?
  buttonClicked(2);
}


void KMAcctSelDlg::buttonClicked( int id )
{
  mSelectedButton = id;
}


int KMAcctSelDlg::selected( void ) const 
{ 
  return mSelectedButton; 
}


