/*  -*- c++ -*-
    identitylist.h

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

#ifndef __KMAIL_IDENTITYLIST_H__
#define __KMAIL_IDENTITYLIST_H__

#include <klistview.h>

class KMIdentity;
class QDropEvent;
class QDragEvent;

namespace KMail {

  class IdentityListView;

  /** @short A @ref QListViewItem for use in @ref IdentityListView
      @author Marc Mutz <mutz@kde.org>
  **/
  class IdentityListViewItem : public KListViewItem {
  public:
    IdentityListViewItem( IdentityListView * parent,
			  const KMIdentity & ident );
    IdentityListViewItem( IdentityListView * parent, QListViewItem * after,
			  const KMIdentity & ident );

    uint uoid() const { return mUOID; }
    KMIdentity & identity() const;
    virtual void setIdentity( const KMIdentity & ident );
    void redisplay();
  private:
    void init( const KMIdentity & ident );

  protected:
    uint mUOID;
  };

  /** @short A listview for @ref KMIdentity
      @author Marc Mutz <mutz@kde.org>
  **/
  class IdentityListView : public KListView {
    Q_OBJECT
  public:
    IdentityListView( QWidget * parent=0, const char * name=0 );
    virtual ~IdentityListView() {}

  protected:
    bool acceptDrag( QDropEvent * ) const;
    QDragObject * dragObject();
  };


}; // namespace KMail

#endif // __KMAIL_IDENTITYLISTVIEW_H__
