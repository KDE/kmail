/*  -*- c++ -*-
    identitycombo.h

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef _KMAIL_IDENTITYCOMBO_H_
#define _KMAIL_IDENTITYCOMBO_H_

#include <qcombobox.h>

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

  QString currentIdentity() const;
  void setCurrentIdentity( const QString & identityName );
  void setCurrentIdentity( const KMIdentity & identity );

signals:
  /** @em Really emitted whenever the current identity changes. Either
   *  by user intervention or on @ref setCurrentIdentity() or if the
   *  current identity disappears or it's default status changes while
   *  this widget is on screen.
   **/
  void identityChanged( const QString & identityName );

public slots:
  /** Connected to @ref IdentityManager::changed(). Reloads the list
   *  of identities.
   **/
  void slotIdentityManagerChanged();

protected slots:
  void slotEmitChanged(int);

protected:
  void reloadCombo();
};



#endif // _KMAIL_IDENTITYCOMBO_H_
