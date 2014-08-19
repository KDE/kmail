/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   Copyright (C) 2010 Volker Krause <vkrause@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "identitypage.h"

#include "identitydialog.h"
#include "newidentitydialog.h"
#ifndef KCM_KPIMIDENTITIES_STANDALONE
#include "kmkernel.h"
#include "settings/globalsettings.h"
#endif

#include <mailcommon/kernel/mailkernel.h>
#include <messageviewer/utils/autoqpointer.h>
#include <KPIMIdentities/kpimidentities/identity.h>
#include <KPIMIdentities/kpimidentities/identitymanager.h>

#include <KMessageBox>
#include <QMenu>
#include <KLocalizedString>
#include <QDebug>

using namespace KMail;

QString IdentityPage::helpAnchor() const
{
    return QString::fromLatin1( "configure-identity" );
}

IdentityPage::IdentityPage( QWidget *parent )
    : ConfigModule( parent ),
      mIdentityDialog( 0 ),
      mOldNumberOfIdentities( 0 ),
      mIdentityManager( 0 )
{
    if ( !MailCommon::Kernel::self()->kernelIsRegistered() ) {
        return;
    }
    mIdentityManager = KernelIf->identityManager();

    mIPage.setupUi( this );
    mIPage.mIdentityList->setIdentityManager( mIdentityManager );

    connect( mIPage.mIdentityList, SIGNAL(itemSelectionChanged()),
             SLOT(slotIdentitySelectionChanged()) );
    connect( this, SIGNAL(changed(bool)),
             SLOT(slotIdentitySelectionChanged()) );
    connect( mIPage.mIdentityList, SIGNAL(rename(KMail::IdentityListViewItem*,QString)),
             SLOT(slotRenameIdentity(KMail::IdentityListViewItem*,QString)) );
    connect( mIPage.mIdentityList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
             SLOT(slotModifyIdentity()) );
    connect( mIPage.mIdentityList, SIGNAL(contextMenu(KMail::IdentityListViewItem*,QPoint)),
             SLOT(slotContextMenu(KMail::IdentityListViewItem*,QPoint)) );
    // ### connect dragged(...), ...

    connect( mIPage.mButtonAdd, SIGNAL(clicked()),
             this, SLOT(slotNewIdentity()) );
    connect( mIPage.mModifyButton, SIGNAL(clicked()),
             this, SLOT(slotModifyIdentity()) );
    connect( mIPage.mRenameButton, SIGNAL(clicked()),
             this, SLOT(slotRenameIdentity()) );
    connect( mIPage.mRemoveButton, SIGNAL(clicked()),
             this, SLOT(slotRemoveIdentity()) );
    connect( mIPage.mSetAsDefaultButton, SIGNAL(clicked()),
             this, SLOT(slotSetAsDefault()) );
}

IdentityPage::~IdentityPage()
{
}

void IdentityPage::load()
{
    if ( !MailCommon::Kernel::self()->kernelIsRegistered() ) {
        return;
    }
    mOldNumberOfIdentities = mIdentityManager->shadowIdentities().count();
    // Fill the list:
    mIPage.mIdentityList->clear();
    QTreeWidgetItem *item = 0;
    KPIMIdentities::IdentityManager::Iterator end( mIdentityManager->modifyEnd() );

    for ( KPIMIdentities::IdentityManager::Iterator it = mIdentityManager->modifyBegin(); it != end; ++it ) {
        item = new IdentityListViewItem( mIPage.mIdentityList, item, *it );
    }
    if ( mIPage.mIdentityList->currentItem() ) {
        mIPage.mIdentityList->currentItem()->setSelected( true );
    }
}

