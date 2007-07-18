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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "kmacctseldlg.h"

#include <QGroupBox>
#include <QButtonGroup>
#include <QLayout>
#include <QRadioButton>
#include <QVBoxLayout>

#include <klocale.h>

KMAcctSelDlg::KMAcctSelDlg( QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n("Add Account") );
  setButtons( Ok|Cancel );
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );

  QGroupBox *group = new QGroupBox(i18n("Account Type"));
  topLayout->addWidget( group, 10 );

  QVBoxLayout *vlay = new QVBoxLayout( group );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( spacingHint()*2 );

  mAccountTypeGroup = new QButtonGroup;
  QRadioButton *radioButton1 = addButton( KAccount::Local, vlay );
  QRadioButton *radioButton2 = addButton( KAccount::Pop, vlay );
  QRadioButton *radioButton3 = addButton( KAccount::Imap, vlay );
  QRadioButton *radioButton4 = addButton( KAccount::DImap, vlay );
  QRadioButton *radioButton5 = addButton( KAccount::Maildir, vlay );

  radioButton2->setChecked(true); // Pop is most common ?
}

QRadioButton* KMAcctSelDlg::addButton( const KAccount::Type type, 
                                       QLayout *layout )
{
  QRadioButton *radioButton =
      new QRadioButton( KAccount::displayNameForType (type ) );
  mAccountTypeGroup->addButton( radioButton, type );
  layout->addWidget( radioButton );
  return radioButton;
}

KAccount::Type KMAcctSelDlg::selected( void ) const
{
  return static_cast<KAccount::Type>( mAccountTypeGroup->checkedId() );
}

#include "kmacctseldlg.moc"


