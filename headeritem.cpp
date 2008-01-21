/*******************************************************************************
**
** Filename   : headeritem.cpp
** Created on : 28 November, 2004
** Copyright  : (c) 2004 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/
#include "headeritem.h"

#include "kmheaders.h"
#include "kmfolder.h"
#include "kmmessagetag.h"
#include "kmkernel.h"

#include <klocale.h>
#include <kio/netaccess.h>

#include <QApplication>
#include <QBitmap>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QRegExp>

using namespace KMail;

// Constuction a new list view item with the given colors and pixmap
HeaderItem::HeaderItem( Q3ListView* parent, int msgId, const QString& key )
  : K3ListViewItem( parent ),
  mMsgId( msgId ),
  mKey( key ),
  mAboutToBeDeleted( false ),
  mSortCacheItem( 0 )
{
  irefresh();
}

// Constuction a new list view item with the given parent, colors, & pixmap
HeaderItem::HeaderItem( Q3ListViewItem* parent, int msgId, const QString& key )
  : K3ListViewItem( parent ),
  mMsgId( msgId ),
  mKey( key ),
  mAboutToBeDeleted( false ),
  mSortCacheItem( 0 )
{
  irefresh();
}

HeaderItem::~HeaderItem ()
{
  delete mSortCacheItem;
}

// Update the msgId this item corresponds to.
void HeaderItem::setMsgId( int aMsgId )
{
  mMsgId = aMsgId;
}

// Profiling note: About 30% of the time taken to initialize the
// listview is spent in this function. About 60% is spent in operator
// new and QListViewItem::QListViewItem.
void HeaderItem::irefresh()
{
  KMHeaders *headers = static_cast<KMHeaders*>(listView());
  NestingPolicy threadingPolicy = headers->getNestingPolicy();
  if ((threadingPolicy == AlwaysOpen) ||
      (threadingPolicy == DefaultOpen)) {
    //Avoid opening items as QListView is currently slow to do so.
    setOpen(true);
    return;

  }
  if (threadingPolicy == DefaultClosed)
    return; //default to closed

  // otherwise threadingPolicy == OpenUnread
  if (parent() && parent()->isOpen()) {
    setOpen(true);
    return;
  }

  KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
  mSerNum = mMsgBase->getMsgSerNum();
  MessageStatus status = mMsgBase->status();
  if ( status.isNew() || status.isUnread() || status.isImportant()
      || status.isTodo() || status.isWatched() ) {
    setOpen(true);
    HeaderItem * topOfThread = this;
    while(topOfThread->parent())
      topOfThread = (HeaderItem*)topOfThread->parent();
    topOfThread->setOpenRecursive(true);
  }
}

// Return the msgId of the message associated with this item
int HeaderItem::msgId() const
{
  return mMsgId;
}

// Return the serial number
quint32 HeaderItem::msgSerNum() const
{
  return mSerNum;
}

// Update this item to summarise a new folder and message

//Opens all children in the thread
void HeaderItem::setOpenRecursive( bool open )
{
  if (open){
    Q3ListViewItem * lvchild;
    lvchild = firstChild();
    while (lvchild){
      ((HeaderItem*)lvchild)->setOpenRecursive( true );
      lvchild = lvchild->nextSibling();
    }
    setOpen( true );
  } else {
    setOpen( false );
  }
}

QString HeaderItem::text( int col) const
{
  KMHeaders *headers = static_cast<KMHeaders*>(listView());
  KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
  QString tmp;

  if ( !mMsgBase )
    return QString();

  if ( col == headers->paintInfo()->senderCol ) {
    if ( (headers->folder()->whoField().toLower() == "to") && !headers->paintInfo()->showReceiver )
      tmp = mMsgBase->toStrip();
    else
      tmp = mMsgBase->fromStrip();
    if (tmp.isEmpty())
      tmp = i18n("Unknown");
    else
      tmp = tmp.simplified();

  } else if ( col == headers->paintInfo()->receiverCol ) {
    tmp = mMsgBase->toStrip();
    if (tmp.isEmpty())
      tmp = i18n("Unknown");
    else
      tmp = tmp.simplified();

  } else if(col == headers->paintInfo()->subCol) {
    tmp = mMsgBase->subject();
    if (tmp.isEmpty())
      tmp = i18n("No Subject");
    else
      tmp.remove(QRegExp("[\r\n]"));

  } else if(col == headers->paintInfo()->dateCol) {
    tmp = headers->mDate.dateString( mMsgBase->date() );
  } else if(col == headers->paintInfo()->sizeCol
      && headers->paintInfo()->showSize) {
    if ( mMsgBase->parent()->folderType() == KMFolderTypeImap ) {
      tmp = KIO::convertSize( mMsgBase->msgSizeServer() );
    } else {
      tmp = KIO::convertSize( mMsgBase->msgSize() );
    }
  }
  return tmp;
}

void HeaderItem::setup()
{
  widthChanged();
  const int ph = KMHeaders::pixNew->height();
  Q3ListView *v = listView();
  int h = qMax( v->fontMetrics().height(), ph ) + 2*v->itemMargin();
  h = qMax( h, QApplication::globalStrut().height());
  if ( h % 2 > 0 )
    h++;
  setHeight( h );
}

QPixmap HeaderItem::pixmapMerge( PixmapList pixmaps ) const
{
  // Calculate the width and height of the resulting pixmap
  int width = 0;
  int height = 0;
  for ( PixmapList::ConstIterator it = pixmaps.begin();
      it != pixmaps.end(); ++it ) {
    width += (*it).width();
    height = qMax( height, (*it).height() );
  }

  QPixmap res( width, height );
  res.fill( Qt::transparent );
  QPainter resultPainter( &res );

  // Paint all pixmaps of the list to the resulting pixmap
  int x = 0;
  for ( PixmapList::ConstIterator it = pixmaps.begin();
      it != pixmaps.end(); ++it ) {
    resultPainter.drawPixmap( x, ( height - (*it).height() ) / 2, *it );
    x += (*it).width();
  }

  resultPainter.end();
  return res;
}

const QPixmap *HeaderItem::cryptoIcon(KMMsgBase *msgBase) const
{
  switch ( msgBase->encryptionState() )
  {
    case KMMsgFullyEncrypted        : return KMHeaders::pixFullyEncrypted;
    case KMMsgPartiallyEncrypted    : return KMHeaders::pixPartiallyEncrypted;
    case KMMsgEncryptionStateUnknown: return KMHeaders::pixUndefinedEncrypted;
    case KMMsgEncryptionProblematic : return KMHeaders::pixEncryptionProblematic;
    default                         : return 0;
  }
}

const QPixmap *HeaderItem::signatureIcon(KMMsgBase *msgBase) const
{
  switch ( msgBase->signatureState() )
  {
    case KMMsgFullySigned          : return KMHeaders::pixFullySigned;
    case KMMsgPartiallySigned      : return KMHeaders::pixPartiallySigned;
    case KMMsgSignatureStateUnknown: return KMHeaders::pixUndefinedSigned;
    case KMMsgSignatureProblematic : return KMHeaders::pixSignatureProblematic;
    default                        : return 0;
  }
}

const QPixmap *HeaderItem::statusIcon(KMMsgBase *msgBase) const
{
  // forwarded, replied have precedence over the other states
  MessageStatus status = msgBase->status();
  if (  status.isForwarded() && !status.isReplied() ) return KMHeaders::pixReadFwd;
  if ( !status.isForwarded() &&  status.isReplied() ) return KMHeaders::pixReadReplied;
  if (  status.isForwarded() &&  status.isReplied() ) return KMHeaders::pixReadFwdReplied;

  // a queued or sent mail is usually also read
  if ( status.isQueued() ) return KMHeaders::pixQueued;
  if ( status.isSent()   ) return KMHeaders::pixSent;

  if ( status.isNew()                    ) return KMHeaders::pixNew;
  if ( status.isRead() || status.isOld() ) return KMHeaders::pixRead;
  if ( status.isUnread()                 ) return KMHeaders::pixUns;
  if ( status.isDeleted()                ) return KMHeaders::pixDel;

  return 0;
}

const QPixmap *HeaderItem::pixmap(int col) const
{
  KMHeaders *headers = static_cast<KMHeaders*>(listView());
  KMMsgBase *msgBase = headers->folder()->getMsgBase( mMsgId );
  MessageStatus status = msgBase->status();

  if ( col == headers->paintInfo()->subCol ) {

    PixmapList pixmaps;

    if ( !headers->mPaintInfo.showSpamHam ) {
      // Have the spam/ham and watched/ignored icons first, I guess.
      if ( status.isSpam() ) pixmaps << *KMHeaders::pixSpam;
      if ( status.isHam()  ) pixmaps << *KMHeaders::pixHam;
    }

    if ( !headers->mPaintInfo.showWatchedIgnored ) {
      if ( status.isIgnored() ) pixmaps << *KMHeaders::pixIgnored;
      if ( status.isWatched() ) pixmaps << *KMHeaders::pixWatched;
    }

    if ( !headers->mPaintInfo.showStatus ) {
      const QPixmap *pix = statusIcon(msgBase);
      if ( pix ) pixmaps << *pix;
    }

    // Only merge the attachment icon in if that is configured.
    if ( headers->paintInfo()->showAttachmentIcon &&
        !headers->paintInfo()->showAttachment &&
        msgBase->attachmentState() == KMMsgHasAttachment )
      pixmaps << *KMHeaders::pixAttachment;

    // Only merge the crypto icons in if that is configured.
    if ( headers->paintInfo()->showCryptoIcons ) {
      const QPixmap *pix;

      if ( !headers->paintInfo()->showCrypto )
        if ( (pix = cryptoIcon(msgBase))    ) pixmaps << *pix;

      if ( !headers->paintInfo()->showSigned )
        if ( (pix = signatureIcon(msgBase)) ) pixmaps << *pix;
    }

    if ( !headers->mPaintInfo.showImportant )
      if ( status.isImportant() ) pixmaps << *KMHeaders::pixFlag;

    if ( !headers->mPaintInfo.showTodo )
      if ( status.isTodo() ) pixmaps << *KMHeaders::pixTodo;

    if ( !pixmaps.isEmpty() ) {
      static QPixmap mergedpix;
      mergedpix = pixmapMerge( pixmaps );
      return &mergedpix;
    }
    else
      return 0;
  }
  else if ( col == headers->paintInfo()->statusCol ) {
    return statusIcon(msgBase);
  }
  else if ( col == headers->paintInfo()->attachmentCol ) {
    if ( msgBase->attachmentState() == KMMsgHasAttachment )
      return KMHeaders::pixAttachment;
  }
  else if ( col == headers->paintInfo()->importantCol ) {
    if ( status.isImportant() )
      return KMHeaders::pixFlag;
  }
  else if ( col == headers->paintInfo()->todoCol ) {
    if ( status.isTodo() )
      return KMHeaders::pixTodo;
  }
  else if ( col == headers->paintInfo()->spamHamCol ) {
    if ( status.isSpam() ) return KMHeaders::pixSpam;
    if ( status.isHam()  ) return KMHeaders::pixHam;
  }
  else if ( col == headers->paintInfo()->watchedIgnoredCol ) {
    if ( status.isWatched() ) return KMHeaders::pixWatched;
    if ( status.isIgnored() ) return KMHeaders::pixIgnored;
  }
  else if ( col == headers->paintInfo()->signedCol ) {
    return signatureIcon(msgBase);
  }
  else if ( col == headers->paintInfo()->cryptoCol ) {
    return cryptoIcon(msgBase);
  }
  return 0;
}

void HeaderItem::paintCell( QPainter * p, const QColorGroup & cg,
    int column, int width, int align )
{
  KMHeaders *headers = static_cast<KMHeaders*>(listView());
  if (headers->noRepaint) return;
  if (!headers->folder()) return;
  KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
  if (!mMsgBase) return;
  MessageStatus status = mMsgBase->status();

  QColorGroup _cg( cg );
  QColor c = _cg.color( QPalette::Text );
  QColor *color = const_cast<QColor *>( &headers->paintInfo()->colFore );
  QFont font = p->font();
  int weight = font.weight();

  QColor tmp_fgcolor;
  QFont tmp_font;
  KMMessageTagList* tmp_taglist = mMsgBase->tagList();
  if ( ( tmp_taglist ) && ( ! tmp_taglist->isEmpty() ) ) {
    //Since the tag list is supposed to be priority sorted,
    //only the first item is of interest
    const KMMessageTagDescription* firstTag = kmkernel->msgTagMgr()->find(
                                                        (*tmp_taglist)[0] );
    if ( firstTag ) {
      tmp_fgcolor = firstTag->textColor();
      if ( tmp_fgcolor.isValid() ) {
        color = &tmp_fgcolor;
      }
      QFont tmp_font = firstTag->textFont();
      if ( tmp_font != QFont() ) {
        font = tmp_font;
        weight = qMax( weight, font.weight() );
      }
    }
  }

  // for color and font family "important" overrides "new" overrides "unread"
  // overrides "todo" for the weight we use the maximal weight
  if ( status.isTodo() ) {
    color = const_cast<QColor*>( &headers->paintInfo()->colTodo );
    font = headers->todoFont();
    weight = qMax( weight, font.weight() );
  }
  if ( status.isUnread() ) {
    color = const_cast<QColor*>( &headers->paintInfo()->colUnread );
    font = headers->unreadFont();
    weight = qMax( weight, font.weight() );
  }
  if ( status.isNew() ) {
    color = const_cast<QColor*>( &headers->paintInfo()->colNew );
    font = headers->newFont();
    weight = qMax( weight, font.weight() );
  }

  if ( status.isImportant() ) {
    color = const_cast<QColor*>( &headers->paintInfo()->colFlag );
    font = headers->importantFont();
    weight = qMax( weight, font.weight() );
  }
  if ( column == headers->paintInfo()->dateCol ) {
    font = headers->dateFont();
  }

  QColor cdisabled = KGlobalSettings::inactiveTextColor();
  if ( headers->isMessageCut( msgSerNum() ) ) {
    color = &cdisabled;
  }

  // set color and font
  _cg.setColor( QPalette::Text, *color );
  font.setWeight( weight );
  p->setFont( font );

  K3ListViewItem::paintCell( p, _cg, column, width, align );

  if (aboutToBeDeleted()) {
    // strike through
    p->drawLine( 0, height()/2, width, height()/2);
  }

  // reset color
  _cg.setColor( QPalette::Text, c );
}

QString HeaderItem::generate_key( KMHeaders *headers,
    KMMsgBase *msg,
    const KPaintInfo *paintInfo,
    int sortOrder )
{
  // It appears, that QListView in Qt-3.0 asks for the key
  // in QListView::clear(), which is called from
  // readSortOrder()
  if (!msg) return QString();
  MessageStatus status = msg->status();

  int column = sortOrder & ((1 << 5) - 1);
  QString ret = QString( (char)sortOrder );
  QString sortArrival = QString( "%1" ).arg( msg->getMsgSerNum(), 0, 36 );
  while (sortArrival.length() < 7) sortArrival = '0' + sortArrival;

  if (column == paintInfo->dateCol) {
    if (paintInfo->orderOfArrival)
      return ret + sortArrival;
    else {
      QString d = QString::number(msg->date());
      while (d.length() <= 10) d = '0' + d;
      return ret + d + sortArrival;
    }
  } else if (column == paintInfo->senderCol) {
    QString tmp;
    if ( (headers->folder()->whoField().toLower() == "to") && !headers->paintInfo()->showReceiver )
      tmp = msg->toStrip();
    else
      tmp = msg->fromStrip();
    return ret + tmp.toLower() + ' ' + sortArrival;
  } else if (column == paintInfo->receiverCol) {
    QString tmp = msg->toStrip();
    return ret + tmp.toLower() + ' ' + sortArrival;
  } else if (column == paintInfo->subCol) {
    QString tmp;
    tmp = ret;
    if (paintInfo->status) {
      tmp += msg->status().getSortRank() + ' ';
    }
    tmp += KMMessage::stripOffPrefixes( msg->subject().toLower() ) + ' ' + sortArrival;
    return tmp;
  }
  else if (column == paintInfo->sizeCol) {
    QString len;
    if ( msg->parent()->folderType() == KMFolderTypeImap )
    {
      len = QString::number( msg->msgSizeServer() );
    } else {
      len = QString::number( msg->msgSize() );
    }
    while (len.length() < 9) len = '0' + len;
    return ret + len + sortArrival;
  }
  else if (column == paintInfo->statusCol) {
    QString s;
    if      ( status.isNew()                              ) s = "1";
    else if ( status.isUnread()                           ) s = "2";
    else if (!status.isForwarded() &&  status.isReplied() ) s = "3";
    else if ( status.isForwarded() &&  status.isReplied() ) s = "4";
    else if ( status.isForwarded() && !status.isReplied() ) s = "5";
    else if ( status.isRead() || status.isOld()           ) s = "6";
    else if ( status.isQueued()                           ) s = "7";
    else if ( status.isSent()                             ) s = "8";
    else if ( status.isDeleted()                          ) s = "9";
    return ret + s + sortArrival;
  }
  else if (column == paintInfo->attachmentCol) {
    QString s(msg->attachmentState() == KMMsgHasAttachment ? "1" : "0");
    return ret + s + sortArrival;
  }
  else if (column == paintInfo->importantCol) {
    QString s( status.isImportant() ? "1" : "0" );
    return ret + s + sortArrival;
  }
  else if ( column == paintInfo->todoCol ) {
    QString s( status.isTodo() ? "1": "0" );
    return ret + s + sortArrival;
  }
  else if (column == paintInfo->spamHamCol) {
    QString s(( status.isSpam() || status.isHam() ) ? "1" : "0" );
    return ret + s + sortArrival;
  }
  else if (column == paintInfo->watchedIgnoredCol) {
    QString s(( status.isWatched() || status.isIgnored() ) ? "1" : "0" );
    return ret + s + sortArrival;
  }
  else if (column == paintInfo->signedCol) {
    QString s;
    switch ( msg->signatureState() )
    {
      case KMMsgFullySigned          : s = "1"; break;
      case KMMsgPartiallySigned      : s = "2"; break;
      case KMMsgSignatureStateUnknown: s = "3"; break;
      case KMMsgSignatureProblematic : s = "4"; break;
      default                        : s = "5"; break;
    }
    return ret + s + sortArrival;
  }
  else if (column == paintInfo->cryptoCol) {
    QString s;
    switch ( msg->encryptionState() )
    {
      case KMMsgFullyEncrypted        : s = "1"; break;
      case KMMsgPartiallyEncrypted    : s = "2"; break;
      case KMMsgEncryptionStateUnknown: s = "3"; break;
      case KMMsgEncryptionProblematic : s = "4"; break;
      default                         : s = "5"; break;
    }
    return ret + s + sortArrival;
  }
  return ret + "missing key"; //you forgot something!!
}

QString HeaderItem::key( int column, bool /*ascending*/ ) const
{
  KMHeaders *headers = static_cast<KMHeaders*>(listView());
  int sortOrder = column;
  if (headers->mPaintInfo.orderOfArrival)
    sortOrder |= (1 << 6);
  if (headers->mPaintInfo.status)
    sortOrder |= (1 << 5);
  //This code should stay pretty much like this, if you are adding new
  //columns put them in generate_key
  if(mKey.isEmpty() || mKey[0] != (char)sortOrder) {
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    KMMsgBase *msgBase = headers->folder()->getMsgBase( mMsgId );
    return ((HeaderItem *)this)->mKey =
      generate_key( headers, msgBase, headers->paintInfo(), sortOrder );
  }
  return mKey;
}

