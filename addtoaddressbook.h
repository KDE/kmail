/* This file implements the dialog to add an email address to the KDE   
   addressbook database. 
 *
 * copyright:  (C) Mirko Sucker, 1998, 1999, 2000
 * mail to:    Mirko Sucker <mirko@kde.org>
 * requires:   recent C++-compiler, at least Qt 2.0
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 
 * $Revision$
 */

#ifndef ADDTOADDRESSBOOK_H
#define ADDTOADDRESSBOOK_H

#include <kdialogbase.h>
#include <qstring.h>

class QWidget;
class KabAPI;
class QListBox;

class AddToKabDialog : public KDialogBase
{
  Q_OBJECT
public:
  AddToKabDialog(QString url, KabAPI *, QWidget *parent);
  ~AddToKabDialog();
protected slots:
  /** Create a new entry and add it to the address book. */
  void newEntry();
  /** Add the email address to the selected entry. */
  void addToEntry();
  /** An entry has been highlighted from the list. */
  void highlighted(int);
protected:
  /** Remember the pointer to the address database interface. */
  KabAPI *api;
  /** Remember the url to store. */
  QString url;
  /** The list box showing the entries. */
  QListBox *list;
};

#endif // ADDTOADDRESSBOOK_H

