
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmmimeparttree.h"

#include "kmreaderwin.h"
#include "partNode.h"
#include "kmmsgpart.h"
#include "kmkernel.h"
#include "kmcommands.h"
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
    setLineWidth(0); // don't draw ugly frame
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
    // TODO: remove this member var?
    mCurrentContextMenuItem = dynamic_cast<KMMimePartTreeItem*>( item );
    if ( 0 == mCurrentContextMenuItem ) {
        kdDebug(5006) << "Item was not a KMMimePartTreeItem!" << endl;
    }
    else {
        kdDebug(5006) << "\n**\n** KMMimePartTree::itemRightClicked() **\n**" << endl;

        QPopupMenu* popup = new QPopupMenu;
        popup->insertItem( i18n( "Save &As..." ), this, SLOT( slotSaveAs() ) );
        popup->insertItem( i18n( "Save as &Encoded..." ), this,
                           SLOT( slotSaveAsEncoded() ) );
        popup->insertItem( i18n( "Save All Attachments..." ), this,
                           SLOT( slotSaveAll() ) );
        popup->exec( point );
        delete popup;
        mCurrentContextMenuItem = 0;
    }
}

void KMMimePartTree::slotSaveAs()
{
    QPtrList<QListViewItem> selected = selectedItems();

    Q_ASSERT( !selected.isEmpty() );
    if ( selected.isEmpty() )
        return;
    if ( selected.count() == 1 )
        saveOneFile( selected.first(), false );
    else
        saveMultipleFiles( selected, false );
}

void KMMimePartTree::slotSaveAsEncoded()
{
    QPtrList<QListViewItem> selected = selectedItems();

    Q_ASSERT( !selected.isEmpty() );
    if ( selected.isEmpty() )
        return;
    if ( selected.count() == 1 )
        saveOneFile( selected.first(), true );
    else
        saveMultipleFiles( selected, true );
}

void KMMimePartTree::slotSaveAll()
{
    if( childCount() == 0)
        return;

    QPtrList<QListViewItem> items;
    for ( QListViewItemIterator lit( firstChild() ); lit.current();  ++lit ) {
        KMMimePartTreeItem *item = static_cast<KMMimePartTreeItem*>( lit.current() );
        items.append( item );
    }

    saveMultipleFiles( items, false );
}

void KMMimePartTree::saveOneFile( QListViewItem* item, bool encoded )
{
    QPtrList<partNode> parts;
    parts.append( static_cast<KMMimePartTreeItem *>(item)->node() );
    mReaderWin->setUpdateAttachment();
    KMSaveAttachmentsCommand *command = new KMSaveAttachmentsCommand( this, parts, 
            mReaderWin->message(), encoded );
    command->start();
}

void KMMimePartTree::saveMultipleFiles( const QPtrList<QListViewItem>& selected, bool encoded )
{
    QPtrListIterator<QListViewItem> it( selected );
    QPtrList<partNode> parts;
    while ( it.current() )
    {
        parts.append( static_cast<KMMimePartTreeItem *>(it.current())->node() );
        ++it;
    }
    mReaderWin->setUpdateAttachment();
    KMSaveAttachmentsCommand *command = new KMSaveAttachmentsCommand( this, parts, 
            mReaderWin->message(), encoded );
    command->start();
}


KMMimePartTreeItem::KMMimePartTreeItem( KMMimePartTree * parent,
                                        partNode* node,
                                        const QString & description,
                                        const QString & mimetype,
                                        const QString & encoding,
                                        KIO::filesize_t size )
  : QListViewItem( parent, description,
		   QString::null, // set by setIconAndTextForType()
		   encoding,
		   KIO::convertSize( size ) ),
    mPartNode( node )
{
  if( node )
    node->setMimePartTreeItem( this );
  setIconAndTextForType( mimetype );
}

KMMimePartTreeItem::KMMimePartTreeItem( KMMimePartTreeItem * parent,
                                        partNode* node,
                                        const QString & description,
                                        const QString & mimetype,
                                        const QString & encoding,
                                        KIO::filesize_t size,
                                        bool revertOrder )
  : QListViewItem( parent, description,
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
  } else if ( mimetype == "application/octet-stream" ) {
    setText( 1, i18n("Unspecified Binary Data") ); // don't show "Unknown"...
    setPixmap( 0, SmallIcon("unknown") );
  } else {
    KMimeType::Ptr mtp = KMimeType::mimeType( mimetype );
    setText( 1, mtp ? mtp->comment() : mimetype );
    setPixmap( 0, mtp ? mtp->pixmap( KIcon::Small) : SmallIcon("unknown") );
  }
}



#include "kmmimeparttree.moc"
