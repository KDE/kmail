/* -*- c++ -*-
    kmmimeparttree.cpp A MIME part tree viwer.

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "kmmimeparttree.h"

#include "kmreaderwin.h"
#include "partNode.h"
#include "kmmsgpart.h"
#include "kmkernel.h"
#include "kmcommands.h"

#include <kdebug.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kurl.h>
#include <kmenu.h>

#include <QClipboard>
#include <QHeaderView>

static const int columnDescription = 0;
static const int columnType = 1;
static const int columnEncoding = 2;
static const int columnSize = 3;


KMMimePartTree::KMMimePartTree( KMReaderWin* readerWin,
                                QWidget* parent )
  : KPIM::TreeWidget( parent ),
    mReaderWin( readerWin ), mLayoutColumnsOnFirstShow( false )
{
  // Setup the header
  addColumn( i18n( "Description" ) );
  addColumn( i18n( "Type" ) );
  addColumn( i18n( "Encoding" ) );
  addColumn( i18n( "Size" ), Qt::AlignRight );

  // Setup our mouse handlers
  connect( this, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ),
           this, SLOT( slotItemClicked( QTreeWidgetItem* ) ) );

  setContextMenuPolicy( Qt::CustomContextMenu );
  connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ),
           this, SLOT( slotContextMenuRequested( const QPoint& ) ) );

  // Setup view properties
  setSelectionMode( QTreeWidget::ExtendedSelection );
  setAlternatingRowColors( true );
  setRootIsDecorated( false );
  setAllColumnsShowFocus( true );
  // FIXME: With Qt4.3 there seems to be no appropriate substitute for setShowToolTips()
  //setShowToolTips( true );
  setDragEnabled( true );

  setSortingEnabled( true );
  header()->setClickable( true );
  header()->setSortIndicator( columnCount(), Qt::AscendingOrder );
  header()->setSortIndicatorShown( true );

  restoreLayoutIfPresent();
}

static const char configGroup[] = "MimePartTree";
static const char configEntry[] = "State";

KMMimePartTree::~KMMimePartTree()
{
  saveLayout( KMKernel::config(), configGroup, configEntry );
}

void KMMimePartTree::clearAndResetSortOrder()
{
  clear();
  // reset the sort indicator section to something outside the allowable
  // range in order to preserve insertion sort order.
  header()->setSortIndicator( columnCount(), Qt::AscendingOrder );
}

void KMMimePartTree::restoreLayoutIfPresent()
{
  if ( restoreLayout( KMKernel::config(), configGroup, configEntry ) )
    return;
  // No configGroup or no configEntry.
  // Provide nice defaults on first show
  mLayoutColumnsOnFirstShow = true;
}

void KMMimePartTree::showEvent( QShowEvent* e )
{
  if ( mLayoutColumnsOnFirstShow ) {
    // This seems to be the best way to provide reasonable defaults
    // for the column widths. We're triggered before the tree
    // is actually filled so fitting to contents is not an option (here).
    // Having a function called from outside seems to be an overkill.
    // QHeaderView::ResizeToContents can't be set since it disables
    // user resizing of columns...
    // So in the end, we provide heuristic defaults based on the width
    // of the widget the first time we're shown...
    int w = width();
    header()->resizeSection( columnDescription, ( w / 10 ) * 6 );
    header()->resizeSection( columnType, ( w / 10 ) * 2 );
    header()->resizeSection( columnEncoding, ( w / 10 ) );
    header()->resizeSection( columnSize, ( w / 10 ) );
    mLayoutColumnsOnFirstShow = false;
  }

  QTreeWidget::showEvent( e );
}

void KMMimePartTree::slotItemClicked( QTreeWidgetItem* item )
{
  if ( const KMMimePartTreeItem * i = dynamic_cast<KMMimePartTreeItem*>( item ) ) {
    // Display the clicked tree node in the reader window
    if ( mReaderWin->mRootNode == i->node() ) {
      const bool selected = i->isSelected();
      mReaderWin->update( true ); // Force update so the reader will display the whole message
      if ( selected ) {
        setCurrentItem( invisibleRootItem()->child( 0 ) );
        invisibleRootItem()->child( 0 )->setSelected( true );
      }
    }
    else
      mReaderWin->setMsgPart( i->node() ); // Show the message sub-part
  }
}

void KMMimePartTree::slotContextMenuRequested( const QPoint& p )
{
  KMMimePartTreeItem * item = dynamic_cast<KMMimePartTreeItem *>( itemAt( p ) );
  const bool isAttachment = item && ( item->node()->nodeId() > 2 ) &&
                      ( item->node()->typeString() != "Multipart" );
  const bool isRoot = item && mReaderWin->mRootNode == item->node();

  KMenu popup;

  if ( !isRoot ) {
    if ( item ) {
      popup.addAction( SmallIcon( "document-save-as" ), i18n( "Save &As..." ),
                     this, SLOT( slotSaveAs() ) );
    }

    if ( isAttachment ) {
      popup.addAction( SmallIcon( "document-open" ), i18nc( "to open", "Open" ),
                       this, SLOT( slotOpen() ) );
      popup.addAction( i18n( "Open With..." ), this, SLOT( slotOpenWith() ) );
      popup.addAction( i18nc( "to view something", "View" ), this, SLOT( slotView() ) );
    }
  }

  /*
   * FIXME make optional?
  popup.addAction( i18n( "Save as &Encoded..." ), this,
                   SLOT( slotSaveAsEncoded() ) );
  */

  popup.addAction( i18n( "Save All Attachments..." ), this,
                   SLOT( slotSaveAll() ) );

  // edit + delete only for attachments
  if ( !isRoot ) {
    if ( isAttachment ) {
      popup.addAction( SmallIcon( "edit-copy" ), i18n( "Copy" ),
                       this, SLOT( slotCopy() ) );
      if ( GlobalSettings::self()->allowAttachmentDeletion() )
        popup.addAction( SmallIcon( "edit-delete" ), i18n( "Delete Attachment" ),
                         this, SLOT( slotDelete() ) );
      if ( GlobalSettings::self()->allowAttachmentEditing() )
        popup.addAction( SmallIcon( "document-properties" ), i18n( "Edit Attachment" ),
                         this, SLOT( slotEdit() ) );
    }

    if ( item && item->node()->nodeId() > 0 )
      popup.addAction( i18n( "Properties" ), this, SLOT( slotProperties() ) );
  }
  popup.exec( viewport()->mapToGlobal( p ) );
}

