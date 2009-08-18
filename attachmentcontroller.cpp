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

#include "attachmentmodel.h"
#include "attachmentview.h"
#include "attachmentfrompublickeyjob.h"
#include "editorwatcher.h"
#include "globalsettings.h"
#include "kmkernel.h"
#include "newcomposerwin.h"

#include <QMenu>
#include <QPointer>

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KEncodingFileDialog>
#include <KFileDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMimeTypeTrader>
#include <KPushButton>
#include <KRun>
#include <KTemporaryFile>

#include <libkleo/kleo/cryptobackendfactory.h>
#include <libkleo/ui/keyselectiondialog.h>

#include <libkdepim/attachmentcompressjob.h>
#include <libkdepim/attachmentfrommimecontentjob.h>
#include <libkdepim/attachmentfromurljob.h>
#include <libkdepim/attachmentpropertiesdialog.h>

using namespace KMail;
using namespace KPIM;

class KMail::AttachmentController::Private
{
  public:
    Private( AttachmentController *qq );
    ~Private();

    void identityChanged(); // slot
    void selectionChanged(); // slot
    void attachmentRemoved( AttachmentPart::Ptr part ); // slot
    void compressJobResult( KJob *job ); // slot
    void loadJobResult( KJob *job ); // slot
    void openSelectedAttachments(); // slot
    void viewSelectedAttachments(); // slot
    void editSelectedAttachment(); // slot
    void editSelectedAttachmentWith(); // slot
    void removeSelectedAttachments(); // slot
    void saveSelectedAttachmentAs(); // slot
    void selectedAttachmentProperties(); // slot
    void editDone( KMail::EditorWatcher *watcher ); // slot
    void exportPublicKey( const QString &fingerprint );
    void attachPublicKeyJobResult( KJob *job ); // slot

    AttachmentController *const q;
    bool encryptEnabled;
    bool signEnabled;
    AttachmentModel *model;
    AttachmentView *view;
    KMComposeWin *composer;
    QHash<EditorWatcher*,AttachmentPart::Ptr> editorPart;
    QHash<EditorWatcher*,KTemporaryFile*> editorTempFile;

    QMenu *contextMenu;
    AttachmentPart::List selectedParts;
    QAction *attachPublicKeyAction;
    QAction *attachMyPublicKeyAction;
    QAction *openContextAction;
    QAction *viewContextAction;
    QAction *editContextAction;
    QAction *editWithContextAction;
    QAction *removeAction;
    QAction *removeContextAction;
    QAction *saveAsAction;
    QAction *saveAsContextAction;
    QAction *propertiesAction;
    QAction *propertiesContextAction;
    QAction *addAction;
    QAction *addContextAction;

    // If part p is compressed, uncompressedParts[p] is the uncompressed part.
    QHash<AttachmentPart::Ptr, AttachmentPart::Ptr> uncompressedParts;
};

AttachmentController::Private::Private( AttachmentController *qq )
  : q( qq )
  , encryptEnabled( false )
  , signEnabled( false )
  , model( 0 )
  , view( 0 )
  , composer( 0 )
  , contextMenu( 0 )
  , attachPublicKeyAction( 0 )
  , attachMyPublicKeyAction( 0 )
  , openContextAction( 0 )
  , viewContextAction( 0 )
  , editContextAction( 0 )
  , editWithContextAction( 0 )
  , removeAction( 0 )
  , removeContextAction( 0 )
  , saveAsAction( 0 )
  , saveAsContextAction( 0 )
  , propertiesAction( 0 )
  , propertiesContextAction( 0 )
  , addAction( 0 )
  , addContextAction( 0 )
{
}

AttachmentController::Private::~Private()
{
}

void AttachmentController::Private::identityChanged()
{
  const KPIMIdentities::Identity &identity = composer->identity();

  // "Attach public key" is only possible if OpenPGP support is available:
  attachPublicKeyAction->setEnabled(
      Kleo::CryptoBackendFactory::instance()->openpgp() );

  // "Attach my public key" is only possible if OpenPGP support is
  // available and the user specified his key for the current identity:
  attachMyPublicKeyAction->setEnabled(
      Kleo::CryptoBackendFactory::instance()->openpgp() && !identity.pgpEncryptionKey().isEmpty() );
}

