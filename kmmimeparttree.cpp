/* -*- c++ -*-
    kmmimeparttree.h A MIME part tree viwer.

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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


#include <config.h>

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

#include <qheader.h>
#include <qpopupmenu.h>
#include <qstyle.h>

KMMimePartTree::KMMimePartTree( KMReaderWin* readerWin,
                                QWidget* parent,
                                const char* name )
    : KListView(  parent, name ),
      mReaderWin( readerWin ), mSizeColumn(0)
{
    setStyleDependantFrameWidth();
    addColumn( i18n("Description") );
    addColumn( i18n("Type") );
    addColumn( i18n("Encoding") );
    mSizeColumn = addColumn( i18n("Size") );
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
  if ( const KMMimePartTreeItem * i = dynamic_cast<KMMimePartTreeItem*>( item ) ) {
    if( mReaderWin->mRootNode == i->node() )
      mReaderWin->update( true ); // Force update
    else
      mReaderWin->setMsgPart( i->node() );
  } else
    kdWarning(5006) << "Item was not a KMMimePartTreeItem!" << endl;
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
        popup->insertItem( SmallIcon("filesaveas"),i18n( "Save &As..." ), this, SLOT( slotSaveAs() ) );
        popup->insertItem( i18n( "Save as &Encoded..." ), this,
                           SLOT( slotSaveAsEncoded() ) );
        popup->insertItem( i18n( "Save All Attachments..." ), this,
                           SLOT( slotSaveAll() ) );
        popup->exec( point );
        delete popup;
        mCurrentContextMenuItem = 0;
    }
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
  QPtrList<QListViewItem> selected = selectedItems();

  Q_ASSERT( !selected.isEmpty() );
  if ( selected.isEmpty() )
    return;

  QPtrListIterator<QListViewItem> it( selected );
  QPtrList<partNode> parts;
  while ( it.current() ) {
    parts.append( static_cast<KMMimePartTreeItem *>(it.current())->node() );
    ++it;
  }
  mReaderWin->setUpdateAttachment();
  KMSaveAttachmentsCommand *command =
    new KMSaveAttachmentsCommand( this, parts, mReaderWin->message(), encoded );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMimePartTree::slotSaveAll()
{
    if( childCount() == 0)
        return;

    mReaderWin->setUpdateAttachment();
    KMCommand *command =
      new KMSaveAttachmentsCommand( this, mReaderWin->message() );
    command->start();
}

//-----------------------------------------------------------------------------
void KMMimePartTree::setStyleDependantFrameWidth()
{
  // set the width of the frame to a reasonable value for the current GUI style
  int frameWidth;
  if( style().isA("KeramikStyle") )
    frameWidth = style().pixelMetric( QStyle::PM_DefaultFrameWidth ) - 1;
  else
    frameWidth = style().pixelMetric( QStyle::PM_DefaultFrameWidth );
  if ( frameWidth < 0 )
    frameWidth = 0;
  if ( frameWidth != lineWidth() )
    setLineWidth( frameWidth );
}


//-----------------------------------------------------------------------------
void KMMimePartTree::styleChange( QStyle& oldStyle )
{
  setStyleDependantFrameWidth();
  KListView::styleChange( oldStyle );
}

//-----------------------------------------------------------------------------
void KMMimePartTree::correctSize( QListViewItem * item )
{
  if (!item) return;

  KIO::filesize_t totalSize = 0;
  QListViewItem * myChild = item->firstChild();
  while ( myChild )
  {
    totalSize += static_cast<KMMimePartTreeItem*>(myChild)->origSize();
    myChild = myChild->nextSibling();
  }
  if ( totalSize > static_cast<KMMimePartTreeItem*>(item)->origSize() )
    item->setText( mSizeColumn, KIO::convertSize(totalSize) );
  if ( item->parent() )
    correctSize( item->parent() );
}

//=============================================================================
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
    mPartNode( node ), mOrigSize(size)
{
  if( node )
    node->setMimePartTreeItem( this );
  setIconAndTextForType( mimetype );
  if ( parent )
    parent->correctSize(this);
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
    mPartNode( node ), mOrigSize(size)
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
  if ( listView() )
    static_cast<KMMimePartTree*>(listView())->correctSize(this);
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
    setText( 1, (mtp && !mtp->comment().isEmpty()) ? mtp->comment() : mimetype );
    setPixmap( 0, mtp ? mtp->pixmap( KIcon::Small) : SmallIcon("unknown") );
  }
}


#include "kmmimeparttree.moc"
