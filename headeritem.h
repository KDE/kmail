/*******************************************************************************
**
** Filename   : headeritem.h
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
#ifndef HEADERITEM_H
#define HEADERITEM_H

#include <stdlib.h>

#include <klistview.h> // include for the base class

class KMMsgBase;
class KPaintInfo;
class KMFolder;
class KMHeaders;

namespace KMail
{
class HeaderItem; // forward declaration

class KMSortCacheItem {
    HeaderItem *mItem;
    KMSortCacheItem *mParent;
    int mId, mSortOffset;
    QString mKey;

    QPtrList<KMSortCacheItem> mSortedChildren;
    int mUnsortedCount, mUnsortedSize;
    KMSortCacheItem **mUnsortedChildren;
    bool mImperfectlyThreaded;

public:
    KMSortCacheItem() : mItem(0), mParent(0), mId(-1), mSortOffset(-1),
        mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0),
        mImperfectlyThreaded (true) { }
    KMSortCacheItem(int i, QString k, int o=-1)
        : mItem(0), mParent(0), mId(i), mSortOffset(o), mKey(k),
          mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0),
          mImperfectlyThreaded (true) { }
    ~KMSortCacheItem() { if(mUnsortedChildren) free(mUnsortedChildren); }

    KMSortCacheItem *parent() const { return mParent; } //can't be set, only by the parent
    bool isImperfectlyThreaded() const
        { return mImperfectlyThreaded; }
    void setImperfectlyThreaded (bool val)
        { mImperfectlyThreaded = val; }
    bool hasChildren() const
        { return mSortedChildren.count() || mUnsortedCount; }
    const QPtrList<KMSortCacheItem> *sortedChildren() const
        { return &mSortedChildren; }
    KMSortCacheItem **unsortedChildren(int &count) const
        { count = mUnsortedCount; return mUnsortedChildren; }
    void addSortedChild(KMSortCacheItem *i) {
        i->mParent = this;
        mSortedChildren.append(i);
    }
    void addUnsortedChild(KMSortCacheItem *i) {
        i->mParent = this;
        if(!mUnsortedChildren)
            mUnsortedChildren = (KMSortCacheItem **)malloc((mUnsortedSize = 25) * sizeof(KMSortCacheItem *));
        else if(mUnsortedCount >= mUnsortedSize)
            mUnsortedChildren = (KMSortCacheItem **)realloc(mUnsortedChildren,
                                                            (mUnsortedSize *= 2) * sizeof(KMSortCacheItem *));
        mUnsortedChildren[mUnsortedCount++] = i;
    }

    HeaderItem *item() const { return mItem; }
    void setItem(HeaderItem *i) { Q_ASSERT(!mItem); mItem = i; }

    const QString &key() const { return mKey; }
    void setKey(const QString &key) { mKey = key; }

    int id() const { return mId; }
    void setId(int id) { mId = id; }

    int offset() const { return mSortOffset; }
    void setOffset(int x) { mSortOffset = x; }

    void updateSortFile( FILE *sortStream, KMFolder *folder,
                         bool waiting_for_parent = false,
                         bool update_discovered_count = false);
};

class HeaderItem : public KListViewItem
{
public:
  HeaderItem( QListView* parent, int msgId, const QString& key = QString::null );
  HeaderItem( QListViewItem* parent, int msgId, const QString& key = QString::null );
  ~HeaderItem ();

  // Update the msgId this item corresponds to.
  void setMsgId( int aMsgId );

  // Profiling note: About 30% of the time taken to initialize the
  // listview is spent in this function. About 60% is spent in operator
  // new and QListViewItem::QListViewItem.
  void irefresh();

  // Return the msgId of the message associated with this item
  int msgId() const;

  // Update this item to summarise a new folder and message
  void reset( int aMsgId );

  //Opens all children in the thread
  void setOpenRecursive( bool open );

  QString text( int col) const;

  void setup();

  typedef QValueList<QPixmap> PixmapList;

  QPixmap pixmapMerge( PixmapList pixmaps ) const;

  const QPixmap *cryptoIcon(KMMsgBase *msgBase) const;
  const QPixmap *signatureIcon(KMMsgBase *msgBase) const;
  const QPixmap *statusIcon(KMMsgBase *msgBase) const;

  const QPixmap *pixmap(int col) const;

  void paintCell( QPainter * p, const QColorGroup & cg,
                                int column, int width, int align );

  static QString generate_key( KMHeaders *headers, KMMsgBase *msg, const KPaintInfo *paintInfo, int sortOrder );

  virtual QString key( int column, bool /*ascending*/ ) const;

  void setTempKey( QString key );

  int compare( QListViewItem *i, int col, bool ascending ) const;
  
  QListViewItem* firstChildNonConst(); /* Non const! */ 

  bool aboutToBeDeleted() const { return mAboutToBeDeleted; }
  void setAboutToBeDeleted( bool val ) { mAboutToBeDeleted = val; }

  void setSortCacheItem( KMSortCacheItem *item ) { mSortCacheItem = item; }
  KMSortCacheItem* sortCacheItem() const { return mSortCacheItem; }

private:
  int mMsgId;
  QString mKey;
  bool mAboutToBeDeleted;
  KMSortCacheItem *mSortCacheItem;
}; // End of class HeaderItem

} // End of namespace KMail


#endif // HEADERITEM_H