void AttachmentController::Private::selectionChanged()
{
  QModelIndexList selectedRows = view->selectedSourceRows();
  selectedParts.clear();
  foreach( const QModelIndex &index, selectedRows ) {
    selectedParts.append( model->attachment( index ) );
  }
  const int selectedCount = selectedParts.count();

  openContextAction->setEnabled( selectedCount > 0 );
  viewContextAction->setEnabled( selectedCount > 0 );
  editContextAction->setEnabled( selectedCount == 1 );
  editWithContextAction->setEnabled( selectedCount == 1 );
  removeAction->setEnabled( selectedCount > 0 );
  removeContextAction->setEnabled( selectedCount > 0 );
  saveAsAction->setEnabled( selectedCount == 1 );
  saveAsContextAction->setEnabled( selectedCount == 1 );
  propertiesAction->setEnabled( selectedCount == 1 );
  propertiesContextAction->setEnabled( selectedCount == 1 );
}

void AttachmentController::Private::attachmentRemoved( AttachmentPart::Ptr part )
{
  if( uncompressedParts.contains( part ) ) {
    uncompressedParts.remove( part );
  }
}

void AttachmentController::Private::compressJobResult( KJob *job )
{
  if( job->error() ) {
    KMessageBox::sorry( composer, job->errorString(), i18n( "Failed to compress attachment" ) );
    return;
  }

  Q_ASSERT( dynamic_cast<AttachmentCompressJob*>( job ) );
  AttachmentCompressJob *ajob = static_cast<AttachmentCompressJob*>( job );
  //AttachmentPart *originalPart = const_cast<AttachmentPart*>( ajob->originalPart() );
  AttachmentPart::Ptr originalPart = ajob->originalPart();
  AttachmentPart::Ptr compressedPart = ajob->compressedPart();

  if( ajob->isCompressedPartLarger() ) {
    const int result = KMessageBox::questionYesNo( composer,
          i18n( "The compressed attachment is larger than the original. "
                "Do you want to keep the original one?" ),
          QString( /*caption*/ ),
          KGuiItem( i18nc( "Do not compress", "Keep" ) ),
          KGuiItem( i18n( "Compress" ) ) );
    if( result == KMessageBox::Yes ) {
      // The user has chosen to keep the uncompressed file.
      return;
    }
  }

  kDebug() << "Replacing uncompressed part in model.";
  uncompressedParts[ compressedPart ] = originalPart;
  bool ok = model->replaceAttachment( originalPart, compressedPart );
  if( !ok ) {
    // The attachment was removed from the model while we were compressing.
    kDebug() << "Compressed a zombie.";
  }
}

void AttachmentController::Private::loadJobResult( KJob *job )
{
  if( job->error() ) {
    KMessageBox::sorry( composer, job->errorString(), i18n( "Failed to attach file" ) );
    return;
  }

  Q_ASSERT( dynamic_cast<AttachmentFromUrlJob*>( job ) );
  AttachmentFromUrlJob *ajob = static_cast<AttachmentFromUrlJob*>( job );
  AttachmentPart::Ptr part = ajob->attachmentPart();
  part->setEncrypted( model->isEncryptSelected() );
  part->setSigned( model->isSignSelected() );
  model->addAttachment( part );
}

void AttachmentController::Private::openSelectedAttachments()
{
  Q_ASSERT( selectedParts.count() >= 1 );
  foreach( AttachmentPart::Ptr part, selectedParts ) {
    q->openAttachment( part );
  }
}

void AttachmentController::Private::viewSelectedAttachments()
{
  Q_ASSERT( selectedParts.count() >= 1 );
  foreach( AttachmentPart::Ptr part, selectedParts ) {
    q->viewAttachment( part );
  }
}

void AttachmentController::Private::editSelectedAttachment()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->editAttachment( selectedParts.first(), false /*openWith*/ );
  // TODO nicer api would be enum { OpenWithDialog, NoOpenWithDialog }
}

void AttachmentController::Private::editSelectedAttachmentWith()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->editAttachment( selectedParts.first(), true /*openWith*/ );
}

