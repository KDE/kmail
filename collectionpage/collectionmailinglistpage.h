/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2005 Till Adam <adam@kde.org>
  Copyright (c) 2011 Montel Laurent <montel@kde.org>
  Copyright (c) 2012 Jonathan Marten <jjm@keelhaul.me.uk>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COLLECTIONMAILINGLISTPAGE_H
#define COLLECTIONMAILINGLISTPAGE_H

#include "messagecore/misc/mailinglist.h"
#include "foldercollection.h"

#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>


class QCheckBox;
class QPushButton;

template <typename T> class QSharedPointer;

class KComboBox;
class KJob;
class KEditListWidget;
class KSqueezedTextLabel;


class CollectionMailingListPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionMailingListPage( QWidget *parent = 0 );
    ~CollectionMailingListPage();

    void load( const Akonadi::Collection & col );
    void save( Akonadi::Collection & col );

    bool canHandle( const Akonadi::Collection & col ) const;

protected:
    void init(const Akonadi::Collection&);

protected slots:
    void slotFetchDone( KJob* job );

private slots:
    /*
   * Detects mailing-list related stuff
   */
    void slotDetectMailingList();
    void slotInvokeHandler();
    void slotMLHandling( int element );
    void slotHoldsML( bool holdsML );
    void slotAddressChanged( int addr );
    void slotConfigChanged();

private:
    void fillMLFromWidgets();
    void fillEditBox();

    Akonadi::Collection mCurrentCollection;
    QSharedPointer<MailCommon::FolderCollection> mFolder;

    int           mLastItem;
    MailingList   mMailingList;
    QCheckBox    *mHoldsMailingList;
    KComboBox    *mMLHandlerCombo;
    QPushButton  *mDetectButton;
    KComboBox    *mAddressCombo;
    KEditListWidget *mEditList;
    KSqueezedTextLabel *mMLId;
    QWidget *mGroupWidget;
    bool changed;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY( CollectionMailingListPageFactory, CollectionMailingListPage )

#endif /* COLLECTIONMAILINGLISTPAGE_H */