void IdentityPage::save()
{
    if ( !MailCommon::Kernel::self()->kernelIsRegistered() ) {
        return;
    }
    mIdentityManager->sort();
    mIdentityManager->commit();

#ifndef KCM_KPIMIDENTITIES_STANDALONE
    if( mOldNumberOfIdentities < 2 && mIPage.mIdentityList->topLevelItemCount() > 1 ) {
        // have more than one identity, so better show the combo in the
        // composer now:
        int showHeaders = GlobalSettings::self()->headers();
        showHeaders |= KMail::Composer::HDR_IDENTITY;
        GlobalSettings::self()->setHeaders( showHeaders );
    }
    // and now the reverse
    if( mOldNumberOfIdentities > 1 && mIPage.mIdentityList->topLevelItemCount() < 2 ) {
        // have only one identity, so remove the combo in the composer:
        int showHeaders = GlobalSettings::self()->headers();
        showHeaders &= ~KMail::Composer::HDR_IDENTITY;
        GlobalSettings::self()->setHeaders( showHeaders );
    }
#endif
}

void IdentityPage::slotNewIdentity()
{
    Q_ASSERT( !mIdentityDialog );

    MessageViewer::AutoQPointer<NewIdentityDialog> dialog( new NewIdentityDialog(
                                                               mIdentityManager, this ) );
    dialog->setObjectName( QLatin1String("new") );

    if ( dialog->exec() == QDialog::Accepted && dialog ) {
        QString identityName = dialog->identityName().trimmed();
        Q_ASSERT( !identityName.isEmpty() );

        //
        // Construct a new Identity:
        //
        switch ( dialog->duplicateMode() ) {
        case NewIdentityDialog::ExistingEntry:
        {
            KPIMIdentities::Identity &dupThis = mIdentityManager->modifyIdentityForName( dialog->duplicateIdentity() );
            mIdentityManager->newFromExisting( dupThis, identityName );
            break;
        }
        case NewIdentityDialog::ControlCenter:
            mIdentityManager->newFromControlCenter( identityName );
            break;
        case NewIdentityDialog::Empty:
            mIdentityManager->newFromScratch( identityName );
        default: ;
        }

        //
        // Insert into listview:
        //
        KPIMIdentities::Identity &newIdent = mIdentityManager->modifyIdentityForName( identityName );
        QTreeWidgetItem *item = 0;
        if ( !mIPage.mIdentityList->selectedItems().isEmpty() ) {
            item = mIPage.mIdentityList->selectedItems()[0];
        }

        QTreeWidgetItem * newItem = 0;
        if ( item ) {
            newItem = new IdentityListViewItem( mIPage.mIdentityList, mIPage.mIdentityList->itemAbove( item ), newIdent );
        } else {
            newItem = new IdentityListViewItem( mIPage.mIdentityList, newIdent );
        }

        mIPage.mIdentityList->selectionModel()->clearSelection();
        if ( newItem ) {
            newItem->setSelected( true );
        }

        slotModifyIdentity();
        updateButtons();
    }
}

void IdentityPage::slotModifyIdentity()
{
    Q_ASSERT( !mIdentityDialog );

    IdentityListViewItem *item = 0;
    if ( !mIPage.mIdentityList->selectedItems().isEmpty() ) {
        item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems().first() );
    }
    if ( !item ) {
        return;
    }

    mIdentityDialog = new IdentityDialog( this );
    mIdentityDialog->setIdentity( item->identity() );

    // Hmm, an unmodal dialog would be nicer, but a modal one is easier ;-)
    if ( mIdentityDialog->exec() == QDialog::Accepted ) {
        mIdentityDialog->updateIdentity( item->identity() );
        item->redisplay();
        save();
    }

    delete mIdentityDialog;
    mIdentityDialog = 0;
}

