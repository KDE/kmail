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

#include "attachmentcontrollerbase.h"

#include "messagecomposer/attachmentmodel.h"
#include "messagecomposer/attachmentfrompublickeyjob.h"
#include "messageviewer/editorwatcher.h"

#include <akonadi/itemfetchjob.h>
#include <kio/jobuidelegate.h>

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

#include "kmreadermainwin.h"
#include <kpimutils/kfileio.h>

#include <libkleo/kleo/cryptobackendfactory.h>
#include <libkleo/ui/keyselectiondialog.h>

#include <messagecore/attachmentcompressjob.h>
#include <messagecore/attachmentfrommimecontentjob.h>
#include <messagecore/attachmentfromurljob.h>
#include <messagecore/attachmentpropertiesdialog.h>
#include <messagecomposersettings.h>
#include <KIO/Job>

using namespace KMail;
using namespace KPIM;

class KMail::AttachmentControllerBase::Private
{
  public:
    Private( AttachmentControllerBase *qq );
    ~Private();

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
    void editDone( MessageViewer::EditorWatcher *watcher ); // slot
    void attachPublicKeyJobResult( KJob *job ); // slot
    void addAttachmentPart( AttachmentPart::Ptr part );

    AttachmentControllerBase *const q;
    bool encryptEnabled;
    bool signEnabled;
    Message::AttachmentModel *model;
    QWidget *wParent;
    QHash<MessageViewer::EditorWatcher*,AttachmentPart::Ptr> editorPart;
    QHash<MessageViewer::EditorWatcher*,KTemporaryFile*> editorTempFile;
    QList<KTemporaryFile*> mAttachmentTempList;