void HeaderItem::setTempKey( const QString &key ) {
  mKey = key;
}

int HeaderItem::compare( Q3ListViewItem *i, int col, bool ascending ) const
{
  int res = 0;
  KMHeaders *headers = static_cast<KMHeaders*>(listView());
  if ( ( col == headers->paintInfo()->statusCol         ) ||
      ( col == headers->paintInfo()->sizeCol           ) ||
      ( col == headers->paintInfo()->attachmentCol     ) ||
      ( col == headers->paintInfo()->importantCol      ) ||
      ( col == headers->paintInfo()->todoCol           ) ||
      ( col == headers->paintInfo()->spamHamCol        ) ||
      ( col == headers->paintInfo()->signedCol         ) ||
      ( col == headers->paintInfo()->cryptoCol         ) ||
      ( col == headers->paintInfo()->watchedIgnoredCol ) ) {
    res = key( col, ascending ).compare( i->key( col, ascending ) );
  } else if ( col == headers->paintInfo()->dateCol ) {
    res = key( col, ascending ).compare( i->key( col, ascending ) );
    if (i->parent() && !ascending)
      res = -res;
  } else if ( col == headers->paintInfo()->subCol ||
      col == headers->paintInfo()->senderCol ||
      col == headers->paintInfo()->receiverCol ) {
    res = key( col, ascending ).localeAwareCompare( i->key( col, ascending ) );
  }
  return res;
}

Q3ListViewItem* HeaderItem::firstChildNonConst() /* Non const! */
{
  enforceSortOrder(); // Try not to rely on QListView implementation details
  return firstChild();
}

