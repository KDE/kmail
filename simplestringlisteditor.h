/*  -*- c++ -*-
    simplestringlisteditor.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2001 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef _SIMPLESTRINGLISTEDITOR_H_
#define _SIMPLESTRINGLISTEDITOR_H_

#include <qwidget.h>
#include <qstringlist.h>
#include <qstring.h>

class QListBox;
class QPushButton;

//
//
// SimpleStringListEditor (a listbox with "add..." and "remove" buttons)
//
//

class SimpleStringListEditor : public QWidget {
  Q_OBJECT
public:
  enum ButtonCode {
    None = 0x00, Add = 0x01,
    Remove = 0x02, Modify = 0x04,
    Up = 0x08, Down = 0x10,
    All = Add|Remove|Modify|Up|Down,
    Unsorted = Add|Remove|Modify
  };

  /** Constructor. Populates the list with @p strings. */
  SimpleStringListEditor( QWidget * parent=0, const char * name=0,
			  ButtonCode buttons=Unsorted,
			  const QString & addLabel=QString::null,
			  const QString & removeLabel=QString::null,
			  const QString & modifyLabel=QString::null,
			  const QString & addDialogLabel=QString::null );

  /** Sets the list of strings displayed to @p strings */
  void setStringList( const QStringList & strings );
  /** Adds @p strings to the list of displayed strings */
  void appendStringList( const QStringList & strings );
  /** Retrieves the current list of strings */
  QStringList stringList() const;

  /** Sets the text of button @p button to @p text */
  void setButtonText( ButtonCode button, const QString & text );

signals:
  /** Connected slots can alter the argument to be added or set the
      argument to QString::null to suppress adding.
  */
  void aboutToAdd(QString&);
  void changed(void);

protected slots:
  void slotAdd();
  void slotRemove();
  void slotModify();
  void slotUp();
  void slotDown();

  void slotSelectionChanged();

protected:
  QListBox      *mListBox;
  QPushButton   *mAddButton;
  QPushButton   *mRemoveButton;
  QPushButton   *mModifyButton;
  QPushButton   *mUpButton;
  QPushButton   *mDownButton;
  const QString mAddDialogLabel;
};




#endif // _SIMPLESTRINGLISTEDITOR_H_
