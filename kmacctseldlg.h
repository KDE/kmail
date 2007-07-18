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


#ifndef kmacctseldlg_h
#define kmacctseldlg_h

#include <kdialog.h>
#include <kaccount.h>

class QButtonGroup;
class QRadioButton;
class QLayout;

/** Select account from given list of account types */
class KMAcctSelDlg: public KDialog
{
  Q_OBJECT

  public:
    KMAcctSelDlg( QWidget *parent=0 );

    /**
     * Returns selected button from the account selection group.
     * By default, KAccount::Pop is selected.
     */
    KAccount::Type selected(void) const;

  private:

    // Small Helper function which creates a new button and adds it to the
    // layout and the button group.
    QRadioButton *addButton( const KAccount::Type type, QLayout *layout );

    QButtonGroup *mAccountTypeGroup;
};


#endif /*kmacctseldlg_h*/