void AttachmentController::Private::removeSelectedAttachments()
{
  Q_ASSERT( selectedParts.count() >= 1 );
  foreach( AttachmentPart::Ptr part, selectedParts ) {
    model->removeAttachment( part );
  }
}

void AttachmentController::Private::saveSelectedAttachmentAs()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->saveAttachmentAs( selectedParts.first() );
}

void AttachmentController::Private::selectedAttachmentProperties()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->attachmentProperties( selectedParts.first() );
}

void AttachmentController::Private::editDone( EditorWatcher *watcher )
{
  AttachmentPart::Ptr part = editorPart.take( watcher );
  Q_ASSERT( part );
  KTemporaryFile *tempFile = editorTempFile.take( watcher );
  Q_ASSERT( tempFile );

  if( watcher->fileChanged() ) {
    kDebug() << "File has changed.";

    // Read the new data and update the part in the model.
    tempFile->reset();
    QByteArray data = tempFile->readAll();
    part->setData( data );
    model->updateAttachment( part );
  }

  delete tempFile;
  // The watcher deletes itself.
}

void AttachmentController::Private::exportPublicKey( const QString &fingerprint )
{
  if( fingerprint.isEmpty() || !Kleo::CryptoBackendFactory::instance()->openpgp() ) {
    kWarning() << "Tried to export key with empty fingerprint, or no OpenPGP.";
    Q_ASSERT( false ); // Can this happen?
    return;
  }

  AttachmentFromPublicKeyJob *ajob = new AttachmentFromPublicKeyJob( fingerprint, q );
  connect( ajob, SIGNAL(result(KJob*)), q, SLOT(attachPublicKeyJobResult(KJob*)) );
  ajob->start();
}

void AttachmentController::Private::attachPublicKeyJobResult( KJob *job )
{
  if( job->error() ) {
    KMessageBox::sorry( composer, job->errorString(), i18n( "Failed to attach public key" ) );
    return;
  }

  Q_ASSERT( dynamic_cast<AttachmentFromPublicKeyJob*>( job ) );
  AttachmentFromPublicKeyJob *ajob = static_cast<AttachmentFromPublicKeyJob*>( job );
  AttachmentPart::Ptr part = ajob->attachmentPart();
  model->addAttachment( part );
}



static KTemporaryFile *dumpAttachmentToTempFile( const AttachmentPart::Ptr part ) // local
{
  KTemporaryFile *file = new KTemporaryFile;
  if( !file->open() ) {
    kError() << "Could not open tempfile" << file->fileName();
    delete file;
    return 0;
  }
  if( file->write( part->data() ) == -1 ) {
    kError() << "Could not dump attachment to tempfile.";
    delete file;
    return 0;
  }
  file->flush();
  return file;
}



AttachmentController::AttachmentController( AttachmentModel *model, AttachmentView *view, KMComposeWin *composer )
  : QObject( view )
  , d( new Private( this ) )
{
  d->view = view;
  connect( view, SIGNAL(contextMenuRequested()), this, SLOT(showContextMenu()) );
  connect( view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
      this, SLOT(selectionChanged()) );

  d->model = model;
  connect( model, SIGNAL(attachUrlsRequested(KUrl::List)), this, SLOT(addAttachments(KUrl::List)) );
  connect( model, SIGNAL(attachmentRemoved(KPIM::AttachmentPart::Ptr)),
      this, SLOT(attachmentRemoved(KPIM::AttachmentPart::Ptr)) );
  connect( model, SIGNAL(attachmentCompressRequested(KPIM::AttachmentPart::Ptr,bool)),
      this, SLOT(compressAttachment(KPIM::AttachmentPart::Ptr,bool)) );
  connect( model, SIGNAL(encryptEnabled(bool)), this, SLOT(setEncryptEnabled(bool)) );
  connect( model, SIGNAL(signEnabled(bool)), this, SLOT(setSignEnabled(bool)) );

  d->composer = composer;
  connect( composer, SIGNAL(identityChanged(KPIMIdentities::Identity)),
      this, SLOT(identityChanged()) );
}

AttachmentController::~AttachmentController()
{
  delete d;
}