    QMenu *contextMenu;
    AttachmentPart::List selectedParts;
    KActionCollection *mActionCollection;
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

AttachmentControllerBase::Private::Private( AttachmentControllerBase *qq )
  : q( qq )
  , encryptEnabled( false )
  , signEnabled( false )
  , model( 0 )
  , wParent( 0 )
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

AttachmentControllerBase::Private::~Private()
{
  qDeleteAll( mAttachmentTempList );
}

void AttachmentControllerBase::setSelectedParts( const AttachmentPart::List &selectedParts)
{
  const int selectedCount = selectedParts.count();

  d->openContextAction->setEnabled( selectedCount > 0 );
  d->viewContextAction->setEnabled( selectedCount > 0 );
  d->editContextAction->setEnabled( selectedCount == 1 );
  d->editWithContextAction->setEnabled( selectedCount == 1 );
  d->removeAction->setEnabled( selectedCount > 0 );
  d->removeContextAction->setEnabled( selectedCount > 0 );
  d->saveAsAction->setEnabled( selectedCount == 1 );
  d->saveAsContextAction->setEnabled( selectedCount == 1 );
  d->propertiesAction->setEnabled( selectedCount == 1 );
  d->propertiesContextAction->setEnabled( selectedCount == 1 );
}

void AttachmentControllerBase::Private::attachmentRemoved( AttachmentPart::Ptr part )
{
  if( uncompressedParts.contains( part ) ) {
    uncompressedParts.remove( part );
  }
}

void AttachmentControllerBase::Private::compressJobResult( KJob *job )
{
  if( job->error() ) {
    KMessageBox::sorry( wParent, job->errorString(), i18n( "Failed to compress attachment" ) );
    return;
  }

  Q_ASSERT( dynamic_cast<AttachmentCompressJob*>( job ) );
  AttachmentCompressJob *ajob = static_cast<AttachmentCompressJob*>( job );
  //AttachmentPart *originalPart = const_cast<AttachmentPart*>( ajob->originalPart() );
  AttachmentPart::Ptr originalPart = ajob->originalPart();
  AttachmentPart::Ptr compressedPart = ajob->compressedPart();

  if( ajob->isCompressedPartLarger() ) {
    const int result = KMessageBox::questionYesNo( wParent,
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

void AttachmentControllerBase::Private::loadJobResult( KJob *job )
{
  if( job->error() ) {
    KMessageBox::sorry( wParent, job->errorString(), i18n( "Failed to attach file" ) );
    return;
  }

  Q_ASSERT( dynamic_cast<AttachmentFromUrlJob*>( job ) );
  AttachmentFromUrlJob *ajob = static_cast<AttachmentFromUrlJob*>( job );
  AttachmentPart::Ptr part = ajob->attachmentPart();
  q->addAttachment( part );
}

void AttachmentControllerBase::Private::openSelectedAttachments()
{
  Q_ASSERT( selectedParts.count() >= 1 );
  foreach( AttachmentPart::Ptr part, selectedParts ) {
    q->openAttachment( part );
  }
}

void AttachmentControllerBase::Private::viewSelectedAttachments()
{
  Q_ASSERT( selectedParts.count() >= 1 );
  foreach( AttachmentPart::Ptr part, selectedParts ) {
    q->viewAttachment( part );
  }
}

void AttachmentControllerBase::Private::editSelectedAttachment()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->editAttachment( selectedParts.first(), false /*openWith*/ );
  // TODO nicer api would be enum { OpenWithDialog, NoOpenWithDialog }
}

void AttachmentControllerBase::Private::editSelectedAttachmentWith()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->editAttachment( selectedParts.first(), true /*openWith*/ );
}

void AttachmentControllerBase::Private::removeSelectedAttachments()
{
  Q_ASSERT( selectedParts.count() >= 1 );
  foreach( AttachmentPart::Ptr part, selectedParts ) {
    model->removeAttachment( part );
  }
}

void AttachmentControllerBase::Private::saveSelectedAttachmentAs()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->saveAttachmentAs( selectedParts.first() );
}

void AttachmentControllerBase::Private::selectedAttachmentProperties()
{
  Q_ASSERT( selectedParts.count() == 1 );
  q->attachmentProperties( selectedParts.first() );
}

void AttachmentControllerBase::Private::editDone( MessageViewer::EditorWatcher *watcher )
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

void AttachmentControllerBase::exportPublicKey( const QString &fingerprint )
{
  if( fingerprint.isEmpty() || !Kleo::CryptoBackendFactory::instance()->openpgp() ) {
    kWarning() << "Tried to export key with empty fingerprint, or no OpenPGP.";
    Q_ASSERT( false ); // Can this happen?
    return;
  }

  Message::AttachmentFromPublicKeyJob *ajob = new Message::AttachmentFromPublicKeyJob( fingerprint, this );
  connect( ajob, SIGNAL(result(KJob*)), this, SLOT(attachPublicKeyJobResult(KJob*)) );
  ajob->start();
}

void AttachmentControllerBase::Private::attachPublicKeyJobResult( KJob *job )
{
  // The only reason we can't use loadJobResult() and need a separate method
  // is that we want to show the proper caption ("public key" instead of "file")...

  if( job->error() ) {
    KMessageBox::sorry( wParent, job->errorString(), i18n( "Failed to attach public key" ) );
    return;
  }

  Q_ASSERT( dynamic_cast<Message::AttachmentFromPublicKeyJob*>( job ) );
  Message::AttachmentFromPublicKeyJob *ajob = static_cast<Message::AttachmentFromPublicKeyJob*>( job );
  AttachmentPart::Ptr part = ajob->attachmentPart();
  q->addAttachment( part );
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



AttachmentControllerBase::AttachmentControllerBase( Message::AttachmentModel *model, QWidget *wParent, KActionCollection *actionCollection )
  : QObject( wParent )
  , d( new Private( this ) )
{
  d->model = model;
  connect( model, SIGNAL(attachUrlsRequested(KUrl::List)), this, SLOT(addAttachments(KUrl::List)) );
  connect( model, SIGNAL(attachItemsRequester(Akonadi::Item::List ) ), this, SLOT( addAttachmentItems( Akonadi::Item::List ) ) );
  connect( model, SIGNAL(attachmentRemoved(KPIM::AttachmentPart::Ptr)),
      this, SLOT(attachmentRemoved(KPIM::AttachmentPart::Ptr)) );
  connect( model, SIGNAL(attachmentCompressRequested(KPIM::AttachmentPart::Ptr,bool)),
      this, SLOT(compressAttachment(KPIM::AttachmentPart::Ptr,bool)) );
  connect( model, SIGNAL(encryptEnabled(bool)), this, SLOT(setEncryptEnabled(bool)) );
  connect( model, SIGNAL(signEnabled(bool)), this, SLOT(setSignEnabled(bool)) );

  d->wParent = wParent;
  d->mActionCollection = actionCollection;
}

AttachmentControllerBase::~AttachmentControllerBase()
{
  delete d;
}

void AttachmentControllerBase::createActions()
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
  d->contextMenu = new QMenu( d->wParent );
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
  KActionCollection *collection = d->mActionCollection;
  collection->addAction( "attach_public_key", d->attachPublicKeyAction );
  collection->addAction( "attach_my_public_key", d->attachMyPublicKeyAction );
  collection->addAction( "attach", d->addAction );
  collection->addAction( "remove", d->removeAction );
  collection->addAction( "attach_save", d->saveAsAction );
  collection->addAction( "attach_properties", d->propertiesAction );

  emit actionsCreated();
}

void AttachmentControllerBase::setEncryptEnabled( bool enabled )
{
  d->encryptEnabled = enabled;
}

void AttachmentControllerBase::setSignEnabled( bool enabled )
{
  d->signEnabled = enabled;
}

void AttachmentControllerBase::compressAttachment( AttachmentPart::Ptr part, bool compress )
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

void AttachmentControllerBase::showContextMenu()
{
  emit refreshSelection();
  d->contextMenu->popup( QCursor::pos() );
}

void AttachmentControllerBase::openAttachment( AttachmentPart::Ptr part )
{
  KTemporaryFile *tempFile = dumpAttachmentToTempFile( part );
  if( !tempFile ) {
    KMessageBox::sorry( d->wParent,
         i18n( "KMail was unable to write the attachment to a temporary file." ),
         i18n( "Unable to open attachment" ) );
    return;
  }

  bool success = KRun::runUrl( KUrl::fromPath( tempFile->fileName() ),
                               part->mimeType(),
                               d->wParent,
                               true /*tempFile*/,
                               false /*runExecutables*/ );
  if( !success ) {
    if( KMimeTypeTrader::self()->preferredService( part->mimeType() ).isNull() ) {
      // KRun showed an Open-With dialog, and it was canceled.
    } else {
      // KRun failed.
      KMessageBox::sorry( d->wParent,
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

void AttachmentControllerBase::viewAttachment( AttachmentPart::Ptr part )
{
  QString pname;
  pname = part->name().trimmed();
  if( pname.isEmpty() ) {
    pname = part->description();
  }
  if( pname.isEmpty() ) {
    pname = QString::fromLatin1( "unnamed" );
  }

  KTemporaryFile *atmTempFile = new KTemporaryFile();
  atmTempFile->open();
  d->mAttachmentTempList.append( atmTempFile );
  KPIMUtils::kByteArrayToFile( part->/*bodyDecodedBinary()*/data(), atmTempFile->fileName(),
                               false, false, false );
  KMime::Content *content = new KMime::Content;
  content->setContent( part->data() );
  KMReaderMainWin *win =
    new KMReaderMainWin( content, false, atmTempFile->fileName(), pname, part->charset() );
  win->show();
}

void AttachmentControllerBase::editAttachment( AttachmentPart::Ptr part, bool openWith )
{
  KTemporaryFile *tempFile = dumpAttachmentToTempFile( part );
  if( !tempFile ) {
    KMessageBox::sorry( d->wParent,
         i18n( "KMail was unable to write the attachment to a temporary file." ),
         i18n( "Unable to edit attachment" ) );
    return;
  }

  MessageViewer::EditorWatcher *watcher = new MessageViewer::EditorWatcher(
      KUrl::fromPath( tempFile->fileName() ),
      part->mimeType(), openWith,
      this, d->wParent );
  connect( watcher, SIGNAL(editDone(MessageViewer::EditorWatcher*)),
           this, SLOT(editDone(MessageViewer::EditorWatcher*)) );

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

void AttachmentControllerBase::editAttachmentWith( AttachmentPart::Ptr part )
{
  editAttachment( part, true );
}

void AttachmentControllerBase::saveAttachmentAs( AttachmentPart::Ptr part )
{
  QString pname = part->name();
  if( pname.isEmpty() ) {
    pname = i18n( "unnamed" );
  }

  KUrl url = KFileDialog::getSaveUrl( pname,
      QString( /*filter*/ ), d->wParent,
      i18n( "Save Attachment As" ) );

  if( url.isEmpty() ) {
    kDebug() << "Save Attachment As dialog canceled.";
    return;
  }

  byteArrayToRemoteFile(part->data(), url);
}

void AttachmentControllerBase::byteArrayToRemoteFile(const QByteArray &aData, const KUrl &aURL, bool overwrite)
{
  KIO::StoredTransferJob *job = KIO::storedPut( aData, aURL, -1, overwrite ? KIO::Overwrite : KIO::DefaultFlags );
  connect( job, SIGNAL(result(KJob *)), SLOT(slotPutResult(KJob *)) );
}

void AttachmentControllerBase::slotPutResult(KJob *job)
{
  KIO::StoredTransferJob *_job = qobject_cast<KIO::StoredTransferJob *>( job );

  if (job->error())
  {
    if (job->error() == KIO::ERR_FILE_ALREADY_EXIST)
    {
      if (KMessageBox::warningContinueCancel(0,
        i18n("File %1 exists.\nDo you want to replace it?", _job->url().toLocalFile()), i18n("Save to File"), KGuiItem(i18n("&Replace")))
        == KMessageBox::Continue)
        byteArrayToRemoteFile(_job->data(), _job->url(), true);
    }
    else {
      KIO::JobUiDelegate *ui = static_cast<KIO::Job*>( job )->ui();
      ui->showErrorMessage();
    }
  }
}

void AttachmentControllerBase::attachmentProperties( AttachmentPart::Ptr part )
{
  QPointer<AttachmentPropertiesDialog> dialog = new AttachmentPropertiesDialog(
      part, d->wParent );

  dialog->setEncryptEnabled( d->encryptEnabled );
  dialog->setSignEnabled( d->signEnabled );

  if( dialog->exec() ) {
    d->model->updateAttachment( part );
  }
  delete dialog;
}

void AttachmentControllerBase::showAddAttachmentDialog()
{
  QPointer<KEncodingFileDialog> dialog = new KEncodingFileDialog(
      QString( /*startDir*/ ), QString( /*encoding*/ ), QString( /*filter*/ ),
      i18n( "Attach File" ), KFileDialog::Other, d->wParent );

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

void AttachmentControllerBase::addAttachment( AttachmentPart::Ptr part )
{
  part->setEncrypted( d->model->isEncryptSelected() );
  part->setSigned( d->model->isSignSelected() );
  d->model->addAttachment( part );

  // TODO I can't find this setting in the config dialog. Has it been removed?
  if( MessageComposer::MessageComposerSettings::self()->showMessagePartDialogOnAttach() ) {
    attachmentProperties( part );
  }
}

void AttachmentControllerBase::addAttachment( const KUrl &url )
{
  AttachmentFromUrlJob *ajob = new AttachmentFromUrlJob( url, this );
  if( MessageComposer::MessageComposerSettings::maximumAttachmentSize() > 0 ) {
    ajob->setMaximumAllowedSize( MessageComposer::MessageComposerSettings::maximumAttachmentSize() * 1024 * 1024 );
  }
  connect( ajob, SIGNAL(result(KJob*)), this, SLOT(loadJobResult(KJob*)) );
  ajob->start();
}

void AttachmentControllerBase::addAttachments( const KUrl::List &urls )
{
  foreach( const KUrl &url, urls ) {
    addAttachment( url );
  }
}

void AttachmentControllerBase::showAttachPublicKeyDialog()
{
  using Kleo::KeySelectionDialog;
  QPointer<KeySelectionDialog> dialog = new KeySelectionDialog(
      i18n( "Attach Public OpenPGP Key" ),
      i18n( "Select the public key which should be attached." ),
			std::vector<GpgME::Key>(),
			KeySelectionDialog::PublicKeys|KeySelectionDialog::OpenPGPKeys,
			false /* no multi selection */,
      false /* no remember choice box */,
      d->wParent, "attach public key selection dialog" );

  if( dialog->exec() == KDialog::Accepted ) {
    exportPublicKey( dialog->fingerprint() );
  }
  delete dialog;
}

void AttachmentControllerBase::enableAttachPublicKey( bool enable )
{
  d->attachPublicKeyAction->setEnabled( enable );
}

void AttachmentControllerBase::enableAttachMyPublicKey( bool enable )
{
  d->attachMyPublicKeyAction->setEnabled( enable );
}

#include "attachmentcontrollerbase.moc"
