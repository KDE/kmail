
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmmimeparttree.h"

#include "kmreaderwin.h"
#include "partNode.h"
#include "kmkernel.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;

#include <kdebug.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kiconloader.h>

#include <qheader.h>
#include <qpopupmenu.h>

KMMimePartTree::KMMimePartTree( KMReaderWin* readerWin,
                                QWidget* parent,
                                const char* name )
    : KListView(  parent, name ),
      mReaderWin( readerWin )
{
    addColumn( i18n("Description") );
    addColumn( i18n("Type") );
    addColumn( i18n("Encoding") );
    addColumn( i18n("Size") );
    setColumnAlignment( 3, Qt::AlignRight );

    restoreLayoutIfPresent();
    connect( this, SIGNAL( clicked( QListViewItem* ) ),
             this, SLOT( itemClicked( QListViewItem* ) ) );
    connect( this, SIGNAL( contextMenuRequested( QListViewItem*,
                                                 const QPoint&, int ) ),
             this, SLOT( itemRightClicked( QListViewItem*, const QPoint& ) ) );
    setSelectionMode( QListView::Extended );
    setRootIsDecorated( false );
    setAllColumnsShowFocus( true );
    setShowToolTips( true );
    setSorting(-1);
}


static const char configGroup[] = "MimePartTree";

KMMimePartTree::~KMMimePartTree() {
  saveLayout( KMKernel::config(), configGroup );
}


void KMMimePartTree::restoreLayoutIfPresent() {
  // first column: soaks up the rest of the space:
  setColumnWidthMode( 0, Manual );
  header()->setStretchEnabled( true, 0 );
  // rest of the columns:
  if ( KMKernel::config()->hasGroup( configGroup ) ) {
    // there is a saved layout. use it...
    restoreLayout( KMKernel::config(), configGroup );
    // and disable Maximum mode:
    for ( int i = 1 ; i < 4 ; ++i )
      setColumnWidthMode( i, Manual );
  } else {
    // columns grow with their contents:
    for ( int i = 1 ; i < 4 ; ++i )
      setColumnWidthMode( i, Maximum );
  }
}


void KMMimePartTree::itemClicked( QListViewItem* item )
{
    KMMimePartTreeItem* i = dynamic_cast<KMMimePartTreeItem*>( item );
    if ( 0 == i ) {
        kdDebug(5006) << "Item was not a KMMimePartTreeItem!" << endl;
    }
    else {
        kdDebug(5006) << "\n**\n** KMMimePartTree::itemClicked() **\n**" << endl;
        if( mReaderWin->mRootNode == i->node() )
          mReaderWin->update( true ); // Force update
        else {
          ObjectTreeParser otp( mReaderWin, 0, true );
	  otp.parseObjectTree( i->node() );
	}
    }
}


void KMMimePartTree::itemRightClicked( QListViewItem* item,
                                       const QPoint& point )
{
    mCurrentContextMenuItem = dynamic_cast<KMMimePartTreeItem*>( item );
    if ( 0 == mCurrentContextMenuItem ) {
        kdDebug(5006) << "Item was not a KMMimePartTreeItem!" << endl;
    }
    else {
        kdDebug(5006) << "\n**\n** KMMimePartTree::itemRightClicked() **\n**" << endl;
/*
        if( mReaderWin->mRootNode == i->node() )
          mReaderWin->setMsg(mReaderWin->mMsg, true); // Force update
        else
          mReaderWin->parseObjectTree( i->node(), true );
//        mReaderWin->parseObjectTree( mCurrentContextMenuItem->node(), true );
*/
        QPopupMenu* popup = new QPopupMenu;
        popup->insertItem( i18n( "Save &As..." ), this, SLOT( slotSaveAs() ) );
        popup->insertItem( i18n( "Save As &Encoded..." ), this,
                           SLOT( slotSaveAsEncoded() ) );
        popup->insertItem( i18n( "Save Selected Items..." ), this,
                           SLOT( slotSaveSelected() ) );
        popup->insertItem( i18n( "Save All..." ), this,
                           SLOT( slotSaveAll() ) );
        popup->exec( point );
        delete popup;
        //mReaderWin->parseObjectTree( mCurrentContextMenuItem->node(), true );
        mCurrentContextMenuItem = 0;
    }
}