void IdentityPage::slotRemoveIdentity()
{
    Q_ASSERT( !mIdentityDialog );

    if ( mIdentityManager->shadowIdentities().count() < 2 ) {
        qCritical() << "Attempted to remove the last identity!";
    }

    IdentityListViewItem *item = 0;
    if ( !mIPage.mIdentityList->selectedItems().isEmpty() ) {
        item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems()[0] );
    }
    if ( !item ) {
        return;
    }

    const QString msg = i18n( "<qt>Do you really want to remove the identity named "
                        "<b>%1</b>?</qt>", item->identity().identityName() );
    if( KMessageBox::warningContinueCancel( this, msg, i18n("Remove Identity"),
                                            KGuiItem(i18n("&Remove"),
                                                     QLatin1String("edit-delete")) )
            == KMessageBox::Continue ) {
        if ( mIdentityManager->removeIdentity( item->identity().identityName() ) ) {
            delete item;
            if ( mIPage.mIdentityList->currentItem() ) {
                mIPage.mIdentityList->currentItem()->setSelected( true );
            }
            refreshList();
            updateButtons();
        }
    }
}

void IdentityPage::slotRenameIdentity()
{
    Q_ASSERT( !mIdentityDialog );

    QTreeWidgetItem *item = 0;

    if ( !mIPage.mIdentityList->selectedItems().isEmpty() ) {
        item = mIPage.mIdentityList->selectedItems().first();
    }
    if ( !item )
        return;

    mIPage.mIdentityList->editItem( item );
}

void IdentityPage::slotRenameIdentity( KMail::IdentityListViewItem *item , const QString &text )
{
    if ( !item )
        return;

    const QString newName = text.trimmed();
    if ( !newName.isEmpty() &&
         !mIdentityManager->shadowIdentities().contains( newName ) ) {
        KPIMIdentities::Identity &ident = item->identity();
        ident.setIdentityName( newName );
        save();
    }
    item->redisplay();
}

void IdentityPage::slotContextMenu( IdentityListViewItem *item, const QPoint &pos )
{
    QMenu *menu = new QMenu( this );
    menu->addAction( i18n( "Add..." ), this, SLOT(slotNewIdentity()) );
    if ( item ) {
        menu->addAction( i18n( "Modify..." ), this, SLOT(slotModifyIdentity()) );
        menu->addAction( i18n( "Rename" ), this, SLOT(slotRenameIdentity()) );
        if ( mIPage.mIdentityList->topLevelItemCount() > 1 ) {
            menu->addAction( i18n( "Remove" ), this, SLOT(slotRemoveIdentity()) );
        }
        if ( !item->identity().isDefault() ) {
            menu->addSeparator();
            menu->addAction( i18n( "Set as Default" ), this, SLOT(slotSetAsDefault()) );
        }
    }
    menu->exec( pos );
    delete menu;
}


void IdentityPage::slotSetAsDefault()
{
    Q_ASSERT( !mIdentityDialog );

    IdentityListViewItem *item = 0;
    if ( !mIPage.mIdentityList->selectedItems().isEmpty() ) {
        item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems().first() );
    }
    if ( !item ) {
        return;
    }

    mIdentityManager->setAsDefault( item->identity().uoid() );
    refreshList();
    mIPage.mSetAsDefaultButton->setEnabled( false );
}

void IdentityPage::refreshList()
{
    const int numberOfTopLevel( mIPage.mIdentityList->topLevelItemCount() );
    for ( int i = 0; i < numberOfTopLevel; ++i ) {
        IdentityListViewItem *item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->topLevelItem( i ) );
        if ( item ) {
            item->redisplay();
        }
    }
    save();
}

void IdentityPage::slotIdentitySelectionChanged()
{
    updateButtons();
}

void IdentityPage::updateButtons()
{
    IdentityListViewItem *item = 0;
    if ( !mIPage.mIdentityList->selectedItems().isEmpty() ) {
        item = dynamic_cast<IdentityListViewItem*>( mIPage.mIdentityList->selectedItems().first() );
    }
    mIPage.mRemoveButton->setEnabled( item && mIPage.mIdentityList->topLevelItemCount() > 1 );
    mIPage.mModifyButton->setEnabled( item );
    mIPage.mRenameButton->setEnabled( item );
    mIPage.mSetAsDefaultButton->setEnabled( item && !item->identity().isDefault() );
}

