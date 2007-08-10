/*  -*- c++ -*-
    identitylistview.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>
                  2007 Mathias Soeken <msoeken@tzi.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace KPIMIdentities { class Identity; }
class QDropEvent;

namespace KMail {

  class IdentityListView;

  /** @short A QWidgetTreeItem for use in IdentityListView
   *  @author Marc Mutz <mutz@kde.org>
   **/
  class IdentityListViewItem : public QTreeWidgetItem {
  public:
    IdentityListViewItem( IdentityListView *parent,
                          const KPIMIdentities::Identity &ident );
    IdentityListViewItem( IdentityListView *parent, QTreeWidgetItem *after,
                          const KPIMIdentities::Identity &ident );

    uint uoid() const { return mUOID; }
    KPIMIdentities::Identity &identity() const;
    virtual void setIdentity( const KPIMIdentities::Identity &ident );
    void redisplay();
  private:
    void init( const KPIMIdentities::Identity &ident );

  protected:
    uint mUOID;
  };

  /** @short A QTreeWidget for KPIMIdentities::Identity
    * @author Marc Mutz <mutz@kde.org>
    **/
  class IdentityListView : public QTreeWidget {
    Q_OBJECT
  public:
    IdentityListView( QWidget *parent = 0 );
    virtual ~IdentityListView() {}

  public:
    void editItem( QTreeWidgetItem *item, int column = 0 );

  protected slots:
    void commitData( QWidget *editor );

  public slots:
    void slotCustomContextMenuRequested( const QPoint& );

  signals:
    void contextMenu( KMail::IdentityListViewItem *, const QPoint& );
    void rename( KMail::IdentityListViewItem *, const QString& );

  protected:
    virtual void startDrag ( Qt::DropActions supportedActions );
  };


} // namespace KMail

#endif // __KMAIL_IDENTITYLISTVIEW_H__
