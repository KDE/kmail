/*
    This file is part of KMail.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

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
*/
#ifndef RECIPIENTSPICKER_H
#define RECIPIENTSPICKER_H

#include "recipientseditor.h"

#include <klistview.h>
#include <kabc/addressee.h>

#include <qwidget.h>

class KListViewSearchLine;
class QComboBox;

class RecipientItem
{
  public:
    typedef QValueList<RecipientItem *> List;
  
    RecipientItem();
    
    void setAddressee( const KABC::Addressee & );
    KABC::Addressee addressee() const;

    void setRecipientType( const QString &type );
    QString recipientType() const;

    QString recipient() const;
    
  private:
    KABC::Addressee mAddressee;
    QString mType;
};

class RecipientViewItem : public KListViewItem
{
  public:
    RecipientViewItem( RecipientItem *, KListView * );
    
    RecipientItem *recipientItem() const;
    
  private:
    RecipientItem *mRecipientItem;
};

class RecipientsCollection
{
  public:
    RecipientsCollection();
    
    void setTitle( const QString & );
    QString title() const;
    
    void addItem( RecipientItem * );
    
    RecipientItem::List items() const;

  private:
    QString mTitle;
    RecipientItem::List mItems;
};

class RecipientsPicker : public QWidget
{
    Q_OBJECT
  public:
    RecipientsPicker( QWidget *parent );

    void setRecipients( const Recipient::List & );

  signals:
    void pickedRecipient( const QString & );

  protected:
    void initCollections();
    void insertCollection( RecipientsCollection *coll, int index );

  protected slots:
    void updateList();
    void slotOk();
    void slotPicked( QListViewItem * );
  
  private:
    QComboBox *mCollectionCombo;
    KListView *mRecipientList;
    KListViewSearchLine *mSearchLine;
  
    QMap<int,RecipientsCollection *> mCollectionMap;
    RecipientsCollection *mAllRecipients;
};

#endif
