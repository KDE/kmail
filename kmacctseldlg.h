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


#ifndef kmacctseldlg_h
#define kmacctseldlg_h

#include <kdialogbase.h>

/** Select account from given list of account types */
class KMAcctSelDlg: public KDialogBase
{
  Q_OBJECT

  public:
    KMAcctSelDlg( QWidget *parent=0, const char *name=0, bool modal=true );

    /**
     * Returns selected button from the account selection group:
     * 0=local mail, 1=pop3.
     */
    int selected(void) const;

  private slots:
    void buttonClicked(int);

  private:
    int mSelectedButton;
};


#endif /*kmacctseldlg_h*/