//-----------------------------------------------------------------------------
void KMMimePartTree::slotSaveAs()
{
  saveSelectedBodyParts( false );
}

//-----------------------------------------------------------------------------
void KMMimePartTree::slotSaveAsEncoded()
{
  saveSelectedBodyParts( true );
}

//-----------------------------------------------------------------------------
void KMMimePartTree::saveSelectedBodyParts( bool encoded )
{
  QList<QTreeWidgetItem*> selected = selectedItems();

  Q_ASSERT( !selected.isEmpty() );
  if ( selected.isEmpty() )
    return;

  QList<partNode*> parts;
  for ( QList<QTreeWidgetItem*>::Iterator it = selected.begin(); it != selected.end(); ++it )
    parts.append( static_cast<KMMimePartTreeItem *>( *it )->node() );

  mReaderWin->setUpdateAttachment();

  KMSaveAttachmentsCommand *command =
    new KMSaveAttachmentsCommand( this, parts, mReaderWin->message(), encoded );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMimePartTree::slotSaveAll()
{
  if ( topLevelItemCount() == 0 )
    return;

  mReaderWin->setUpdateAttachment();

  KMCommand *command =
    new KMSaveAttachmentsCommand( this, mReaderWin->message() );
  command->start();
}


void KMMimePartTree::slotDelete()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  if ( selected.count() != 1 )
    return;
  mReaderWin->slotDeleteAttachment(
      static_cast<KMMimePartTreeItem*>( selected.first() )->node()
    );
}

void KMMimePartTree::slotEdit()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  if ( selected.count() != 1 )
    return;
  mReaderWin->slotEditAttachment(
      static_cast<KMMimePartTreeItem*>( selected.first() )->node()
    );
}

void KMMimePartTree::slotOpen()
{
  startHandleAttachmentCommand( KMHandleAttachmentCommand::Open );
}

void KMMimePartTree::slotOpenWith()
{
  startHandleAttachmentCommand( KMHandleAttachmentCommand::OpenWith );
}

void KMMimePartTree::slotView()
{
  startHandleAttachmentCommand( KMHandleAttachmentCommand::View );
}

void KMMimePartTree::slotProperties()
{
  startHandleAttachmentCommand( KMHandleAttachmentCommand::Properties );
}

void KMMimePartTree::startHandleAttachmentCommand( int action )
{
  QList<QTreeWidgetItem *> selected = selectedItems();
  if ( selected.count() != 1 )
    return;

  partNode *node = static_cast<KMMimePartTreeItem *>( selected.first() )->node();
  QString name = mReaderWin->tempFileUrlFromPartNode( node ).path();

  mReaderWin->prepareHandleAttachment( node->nodeId(), name );
  mReaderWin->slotHandleAttachment( action );
}

void KMMimePartTree::slotCopy()
{
  QList<QTreeWidgetItem *> selected = selectedItems();
  if ( selected.count() != 1 )
    return;
  partNode *node = static_cast<KMMimePartTreeItem *>( selected.first() )->node();
  QList<QUrl> urls;
  KUrl kUrl = mReaderWin->tempFileUrlFromPartNode( node );
  QUrl url = QUrl::fromPercentEncoding( kUrl.toEncoded() );
  if ( !url.isValid() )
    return;
  urls.append( url );

  QMimeData *mimeData = new QMimeData;
  mimeData->setUrls( urls );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );
}

