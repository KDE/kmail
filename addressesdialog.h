/*
 * addressesdialog.h
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef ADDRESSESDIALOG_H
#define ADDRESSESDIALOG_H

#include <kdialogbase.h>
#include <klistview.h>
#include <qstringlist.h>
#include <kabc/addressee.h>

namespace KMail {

  struct AddresseeViewItemPrivate;
  class AddresseeViewItem : public KListViewItem
  {
  public:
    enum Category {
      To    =0,
      CC    =1,
      BCC   =2,
      Group =3,
      Entry =4
    };
    AddresseeViewItem( AddresseeViewItem *parent, const KABC::Addressee& addr );
    AddresseeViewItem( KListView *lv, const QString& name, Category cat=Group );
    //AddresseeViewItem( AddresseeViewItem *parent, const QString& name,
    //                       const QString& email = QString::null );
    ~AddresseeViewItem();

    KABC::Addressee  addressee() const;
    Category         category() const;

    QString name()  const;
    QString email() const;

    virtual int compare( QListViewItem * i, int col, bool ascending ) const;

  private:
    AddresseeViewItemPrivate *d;
  };

  struct AddressesDialogPrivate;
  class AddressesDialog : public KDialogBase
  {
    Q_OBJECT
  public:
    AddressesDialog( QWidget *widget=0, const char *name=0 );
    ~AddressesDialog();

    void setSelectedTo( const QStringList& l );
    void setSelectedCC( const QStringList& l );
    void setSelectedBCC( const QStringList& l );

    QStringList to()  const;
    QStringList cc()  const;
    QStringList bcc() const;

    KABC::Addressee::List toAddresses()  const;
    KABC::Addressee::List ccAddresses()  const;
    KABC::Addressee::List bccAddresses() const;

    static QString personalGroup;
    static QString recentGroup;
  protected slots:
    void addSelectedTo();
    void addSelectedCC();
    void addSelectedBCC();

    void removeEntry();
    void saveAs();

    void editEntry();
    void newEntry();
    void deleteEntry();

    void cleanEdit();
    void filterChanged( const QString & );

  protected:
    void initGUI();
    void initConnections();
    void addAddresseeToAvailable( const KABC::Addressee& addr,
                                  AddresseeViewItem* defaultParent=0 );
    void addAddresseeToSelected( const KABC::Addressee& addr,
                                 AddresseeViewItem* defaultParent=0 );
    QStringList entryToString( const KABC::Addressee::List& l ) const;
    KABC::Addressee::List selectedAddressee( KListView* view ) const;
    KABC::Addressee::List allAddressee( AddresseeViewItem* parent ) const;
  private:
    AddressesDialogPrivate *d;
  };

};

#endif /* ADDRESSESDIALOG_H */
