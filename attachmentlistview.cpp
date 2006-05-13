/*  -*- c++ -*-
    attachmentlistview.cpp

    KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// my header file
#include "attachmentlistview.h"

// other KMail headers
#include "kmmsgbase.h"
#include "kmfolder.h"
#include "kmcommands.h"
#include "kmmsgdict.h"
#include "composer.h"

// other module headers
#include <maillistdrag.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QDropEvent>
using KPIM::MailListDrag;

// other KDE headers
#include <kurl.h>

// other Qt headers
#include <q3cstring.h>
#include <QBuffer>
#include <QDataStream>
#include <QEvent>
#include <QString>

// other headers (none)


namespace KMail {

AttachmentListView::AttachmentListView( KMail::Composer * composer,
                                        QWidget* parent )
  : K3ListView( parent ),
    mComposer( composer )
{
  setAcceptDrops( true );
}

//-----------------------------------------------------------------------------

AttachmentListView::~AttachmentListView()
{
}

//-----------------------------------------------------------------------------

void AttachmentListView::contentsDragEnterEvent( QDragEnterEvent* e )
{
  if( e->provides( MailListDrag::format() ) )
    e->setAccepted( true );
  else
    K3ListView::dragEnterEvent( e );
}

//-----------------------------------------------------------------------------

void AttachmentListView::contentsDragMoveEvent( QDragMoveEvent* e )
{
  if( e->provides( MailListDrag::format() ) )
    e->setAccepted( true );
  else
    K3ListView::dragMoveEvent( e );
}

//-----------------------------------------------------------------------------

void AttachmentListView::contentsDropEvent( QDropEvent* e )
{
  if( e->provides( MailListDrag::format() ) ) {
    // Decode the list of serial numbers stored as the drag data
    QByteArray serNums;
    MailListDrag::decode( e, serNums );
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
    KUrl::List urlList = KUrl::List::fromMimeData( e->mimeData() );
    if ( !urlList.isEmpty() ) {
      for( KUrl::List::Iterator it = urlList.begin();
           it != urlList.end(); ++it ) {
        mComposer->addAttach( *it );
      }
    }
    else {
      K3ListView::dropEvent( e );
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


} // namespace KMail

#include "attachmentlistview.moc"
