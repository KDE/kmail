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
#include "attachmentfrompublickeyjob.h"
#include "foldercollection.h"
#include "globalsettings.h"
#include "kmcommands.h"
#include "kmcomposewin.h"
#include "kmkernel.h"
#include "kmreadermainwin.h"
#include "mailutil.h"

#include <akonadi/itemfetchjob.h>
#include <kabc/addressee.h>
#include <kdebug.h>
#include <libkleo/kleo/cryptobackendfactory.h>
#include <libkleo/ui/keyselectiondialog.h>

#include <messagecomposer/attachmentmodel.h>
#include <messagecore/attachmentcompressjob.h>
#include <messagecore/attachmentfrommimecontentjob.h>
#include <messagecore/attachmentfromurljob.h>
#include <messagecore/attachmentpropertiesdialog.h>
#include <messageviewer/editorwatcher.h>

using namespace KMail;
using namespace KPIM;
using namespace MailCommon;
using namespace MessageCore;

AttachmentController::AttachmentController( Message::AttachmentModel *model, AttachmentView *view, KMComposeWin *composer )
  : AttachmentControllerBase( model, composer, composer->actionCollection() )
{
  mComposer = composer;

  connect( composer, SIGNAL(identityChanged(KPIMIdentities::Identity)),
      this, SLOT(identityChanged()) );

  mView = view;
  connect( view, SIGNAL(contextMenuRequested()), this, SLOT(showContextMenu()) );
  connect( view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
      this, SLOT(selectionChanged()) );
  connect( view, SIGNAL(doubleClicked(QModelIndex)),
      this, SLOT(editSelectedAttachment()) );

  connect( this, SIGNAL(refreshSelection()), SLOT(selectionChanged()));

  connect( this, SIGNAL(showAttachment(KMime::Content*,QByteArray)),
           SLOT(onShowAttachment(KMime::Content*,QByteArray)));

  connect( model, SIGNAL(attachItemsRequester(Akonadi::Item::List ) ), this, SLOT( addAttachmentItems( Akonadi::Item::List ) ) );

}

AttachmentController::~AttachmentController()
{
//   delete d;
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
  kDebug() << identity.identityName();
  exportPublicKey( mComposer->identity().pgpEncryptionKey() );
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
  connect( itemFetchJob, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchJob( KJob* ) ) );
}

void AttachmentController::slotFetchJob( KJob *job )
{
  if ( job->error() ) {
    MailCommon::Util::showJobErrorMessage( job );
    return;
  }
  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  if ( !fjob )
    return;
  Akonadi::Item::List items = fjob->items();

  if ( items.isEmpty() )
    return;

  if ( items.first().mimeType() == KMime::Message::mimeType() ) {
    uint identity = 0;
    if ( items.at( 0 ).isValid() && items.at( 0 ).parentCollection().isValid() ) {
      QSharedPointer<FolderCollection> fd( FolderCollection::forCollection( items.at( 0 ).parentCollection() ) );
      identity = fd->identity();
    }
    KMCommand *command = new KMForwardAttachedCommand( mComposer, items,identity, mComposer );
    command->start();
  } else {
    foreach ( const Akonadi::Item &item, items ) {
      QString attachmentName = QLatin1String( "attachment" );
      if ( item.hasPayload<KABC::Addressee>() ) {
        const KABC::Addressee contact = item.payload<KABC::Addressee>();
        attachmentName = contact.realName() + QLatin1String( ".vcf" );
      }

      mComposer->addAttachment( attachmentName, KMime::Headers::CEbase64, QString(), item.payloadData(), item.mimeType().toLatin1() );
    }
  }
}

void AttachmentController::selectionChanged()
{
  const QModelIndexList selectedRows = mView->selectionModel()->selectedRows();
  AttachmentPart::List selectedParts;
  foreach( const QModelIndex &index, selectedRows ) {
    AttachmentPart::Ptr part = mView->model()->data(
        index, Message::AttachmentModel::AttachmentPartRole ).value<AttachmentPart::Ptr>();
    selectedParts.append( part );
  }
  setSelectedParts( selectedParts );
}

void AttachmentController::onShowAttachment( KMime::Content *content, const QByteArray &charset )
{
  KMReaderMainWin *win =
    new KMReaderMainWin( content, false, charset );
  win->show();
}

#include "attachmentcontroller.moc"
