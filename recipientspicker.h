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

#include "recipientseditor.h"

#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>
#include <KDialog>
#include <KTreeWidgetSearchLine>

#include <QPixmap>
#include <QTreeWidgetItem>

class KComboBox;

class QKeyEvent;
class QTreeWidget;
class QWidget;

namespace KABC {
class DistributionList;
}

namespace KPIM {
class  LdapSearchDialog;
}

class RecipientItem
{
  public:
    typedef QList<RecipientItem *> List;

    RecipientItem();
    RecipientItem( RecipientItem *);
    void setDistributionList( KABC::DistributionList * );
    KABC::DistributionList * distributionList() const;
    const KABC::Addressee * adressee() const;

    void setAddressee( const KABC::Addressee &, const QString &email );

    void setRecipientType( const QString &type );
    QString recipientType() const;

    QString recipient() const;

    QPixmap icon() const;
    QString name() const;
    QString email() const;

    QString key() const { return mKey; }

    QString tooltip() const;

    void addAlternativeEmailItem( RecipientItem *altRecipientItem );
    const List alternativeEmailList() const;
    bool isDistributionList() const;
    bool operator==( const Recipient& ) const;

  private:
    QString createTooltip( KABC::DistributionList * ) const;

    KABC::Addressee mAddressee;
    QString mName;
    QString mEmail;
    QString mRecipient;
    KABC::DistributionList *mDistributionList;
    QString mType;
    QString mTooltip;

    QPixmap mIcon;

    QString mKey;

    List mAlternativeEmailList;
};

class RecipientViewItem : public QTreeWidgetItem
{
  public:
    RecipientViewItem( RecipientItem *, QTreeWidget * );
    RecipientViewItem( RecipientItem *, RecipientViewItem * );
    void init( RecipientItem *item );

    RecipientItem *recipientItem() const;

  private:
    RecipientItem *mRecipientItem;
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

class SearchLine : public KTreeWidgetSearchLine
{
    Q_OBJECT
  public:
    SearchLine( QWidget *parent, QTreeWidget *listView );

  signals:
    void downPressed();

  protected:
    void keyPressEvent( QKeyEvent * );
};

//This is a TreeWidget which has an additional signal, returnPressed().
class RecipientsTreeWidget : public QTreeWidget
{
  Q_OBJECT
  public:
    RecipientsTreeWidget( QWidget *parent );

  signals:

    //This signal is emitted whenever the user presses the return key and this
    //widget has focus.
    void returnPressed();

  protected:

    //This function is reimplemented so that the returnPressed() signal can
    //be emitted.
    virtual void keyPressEvent ( QKeyEvent *event );
};

using namespace KABC;

class RecipientsPicker : public KDialog
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
    void slotPicked( QTreeWidgetItem * );
    void slotPicked();
    void setFocusList();
    void insertAddressBook( AddressBook * );
    void slotSearchLDAP();
    void ldapSearchResult();
  private:
    KABC::StdAddressBook *mAddressBook;

    KComboBox *mCollectionCombo;
    RecipientsTreeWidget *mRecipientList;
    KTreeWidgetSearchLine *mSearchLine;

    QPushButton *mToButton;
    QPushButton *mCcButton;
    QPushButton *mBccButton;

    QPushButton *mSearchLDAPButton;
    KPIM::LdapSearchDialog *mLdapSearchDialog;

    QMap<int,RecipientsCollection *> mCollectionMap;
    RecipientsCollection *mAllRecipients;
    RecipientsCollection *mDistributionLists;
    RecipientsCollection *mSelectedRecipients;

    Recipient::Type mDefaultType;

    static const QString mSeparatorString;
};

#endif