void AttachmentController::createActions()
{
  // Create the actions.
  d->attachPublicKeyAction = new KAction( i18n( "Attach &Public Key..." ), this );
  connect( d->attachPublicKeyAction, SIGNAL(triggered(bool)),
      this, SLOT(showAttachPublicKeyDialog()) );

  d->attachMyPublicKeyAction = new KAction( i18n( "Attach &My Public Key" ), this );
  connect( d->attachMyPublicKeyAction, SIGNAL(triggered(bool)), this, SLOT(attachMyPublicKey()) );

  d->addAction = new KAction( KIcon( "mail-attachment" ), i18n( "&Attach File..." ), this );
  d->addAction->setIconText( i18n( "Attach" ) );
  d->addContextAction = new KAction( KIcon( "mail-attachment" ),
      i18n( "Add Attachment..." ), this );
  connect( d->addAction, SIGNAL(triggered(bool)), this, SLOT(showAddAttachmentDialog()) );
  connect( d->addContextAction, SIGNAL(triggered(bool)), this, SLOT(showAddAttachmentDialog()) );

  d->removeAction = new KAction( i18n( "&Remove Attachment" ), this );
  d->removeContextAction = new KAction( i18n( "Remove" ), this ); // FIXME need two texts. is there a better way?
  connect( d->removeAction, SIGNAL(triggered(bool)), this, SLOT(removeSelectedAttachments()) );
  connect( d->removeContextAction, SIGNAL(triggered(bool)), this, SLOT(removeSelectedAttachments()) );

  d->openContextAction = new KAction( i18nc( "to open", "Open" ), this );
  connect( d->openContextAction, SIGNAL(triggered(bool)), this, SLOT(openSelectedAttachments()) );

  d->viewContextAction = new KAction( i18nc( "to view", "View" ), this );
  connect( d->viewContextAction, SIGNAL(triggered(bool)), this, SLOT(viewSelectedAttachments()) );

  d->editContextAction = new KAction( i18nc( "to edit", "Edit" ), this );
  connect( d->editContextAction, SIGNAL(triggered(bool)), this, SLOT(editSelectedAttachment()) );

  d->editWithContextAction = new KAction( i18n( "Edit With..." ), this );
  connect( d->editWithContextAction, SIGNAL(triggered(bool)), this, SLOT(editSelectedAttachmentWith()) );

  d->saveAsAction = new KAction( KIcon( "document-save-as"),
                                 i18n( "&Save Attachment As..." ), this );
  d->saveAsContextAction = new KAction( KIcon( "document-save-as"),
                                        i18n( "Save As..." ), this );
  connect( d->saveAsAction, SIGNAL(triggered(bool)),
      this, SLOT(saveSelectedAttachmentAs()) );
  connect( d->saveAsContextAction, SIGNAL(triggered(bool)),
      this, SLOT(saveSelectedAttachmentAs()) );

  d->propertiesAction = new KAction( i18n( "Attachment Pr&operties" ), this ); // TODO why no '...'?
  d->propertiesContextAction = new KAction( i18n( "Properties" ), this );
  connect( d->propertiesAction, SIGNAL(triggered(bool)),
      this, SLOT(selectedAttachmentProperties()) );
  connect( d->propertiesContextAction, SIGNAL(triggered(bool)),
      this, SLOT(selectedAttachmentProperties()) );

  // Create a context menu for the attachment view.
  Q_ASSERT( d->contextMenu == 0 ); // Not called twice.
  d->contextMenu = new QMenu( d->composer );
  d->contextMenu->addAction( d->openContextAction );
  d->contextMenu->addAction( d->viewContextAction );
  d->contextMenu->addAction( d->editContextAction );
  d->contextMenu->addAction( d->editWithContextAction );
  d->contextMenu->addAction( d->removeContextAction );
  d->contextMenu->addAction( d->saveAsContextAction );
  d->contextMenu->addAction( d->propertiesContextAction );
  d->contextMenu->addSeparator();
  d->contextMenu->addAction( d->addContextAction );

  // Insert the actions into the composer window's menu.
  KActionCollection *collection = d->composer->actionCollection();
  collection->addAction( "attach_public_key", d->attachPublicKeyAction );
  collection->addAction( "attach_my_public_key", d->attachMyPublicKeyAction );
  collection->addAction( "attach", d->addAction );
  collection->addAction( "remove", d->removeAction );
  collection->addAction( "attach_save", d->saveAsAction );
  collection->addAction( "attach_properties", d->propertiesAction );

  // Disable public key actions if appropriate.
  d->identityChanged();

  // Disable actions like 'Remove', since nothing is currently selected.
  d->selectionChanged();
}