void KMMimePartTree::slotSaveAs()
{
    if( mCurrentContextMenuItem ) {  // this should always be true
        QString s = mCurrentContextMenuItem->text(0);
        if( s.startsWith( "file: " ) )
            s = s.mid(6).stripWhiteSpace();
        else
            s = s.stripWhiteSpace();
        QString filename = KFileDialog::getSaveFileName( s,
                                                         QString::null,
                                                         this, QString::null );
        if( !filename.isEmpty() ) {
            if( QFile::exists( filename ) ) {
                if( KMessageBox::warningYesNo( this,
                                               i18n( "A file with this name already exists. Do you want to overwrite it?" ),
                                               i18n( "KMail Warning" ) ) ==
                    KMessageBox::No )
                    return;
            }
            slotSaveItem( mCurrentContextMenuItem, filename );
        }
    }
}

void KMMimePartTree::slotSaveAsEncoded()
{
    if( mCurrentContextMenuItem ) {  // this should always be true
        QString s = mCurrentContextMenuItem->text(0);
        if( s.startsWith( "file: " ) )
            s = s.mid(6).stripWhiteSpace();
        else
	    s = s.stripWhiteSpace();
        QString filename = KFileDialog::getSaveFileName( s,
                                                         QString::null,
                                                         this, QString::null );
        if( !filename.isEmpty() ) {
            if( QFile::exists( filename ) ) {
                if( KMessageBox::warningYesNo( this,
                                               i18n( "A file with this name already exists. Do you want to overwrite it?" ),
                                               i18n( "KMail Warning" ) ) ==
                    KMessageBox::No )
                    return;
            }

            // Note:   SaveAsEncoded *allways* stores the ENCRYPTED content with SIGNATURES.
            // reason: SaveAsEncoded does not decode the Message Content-Transfer-Encoding
            //         but saves the _original_ content of the message (or the message part, resp.)

            QFile file( filename );
            if( file.open( IO_WriteOnly ) ) {
                QDataStream ds( &file );
                QCString cstr( mCurrentContextMenuItem->node()->msgPart().body() );
                ds.writeRawBytes( cstr, cstr.length() );
                file.close();
            } else
                KMessageBox::error( this, i18n( "Could not write the file" ),
                                    i18n( "KMail Error" ) );
        }
    }
}

void KMMimePartTree::slotSaveItem( KMMimePartTreeItem* item, const QString& filename )
{
    if ( item && !filename.isEmpty() ) {
        bool bSaveEncrypted = false;
        bool bEncryptedParts = item->node()->encryptionState() != KMMsgNotEncrypted;
        if( bEncryptedParts )
            if( KMessageBox::questionYesNo( this,
                                            i18n( "This part of the message is encrypted. Do you want to keep the encryption when saving?" ),
                                            i18n( "KMail Question" ) ) ==
                KMessageBox::Yes )
                bSaveEncrypted = true;

        bool bSaveWithSig = true;
        if( item->node()->signatureState() != KMMsgNotSigned )
            if( KMessageBox::questionYesNo( this,
                                            i18n( "This part of the message is signed. Do you want to keep the signature when saving?" ),
                                            i18n( "KMail Question" ) ) !=
                KMessageBox::Yes )
                bSaveWithSig = false;

        QFile file( filename );
        if( file.open( IO_WriteOnly ) ) {
            QDataStream ds( &file );
            if( (bSaveEncrypted || !bEncryptedParts) && bSaveWithSig ) {
                QByteArray cstr = item->node()->msgPart().bodyDecodedBinary();
                ds.writeRawBytes( cstr, cstr.size() );
            } else {
                ObjectTreeParser otp( mReaderWin, 0, true,
                                      bSaveEncrypted, bSaveWithSig );
                otp.parseObjectTree( item->node() );
            }
            file.close();
        } else
            KMessageBox::error( this, i18n( "Could not write the file" ),
                                i18n( "KMail Error" ) );
    }
}