void KMMimePartTree::startDrag( Qt::DropActions )
{
  KMMimePartTreeItem *item = static_cast<KMMimePartTreeItem*>( currentItem() );
  if ( !item )
    return;
  partNode *node = item->node();
  if ( !node )
    return;

  QList<QUrl> urls;
  KUrl kUrl = mReaderWin->tempFileUrlFromPartNode( node );
  QUrl url = QUrl::fromPercentEncoding( kUrl.toEncoded() );
  if ( !url.isValid() )
    return;
  urls.append( url );

  QDrag *drag = new QDrag( this );
  QMimeData *mimeData = new QMimeData;
  mimeData->setUrls( urls );
  drag->setMimeData( mimeData );
  drag->exec( Qt::CopyAction );
}


//=============================================================================

KMMimePartTreeItem::KMMimePartTreeItem( KMMimePartTree * parent,
                                        partNode* node,
                                        const QString & description,
                                        const QString & mimetype,
                                        const QString & encoding,
                                        KIO::filesize_t size )
  : QTreeWidgetItem( parent ),
    mPartNode( node ), mDataSize( size )
{
  Q_ASSERT( parent );
  if ( node )
    node->setMimePartTreeItem( this );
  setText( columnDescription, description );
  setIconAndTextForType( mimetype );
  setText( columnEncoding, encoding );
  setText( columnSize, KIO::convertSize( size ) );
  setTextAlignment( columnSize, Qt::AlignRight );
}

KMMimePartTreeItem::KMMimePartTreeItem( KMMimePartTreeItem * parent,
                                        partNode* node,
                                        const QString & description,
                                        const QString & mimetype,
                                        const QString & encoding,
                                        KIO::filesize_t size,
                                        bool revertOrder )
  : QTreeWidgetItem( parent ),
    mPartNode( node ), mDataSize(size)
{
  // With Qt3 the items were inserted at the beginning of the
  // parent's child list. revertOrder caused the item to be appended.
  // With Qt4 we get the opposite behaviour: we're appended
  // by default and with !revertOrder we want to be prepended.
  if ( ( !revertOrder ) && parent ) {
    int idx = parent->indexOfChild( this );
    if ( idx > 0 ) {
      parent->takeChild( idx ); // should return this.
      parent->insertChild( 0, this );
    }
  }
  // Attach to the data tree node
  if ( node )
    node->setMimePartTreeItem( this );

  // Setup column data
  setText( columnDescription, description );
  setIconAndTextForType( mimetype );
  setText( columnEncoding, encoding );
  setText( columnSize, KIO::convertSize( size ) );
  setTextAlignment( columnSize, Qt::AlignRight );


  if ( parent )
    static_cast<KMMimePartTreeItem*>( parent )->correctSize();
}

void KMMimePartTreeItem::correctSize()
{
  int childCnt = childCount();

  if ( childCnt < 1 )
    return; // nothing to correct

  KIO::filesize_t totalChildSize = 0;

  int idx = 0;
  while ( idx < childCnt )
  {
    KMMimePartTreeItem * ch = static_cast<KMMimePartTreeItem*>( child( idx ) );
    totalChildSize += ch->dataSize();
    idx++;
  }

  if ( totalChildSize > dataSize() )
  {
    setText( columnSize, KIO::convertSize( totalChildSize ) );
    setDataSize( totalChildSize );
  }

  if ( parent() )
    static_cast<KMMimePartTreeItem*>( parent() )->correctSize();
}


void KMMimePartTreeItem::setIconAndTextForType( const QString & mime )
{
  QString mimetype = mime.toLower();
  if ( mimetype.startsWith( "multipart/" ) ) {
    setText( columnType, mimetype );
    setIcon( columnDescription, QIcon( SmallIcon("folder") ) );
  } else if ( mimetype == "application/octet-stream" ) {
    setText( columnType, i18n("Unspecified Binary Data") ); // do not show "Unknown"...
    setIcon( columnDescription, QIcon( SmallIcon("application-octet-stream") ) );
  } else {
    KMimeType::Ptr mtp = KMimeType::mimeType( mimetype );
    setText( columnType, ( mtp && !mtp->comment().isEmpty() ) ? mtp->comment() : mimetype );
    setIcon( columnDescription, QIcon( mtp ? KIconLoader::global()->loadMimeTypeIcon(mtp->iconName(), KIconLoader::Small) : SmallIcon("unknown") ) );
  }
}

bool KMMimePartTreeItem::operator < ( const QTreeWidgetItem &other ) const
{
  int sortCol = treeWidget()->sortColumn();

  const KMMimePartTreeItem & item = static_cast<const KMMimePartTreeItem &>( other );

  if ( sortCol == columnSize )
    return mDataSize < item.dataSize();

  return text( sortCol ) < other.text( sortCol );
}

#include "kmmimeparttree.moc"