void AttachmentController::setEncryptEnabled( bool enabled )
{
  d->encryptEnabled = enabled;
}

void AttachmentController::setSignEnabled( bool enabled )
{
  d->signEnabled = enabled;
}

void AttachmentController::compressAttachment( AttachmentPart::Ptr part, bool compress )
{
  if( compress ) {
    kDebug() << "Compressing part.";

    AttachmentCompressJob *ajob = new AttachmentCompressJob( part, this );
    connect( ajob, SIGNAL(result(KJob*)), this, SLOT( compressJobResult(KJob*)) );
    ajob->start();
  } else {
    kDebug() << "Uncompressing part.";

    // Replace the compressed part with the original uncompressed part, and delete
    // the compressed part.
    AttachmentPart::Ptr originalPart = d->uncompressedParts.take( part );
    Q_ASSERT( originalPart ); // Found in uncompressedParts.
    bool ok = d->model->replaceAttachment( part, originalPart );
    Q_ASSERT( ok );
  }
}

void AttachmentController::showContextMenu()
{
  d->selectionChanged();
  d->contextMenu->popup( QCursor::pos() );
}

void AttachmentController::openAttachment( AttachmentPart::Ptr part )
{
  KTemporaryFile *tempFile = dumpAttachmentToTempFile( part );
  if( !tempFile ) {
    KMessageBox::sorry( d->composer,
         i18n( "KMail was unable to write the attachment to a temporary file." ),
         i18n( "Unable to open attachment" ) );
    return;
  }

  bool success = KRun::runUrl( KUrl::fromPath( tempFile->fileName() ),
                               part->mimeType(),
                               d->composer,
                               true /*tempFile*/,
                               false /*runExecutables*/ );
  if( !success ) {
    if( KMimeTypeTrader::self()->preferredService( part->mimeType() ).isNull() ) {
      // KRun showed an Open-With dialog, and it was canceled.
    } else {
      // KRun failed.
      KMessageBox::sorry( d->composer,
           i18n( "KMail was unable to open the attachment." ),
           i18n( "Unable to open attachment" ) );
    }
    delete tempFile;
    tempFile = 0;
  } else {
    // The file was opened.  Delete it only when the composer is closed
    // (and this object is destroyed).
    tempFile->setParent( this ); // Manages lifetime.
  }
}

void AttachmentController::viewAttachment( AttachmentPart::Ptr part )
{
  QString pname;
  pname = part->name().trimmed();
  if( pname.isEmpty() ) {
    pname = part->description();
  }
  if( pname.isEmpty() ) {
    pname = QString::fromLatin1( "unnamed" );
  }

  kDebug() << "port me"; // TODO
#if 0
  KTemporaryFile *atmTempFile = new KTemporaryFile();
  atmTempFile->open();
  mAtmTempList.append( atmTempFile );
  KPIMUtils::kByteArrayToFile( msgPart->bodyDecodedBinary(), atmTempFile->fileName(),
                               false, false, false );
  KMReaderMainWin *win =
    new KMReaderMainWin( msgPart, false, atmTempFile->fileName(), pname, mCharset );
  win->show();
#endif
}

void AttachmentController::editAttachment( AttachmentPart::Ptr part, bool openWith )
{
  KTemporaryFile *tempFile = dumpAttachmentToTempFile( part );
  if( !tempFile ) {
    KMessageBox::sorry( d->composer,
         i18n( "KMail was unable to write the attachment to a temporary file." ),
         i18n( "Unable to edit attachment" ) );
    return;
  }

  EditorWatcher *watcher = new KMail::EditorWatcher(
      KUrl::fromPath( tempFile->fileName() ),
      part->mimeType(), openWith,
      this, d->composer );
  connect( watcher, SIGNAL(editDone(KMail::EditorWatcher*)),
           this, SLOT(editDone(KMail::EditorWatcher*)) );

  if( watcher->start() ) {
    // The attachment is being edited.
    // We will clean things up in editDone().
    d->editorPart[ watcher ] = part;
    d->editorTempFile[ watcher ] = tempFile;

    // Delete the temp file if the composer is closed (and this object is destroyed).
    tempFile->setParent( this ); // Manages lifetime.
  } else {
    kWarning() << "Could not start EditorWatcher.";
    delete watcher;
    delete tempFile;
  }
}