void KMMimePartTree::slotSaveAll()
{
    if( childCount() == 0)
        return;

    KFileDialog fdlg( QString::null, QString::null, this, 0, TRUE );
    fdlg.setMode( KFile::Directory );
    if (!fdlg.exec())
        return;

    QString dir = fdlg.selectedURL().path();
    QListViewItemIterator lit( firstChild() );
    for ( ; lit.current();  ) {
        KMMimePartTreeItem *item = static_cast<KMMimePartTreeItem*>( lit.current() );
        QString s = item->text(0);
        if( s.startsWith( "file: " ) )
            s = s.mid(6).stripWhiteSpace();
        else
            s = s.stripWhiteSpace();

        QString filename = dir + "/" + s;

        if( !filename.isEmpty() ) {
            if( QFile::exists( filename ) ) {
                if( KMessageBox::warningYesNo( this,
                                               i18n( "A file with this name already exists. Do you want to overwrite it?" ),
                                               i18n( "KMail Warning" ) ) ==
                    KMessageBox::No ) {
                    ++lit;
                    continue;
                }
            }
            slotSaveItem( item, filename );
        }
        ++lit;
    }
}

void KMMimePartTree::slotSaveSelected()
{
    QPtrList<QListViewItem> selected = selectedItems();

    Q_ASSERT( selected.count() > 0 );

    QPtrListIterator<QListViewItem> it( selected );
    KFileDialog fdlg( QString::null, QString::null, this, 0, TRUE );
    fdlg.setMode( KFile::Directory );
    if (!fdlg.exec()) return;
    QString dir = fdlg.selectedURL().path();

    while ( it.current() ) {
        KMMimePartTreeItem *item = static_cast<KMMimePartTreeItem*>( it.current() );
        QString s = item->text(0);
        if( s.startsWith( "file: " ) )
            s = s.mid(6).stripWhiteSpace();
        else
            s = s.stripWhiteSpace();

        QString filename = dir + "/" + s;

        if( !filename.isEmpty() ) {
            if( QFile::exists( filename ) ) {
                if( KMessageBox::warningYesNo( this,
                                               i18n( "A file with this name already exists. Do you want to overwrite it?" ),
                                               i18n( "KMail Warning" ) ) ==
                    KMessageBox::No ) {
                    ++it;
                    continue;
                }
            }
            slotSaveItem( item, filename );
        }
        ++it;
    }
}


KMMimePartTreeItem::KMMimePartTreeItem( KMMimePartTree& parent,
                                        partNode* node,
                                        const QString & description,
                                        const QString & mimetype,
                                        const QString & encoding,
                                        KIO::filesize_t size )
  : QListViewItem( &parent, description,
		   QString::null, // set by setIconAndTextForType()
		   encoding,
		   KIO::convertSize( size ) ),
    mPartNode( node )
{
  if( node )
    node->setMimePartTreeItem( this );
  setIconAndTextForType( mimetype );
}

KMMimePartTreeItem::KMMimePartTreeItem( KMMimePartTreeItem& parent,
                                        partNode* node,
                                        const QString & description,
                                        const QString & mimetype,
                                        const QString & encoding,
                                        KIO::filesize_t size,
                                        bool revertOrder )
  : QListViewItem( &parent, description,
		   QString::null, // set by setIconAndTextForType()
		   encoding,
		   KIO::convertSize( size ) ),
    mPartNode( node )
{
  if( revertOrder && nextSibling() ){
    QListViewItem* sib = nextSibling();
    while( sib->nextSibling() )
      sib = sib->nextSibling();
    moveItem( sib );
  }
  if( node )
    node->setMimePartTreeItem( this );
  setIconAndTextForType( mimetype );
}

void KMMimePartTreeItem::setIconAndTextForType( const QString & mime )
{
  QString mimetype = mime.lower();
  if ( mimetype.startsWith( "multipart/" ) ) {
    setText( 1, mimetype );
    setPixmap( 0, SmallIcon("folder") );
  } else {
    KMimeType::Ptr mtp = KMimeType::mimeType( mimetype );
    setText( 1, mtp ? mtp->comment() : mimetype );
    setPixmap( 0, mtp ? mtp->pixmap( KIcon::Small) : SmallIcon("unknown") );
  }
}



#include "kmmimeparttree.moc"
