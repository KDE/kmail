/*  -*- c++ -*-
    attachmentlistview.cpp

    KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>
    Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "attachmentlistview.h"


#include <QHeaderView>
#include <QContextMenuEvent>
#include <QBuffer>

#include <klocale.h>
#include <maillistdrag.h>

#include "kmfolder.h"
#include "kmmsgdict.h"
#include "kmcommands.h"
#include "composer.h"


static const int encryptColumn = 5;

namespace KMail {

AttachmentListView::AttachmentListView( KMail::Composer * composer,
                                        QWidget* parent )
  : QTreeWidget( parent ),
    mComposer( composer )
{
  setAcceptDrops( true );
  setObjectName( "attachment list view" );
  setSelectionMode( QAbstractItemView::ExtendedSelection );
  setIndentation( 0 );
  setAllColumnsShowFocus( true );
  QStringList headerNames;
  headerNames << i18n("Name") << i18n("Size") << i18n("Encoding") 
      << i18n("Type") << i18n("Compress") << i18n("Encrypt") 
      << i18n("Sign");
  setHeaderLabels( headerNames ); 

  header()->setResizeMode( QHeaderView::Interactive );
  header()->setResizeMode( 3, QHeaderView::Stretch );
  header()->setStretchLastSection( false );
  setColumnWidth( 0, 200);
  setColumnWidth( 1, 80 );
  setColumnWidth( 2, 120 );
  setColumnWidth( 3, 120 );
  setColumnWidth( 4, 100 );
  setColumnWidth( 5, 80 );
  setColumnWidth( 6, 80 );

  // We can not enable automatic sorting, because that would resort the list
  // when the list is sorted by size and the user clicks the compress checkbox.
  header()->setSortIndicatorShown( true );
  header()->setClickable( true );
  connect( header(), SIGNAL(sectionClicked(int)), this, SLOT(slotSort()) );

  enableCryptoCBs( false );

  // This is needed because QTreeWidget forgets to update when a column is
  // moved. It updates correctly when a column is resized, so trick it into
  // thinking a column was resized when it is moved.
  connect ( header(), SIGNAL(sectionMoved(int,int,int)),
            header(), SIGNAL(sectionResized(int,int,int)) );
}

//-----------------------------------------------------------------------------

AttachmentListView::~AttachmentListView()
{
}

//-----------------------------------------------------------------------------

void AttachmentListView::enableCryptoCBs( bool enable )
{
  bool enabledBefore = areCryptoCBsEnabled();

  // Show the crypto columns if they were hidden before
  if ( enable && !enabledBefore ) {

    // determine the total width of the columns
    int totalWidth = 0;
    for ( int col = 0; col < encryptColumn; col++ )
      totalWidth += header()->sectionSize( col );

    int reducedTotalWidth = totalWidth - mEncryptColWidth - mSignColWidth;

    // reduce the width of all columns so that the encrypt and sign columns fit
    int usedWidth = 0;
    for ( int col = 0; col < encryptColumn - 1; col++ ) {
      int newWidth = header()->sectionSize( col ) * reducedTotalWidth /
          totalWidth;
      setColumnWidth( col, newWidth );
      usedWidth += newWidth;
    }

    // the last column before the encrypt column gets the remaining space
    // (because of rounding errors the width of this column isn't calculated
    // the same way as the width of the other columns)
    setColumnWidth( encryptColumn - 1, reducedTotalWidth - usedWidth );
    setColumnWidth( encryptColumn, mEncryptColWidth );
    setColumnWidth( encryptColumn + 1, mSignColWidth );
  }

  // Hide the crypto columns if they were shown before
  if ( !enable && enabledBefore ) {

    mEncryptColWidth = columnWidth( encryptColumn );
    mSignColWidth = columnWidth( encryptColumn + 1 );

    // determine the total width of the columns
    int totalWidth = 0;
    for( int col = 0; col < header()->count(); col++ )
      totalWidth += header()->sectionSize( col );

    int reducedTotalWidth = totalWidth - mEncryptColWidth - mSignColWidth;

    // increase the width of all columns so that the visible columns take
    // up the whole space
    int usedWidth = 0;
    for( int col = 0; col < encryptColumn - 1; col++ ) {
      int newWidth = header()->sectionSize( col ) * totalWidth /
          reducedTotalWidth;
      setColumnWidth( col, newWidth );
      usedWidth += newWidth;
    }

    // the last column before the encrypt column gets the remaining space
    // (because of rounding errors the width of this column isn't calculated
    // the same way as the width of the other columns)
    setColumnWidth( encryptColumn - 1, totalWidth - usedWidth );
    setColumnWidth( encryptColumn, 0 );
    setColumnWidth( encryptColumn + 1, 0 );
  }
}

//-----------------------------------------------------------------------------

bool AttachmentListView::areCryptoCBsEnabled()
{
  return ( header()->sectionSize( encryptColumn  ) != 0 );
}

//-----------------------------------------------------------------------------

void AttachmentListView::dragEnterEvent( QDragEnterEvent* e )
{
  if( KPIM::MailList::canDecode( e->mimeData() ) )
    e->setAccepted( true );
  else
    QTreeWidget::dragEnterEvent( e );
}

//-----------------------------------------------------------------------------

void AttachmentListView::dragMoveEvent( QDragMoveEvent* e )
{
  if( KPIM::MailList::canDecode( e->mimeData() ) )
    e->setAccepted( true );
  else
    QTreeWidget::dragMoveEvent( e );
}

//-----------------------------------------------------------------------------

void AttachmentListView::dropEvent( QDropEvent* e )
{
  const QMimeData *md = e->mimeData();
  if( KPIM::MailList::canDecode( md ) ) {
    e->accept();
    // Decode the list of serial numbers stored as the drag data
    QByteArray serNums = KPIM::MailList::serialsFromMimeData( md );
    QBuffer serNumBuffer( &serNums );
    serNumBuffer.open( QIODevice::ReadOnly );
    QDataStream serNumStream( &serNumBuffer );
    quint32 serNum;
    KMFolder *folder = 0;
    int idx;
    QList<KMMsgBase*> messageList;
    while( !serNumStream.atEnd() ) {
      KMMsgBase *msgBase = 0;
      serNumStream >> serNum;
      KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
      if( folder )
        msgBase = folder->getMsgBase( idx );
      if( msgBase )
        messageList.append( msgBase );
    }
    serNumBuffer.close();
    uint identity = folder ? folder->identity() : 0;
    KMCommand *command = new KMForwardAttachedCommand( mComposer, messageList,
                                                       identity, mComposer );
    command->start();
  }
  else {
    KUrl::List urlList = KUrl::List::fromMimeData( md );
    if ( !urlList.isEmpty() ) {
      e->accept();
      for( KUrl::List::Iterator it = urlList.begin();
           it != urlList.end(); ++it ) {
        mComposer->addAttach( *it );
      }
    }
    else {
      QTreeWidget::dropEvent( e );
    }
  }
}

//-----------------------------------------------------------------------------

void AttachmentListView::keyPressEvent( QKeyEvent * e )
{
  if ( e->key() == Qt::Key_Delete ) {
    emit attachmentDeleted();
  }
}

//-----------------------------------------------------------------------------

void AttachmentListView::contextMenuEvent( QContextMenuEvent* event )
{
  emit rightButtonPressed( itemAt( event->pos() ) );
}

//-----------------------------------------------------------------------------

void AttachmentListView::slotSort()
{
  sortByColumn( header()->sortIndicatorSection(), header()->sortIndicatorOrder() );
}


} // namespace KMail

#include "attachmentlistview.moc"
