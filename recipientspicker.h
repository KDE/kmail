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
#include <klistviewsearchline.h>
#include <kabc/addressee.h>

#include <qwidget.h>

class QComboBox;

namespace KABC {
class DistributionList;
class DistributionListManager;
}

class RecipientItem
{
  public:
    typedef QValueList<RecipientItem *> List;
  
    RecipientItem();

    void setDistributionList( KABC::DistributionList * );
    void setAddressee( const KABC::Addressee &, const QString &email );

    void setRecipientType( const QString &type );
    QString recipientType() const;

    QString recipient() const;
    
    QPixmap icon() const;
    QString name() const;
    QString email() const;
    
    QString key() const { return mKey; }

    QString toolTip() const;
    
  private:
    KABC::Addressee mAddressee;
    QString mEmail;
    KABC::DistributionList *mDistributionList;
    QString mType;
    
    QPixmap mIcon;
    
    QString mKey;
};

class RecipientViewItem : public KListViewItem
{
  public:
    RecipientViewItem( RecipientItem *, KListView * );
    
    RecipientItem *recipientItem() const;
    
  private:
    RecipientItem *mRecipientItem;
};

class RecipientsListToolTip : public QToolTip
{
  public:
    RecipientsListToolTip( QWidget *parent, KListView * );

  protected:
    void maybeTip( const QPoint &pos );

  private:
    KListView *mListView;
};

class RecipientsCollection
{
  public:
    RecipientsCollection();
    ~RecipientsCollection();
    
    void setTitle( const QString & );
    QString title() const;
    
    void addItem( RecipientItem * );
    
    RecipientItem::List items() const;

    bool hasEquivalentItem( RecipientItem * ) const;

    void clear();

    void deleteAll();

  private:
    QString mTitle;
    RecipientItem::List mItems;
    QMap<QString, RecipientItem *> mKeyMap;
};

class SearchLine : public KListViewSearchLine
{
    Q_OBJECT
  public:
    SearchLine( QWidget *parent, KListView *listView );
    
  signals:
    void downPressed();
    
  protected:
    void keyPressEvent( QKeyEvent * );
};

class RecipientsPicker : public QDialog
{
    Q_OBJECT
  public:
    RecipientsPicker( QWidget *parent );
    ~RecipientsPicker();

    void setRecipients( const Recipient::List & );
    void updateRecipient( const Recipient & );

    void setDefaultType( Recipient::Type );

  signals:
    void pickedRecipient( const Recipient & );

  protected:
    void initCollections();
    void insertDistributionLists();
    void insertRecentAddresses();
    void insertCollection( RecipientsCollection *coll );

    void keyPressEvent( QKeyEvent *ev );

    void readConfig();
    void writeConfig();

    void pick( Recipient::Type );

    void setDefaultButton( QPushButton *button );

  protected slots:
    void updateList();
    void slotToClicked();
    void slotCcClicked();
    void slotBccClicked();
    void slotPicked( QListViewItem * );
    void slotPicked();
    void setFocusList();
    void resetSearch();
  
  private:
    QComboBox *mCollectionCombo;
    KListView *mRecipientList;
    KListViewSearchLine *mSearchLine;

    QPushButton *mToButton;
    QPushButton *mCcButton;
    QPushButton *mBccButton;
  
    QMap<int,RecipientsCollection *> mCollectionMap;
    RecipientsCollection *mAllRecipients;
    RecipientsCollection *mSelectedRecipients;

    KABC::DistributionListManager *mDistributionListManager;
    
    Recipient::Type mDefaultType;
};

#endif