void AttachmentController::editAttachmentWith( AttachmentPart::Ptr part )
{
  editAttachment( part, true );
}

void AttachmentController::saveAttachmentAs( AttachmentPart::Ptr part )
{
  QString pname = part->name();
  if( pname.isEmpty() ) {
    pname = "unnamed";
  }

  KUrl url = KFileDialog::getSaveUrl( KUrl( /*startDir*/ ),
      QString( /*filter*/ ), d->composer,
      i18n( "Save Attachment As" ) );

  if( url.isEmpty() ) {
    kDebug() << "Save Attachment As dialog canceled.";
    return;
  }

  KMKernel::self()->byteArrayToRemoteFile( part->data(), url );
}

void AttachmentController::attachmentProperties( AttachmentPart::Ptr part )
{
  QPointer<AttachmentPropertiesDialog> dialog = new AttachmentPropertiesDialog(
      part, d->composer );

  dialog->setEncryptEnabled( d->encryptEnabled );
  dialog->setSignEnabled( d->signEnabled );

  if( dialog->exec() ) {
    d->model->updateAttachment( part );
  }
  delete dialog;
}

void AttachmentController::showAddAttachmentDialog()
{
  QPointer<KEncodingFileDialog> dialog = new KEncodingFileDialog(
      QString( /*startDir*/ ), QString( /*encoding*/ ), QString( /*filter*/ ),
      i18n( "Attach File" ), KFileDialog::Other, d->composer );

  dialog->okButton()->setGuiItem( KGuiItem( i18n("&Attach"), "document-open") );
  dialog->setMode( KFile::Files );
  if( dialog->exec() == KDialog::Accepted ) {
    const KUrl::List files = dialog->selectedUrls();
    foreach( const KUrl &url, files ) {
      KUrl urlWithEncoding = url;
      urlWithEncoding.setFileEncoding( dialog->selectedEncoding() );
      addAttachment( urlWithEncoding );
    }
  }
  delete dialog;
}

void AttachmentController::addAttachment( const KUrl &url )
{
  AttachmentFromUrlJob *ajob = new AttachmentFromUrlJob( url, this );
  if( GlobalSettings::maximumAttachmentSize() > 0 ) {
    ajob->setMaximumAllowedSize( GlobalSettings::maximumAttachmentSize() * 1024 * 1024 );
  }
  connect( ajob, SIGNAL(result(KJob*)), this, SLOT(loadJobResult(KJob*)) );
  ajob->start();
}

void AttachmentController::addAttachments( const KUrl::List &urls )
{
  foreach( const KUrl &url, urls ) {
    addAttachment( url );
  }
}

void AttachmentController::showAttachPublicKeyDialog()
{
  using Kleo::KeySelectionDialog;
  QPointer<KeySelectionDialog> dialog = new KeySelectionDialog(
      i18n( "Attach Public OpenPGP Key" ),
      i18n( "Select the public key which should be attached." ),
			std::vector<GpgME::Key>(),
			KeySelectionDialog::PublicKeys|KeySelectionDialog::OpenPGPKeys,
			false /* no multi selection */,
      false /* no remember choice box */,
      d->composer, "attach public key selection dialog" );

  if( dialog->exec() == KDialog::Accepted ) {
    d->exportPublicKey( dialog->fingerprint() );
  }
  delete dialog;
}

void AttachmentController::attachMyPublicKey()
{
  const KPIMIdentities::Identity &identity = d->composer->identity();
  kDebug() << identity.identityName();
  d->exportPublicKey( d->composer->identity().pgpEncryptionKey() );
}

#include "attachmentcontroller.moc"
