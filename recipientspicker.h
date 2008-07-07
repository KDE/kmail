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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef RECIPIENTSPICKER_H
#define RECIPIENTSPICKER_H

#include <config.h> // for KDEPIM_NEW_DISTRLISTS

#include "recipientseditor.h"

#include <klistview.h>
#include <klistviewsearchline.h>
#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>

#include <qwidget.h>
#include <qdialog.h>
#include <qtooltip.h>

class QComboBox;

#ifdef KDEPIM_NEW_DISTRLISTS
#include <libkdepim/distributionlist.h>
#else
namespace KABC {
class DistributionList;
class DistributionListManager;
}
#endif

namespace KPIM {
class  LDAPSearchDialog;
}

class RecipientItem
{
  public:
    typedef QValueList<RecipientItem *> List;

#ifdef KDEPIM_NEW_DISTRLISTS
    RecipientItem( KABC::AddressBook *ab );
    void setDistributionList( KPIM::DistributionList& );
    KPIM::DistributionList& distributionList();
#else
    RecipientItem();
    void setDistributionList( KABC::DistributionList * );
    KABC::DistributionList * distributionList();
#endif
    void setAddressee( const KABC::Addressee &, const QString &email );

    void setRecipientType( const QString &type );
    QString recipientType() const;

    QString recipient() const;

    QPixmap icon() const;
    QString name() const;
    QString email() const;

    QString key() const { return mKey; }

    QString tooltip() const;

  private:
#ifdef KDEPIM_NEW_DISTRLISTS
    QString createTooltip( KPIM::DistributionList & ) const;
#else
    QString createTooltip( KABC::DistributionList * ) const;
#endif

    KABC::Addressee mAddressee;
    QString mName;
    QString mEmail;
    QString mRecipient;
#ifdef KDEPIM_NEW_DISTRLISTS
    KPIM::DistributionList mDistributionList;
    KABC::AddressBook *mAddressBook;
#else
    KABC::DistributionList *mDistributionList;
#endif
    QString mType;
    QString mTooltip;
    
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
    RecipientsCollection( const QString & );
    ~RecipientsCollection();

    void setReferenceContainer( bool );
    bool isReferenceContainer() const;

    void setTitle( const QString & );
    QString title() const;

    void addItem( RecipientItem * );

    RecipientItem::List items() const;

    bool hasEquivalentItem( RecipientItem * ) const;
    RecipientItem * getEquivalentItem( RecipientItem *) const;

    void clear();

    void deleteAll();

    QString id() const;

  private:
    // flag to indicate if this collection contains just references
    // or should manage memory (de)allocation as well.
    bool mIsReferenceContainer;
    QString mId;
    QString mTitle;
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

using namespace KABC;

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

    void rebuildAllRecipientsList();

  protected slots:
    void updateList();
    void slotToClicked();
    void slotCcClicked();
    void slotBccClicked();
    void slotPicked( QListViewItem * );
    void slotPicked();
    void setFocusList();
    void resetSearch();
    void insertAddressBook( AddressBook * );
    void slotSearchLDAP();
    void ldapSearchResult();
  private:
    KABC::StdAddressBook *mAddressBook;

    QComboBox *mCollectionCombo;
    KListView *mRecipientList;
    KListViewSearchLine *mSearchLine;

    QPushButton *mToButton;
    QPushButton *mCcButton;
    QPushButton *mBccButton;
    
    QPushButton *mSearchLDAPButton;
    KPIM::LDAPSearchDialog            *ldapSearchDialog;
    
    QMap<int,RecipientsCollection *> mCollectionMap;
    RecipientsCollection *mAllRecipients;
    RecipientsCollection *mDistributionLists;
    RecipientsCollection *mSelectedRecipients;

#ifndef KDEPIM_NEW_DISTRLISTS
    KABC::DistributionListManager *mDistributionListManager;
#endif

    Recipient::Type mDefaultType;
};

#endif
