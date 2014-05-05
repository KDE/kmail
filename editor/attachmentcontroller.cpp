/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * Various authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "attachmentcontroller.h"

#include "attachmentview.h"
#include "messagecomposer/job/attachmentfrompublickeyjob.h"
#include "foldercollection.h"
#include "settings/globalsettings.h"
#include "kmcommands.h"
#include "editor/kmcomposewin.h"
#include "kmkernel.h"
#include "kmreadermainwin.h"
#include "util/mailutil.h"

#include <AkonadiCore/itemfetchjob.h>
#include <kabc/addressee.h>
#include <qdebug.h>
#include <libkleo/kleo/cryptobackendfactory.h>

#include <messagecomposer/attachment/attachmentmodel.h>
#include <messagecore/attachment/attachmentpart.h>

using namespace KMail;
using namespace KPIM;
using namespace MailCommon;
using namespace MessageCore;

AttachmentController::AttachmentController( MessageComposer::AttachmentModel *model, AttachmentView *view, KMComposeWin *composer )
    : AttachmentControllerBase( model, composer, composer->actionCollection() ),
      mComposer(composer),
      mView(view)
{
    connect( composer, SIGNAL(identityChanged(KPIMIdentities::Identity)),
             this, SLOT(identityChanged()) );

    connect( view, SIGNAL(contextMenuRequested()), this, SLOT(showContextMenu()) );
    connect( view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(selectionChanged()) );
    connect( view, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(doubleClicked(QModelIndex)) );

    connect( this, SIGNAL(refreshSelection()), SLOT(selectionChanged()));

    connect( this, SIGNAL(showAttachment(KMime::Content*,QByteArray)),
             SLOT(onShowAttachment(KMime::Content*,QByteArray)));
    connect( this, SIGNAL(selectedAllAttachment()), SLOT(slotSelectAllAttachment()));
    connect( model, SIGNAL(attachItemsRequester(Akonadi::Item::List)), this, SLOT(addAttachmentItems(Akonadi::Item::List)) );
}

AttachmentController::~AttachmentController()
{
}

void AttachmentController::slotSelectAllAttachment()
{
    mView->selectAll();
}

void AttachmentController::identityChanged()
{
    const KPIMIdentities::Identity &identity = mComposer->identity();

    // "Attach public key" is only possible if OpenPGP support is available:
    enableAttachPublicKey( Kleo::CryptoBackendFactory::instance()->openpgp() );

    // "Attach my public key" is only possible if OpenPGP support is
    // available and the user specified his key for the current identity:
    enableAttachMyPublicKey( Kleo::CryptoBackendFactory::instance()->openpgp() && !identity.pgpEncryptionKey().isEmpty() );
}

void AttachmentController::attachMyPublicKey()
{
    const KPIMIdentities::Identity &identity = mComposer->identity();
    qDebug() << identity.identityName();
    exportPublicKey( QString::fromLatin1(identity.pgpEncryptionKey()) );
}

void AttachmentController::actionsCreated()
{
    // Disable public key actions if appropriate.
    identityChanged();

    // Disable actions like 'Remove', since nothing is currently selected.
    selectionChanged();
}

void AttachmentController::addAttachmentItems( const Akonadi::Item::List &items )
{
    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob( items, this );
    itemFetchJob->fetchScope().fetchFullPayload( true );
    itemFetchJob->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    connect( itemFetchJob, SIGNAL(result(KJob*)), mComposer, SLOT(slotFetchJob(KJob*)) );
}

void AttachmentController::selectionChanged()
{
    const QModelIndexList selectedRows = mView->selectionModel()->selectedRows();
    AttachmentPart::List selectedParts;
    foreach( const QModelIndex &index, selectedRows ) {
        AttachmentPart::Ptr part = mView->model()->data(
                    index, MessageComposer::AttachmentModel::AttachmentPartRole ).value<AttachmentPart::Ptr>();
        selectedParts.append( part );
    }
    setSelectedParts( selectedParts );
}

void AttachmentController::onShowAttachment( KMime::Content *content, const QByteArray &charset )
{
    KMReaderMainWin *win =
            new KMReaderMainWin( content, false, QString::fromLatin1(charset) );
    win->show();
}

void AttachmentController::doubleClicked( const QModelIndex &itemClicked )
{
    if ( !itemClicked.isValid() ) {
        qDebug() << "Received an invalid item clicked index";
        return;
    }
    // The itemClicked index will contain the column information. But we want to retrieve
    // the AttachmentPart, so we must recreate the QModelIndex without the column information
    const QModelIndex &properItemClickedIndex = mView->model()->index( itemClicked.row(), 0 );
    AttachmentPart::Ptr part = mView->model()->data(
                properItemClickedIndex,
                MessageComposer::AttachmentModel::AttachmentPartRole ).value<AttachmentPart::Ptr>();

    // We can't edit encapsulated messages, but we can view them.
    if ( part->isMessageOrMessageCollection() ) {
        viewAttachment( part );
    }
    else {
        editAttachment( part );
    }
}

