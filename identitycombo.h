/*  -*- c++ -*-
    identitycombo.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

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

#ifndef _KMAIL_IDENTITYCOMBO_H_
#define _KMAIL_IDENTITYCOMBO_H_

#include <qcombobox.h>
#include <qvaluelist.h>

class KMIdentity;
class QString;

/**
 * @short A combo box that always shows the up-to-date identity list.
 * @author Marc Mutz <mutz@kde.org>
 **/

class IdentityCombo : public QComboBox {
  Q_OBJECT
public:
  IdentityCombo( QWidget * parent=0, const char * name=0 );

  QString currentIdentityName() const;
  uint currentIdentity() const;
  void setCurrentIdentity( const QString & identityName );
  void setCurrentIdentity( const KMIdentity & identity );
  void setCurrentIdentity( uint uoid );

signals:
  /** @deprecated
   *  @em Really emitted whenever the current identity changes. Either
   *  by user intervention or on @ref setCurrentIdentity() or if the
   *  current identity disappears.
   **/
  void identityChanged( const QString & identityName );

  /** @em Really emitted whenever the current identity changes. Either
   *  by user intervention or on @ref setCurrentIdentity() or if the
   *  current identity disappears.
   *  
   *  You might also want to listen to @ref IdentityManager::changed,
   *  @ref IdentityManager::deleted and @ref IdentityManager::added.
   **/
  void identityChanged( uint uoid );

public slots:
  /** Connected to @ref IdentityManager::changed(). Reloads the list
   *  of identities.
   **/
  void slotIdentityManagerChanged();

protected slots:
  void slotEmitChanged(int);

protected:
  void reloadCombo();
  void reloadUoidList();

protected:
  QValueList<uint> mUoidList;
};



#endif // _KMAIL_IDENTITYCOMBO_H_
