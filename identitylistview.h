/*  -*- c++ -*-
    identitylistview.h

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

#ifndef __KMAIL_IDENTITYLIST_H__
#define __KMAIL_IDENTITYLIST_H__

#include <klistview.h>

namespace KPIM { class Identity; }
class QDropEvent;
class QDragEvent;

namespace KMail {

  class IdentityListView;

  /** @short A @see QListViewItem for use in @see IdentityListView
      @author Marc Mutz <mutz@kde.org>
  **/
  class IdentityListViewItem : public KListViewItem {
  public:
    IdentityListViewItem( IdentityListView * parent,
			  const KPIM::Identity & ident );
    IdentityListViewItem( IdentityListView * parent, QListViewItem * after,
			  const KPIM::Identity & ident );

    uint uoid() const { return mUOID; }
    KPIM::Identity & identity() const;
    virtual void setIdentity( const KPIM::Identity & ident );
    void redisplay();
  private:
    void init( const KPIM::Identity & ident );

  protected:
    uint mUOID;
  };

  /** @short A listview for @see KPIM::Identity
      @author Marc Mutz <mutz@kde.org>
  **/
  class IdentityListView : public KListView {
    Q_OBJECT
  public:
    IdentityListView( QWidget * parent=0, const char * name=0 );
    virtual ~IdentityListView() {}

  public slots:
    void rename( QListViewItem *, int );

  protected:
    bool acceptDrag( QDropEvent * ) const;
    QDragObject * dragObject();
  };


} // namespace KMail

#endif // __KMAIL_IDENTITYLISTVIEW_H__
