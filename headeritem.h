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


/** 
 * Represents an item in the set of mails to be displayed but only as far
 * as sorting, threading and reading/writing of the current sort order to
 * a disk cache is concerned. Each such item is paired with a HeaderItem, 
 * which holds the graphical representation of each item (mail).
 * This is what the threading trees are built of.
 */
class SortCacheItem {

public:
    SortCacheItem() : mItem(0), mParent(0), mId(-1), mSortOffset(-1),
        mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0),
        mImperfectlyThreaded (true) { }
    SortCacheItem(int i, QString k, int o=-1)
        : mItem(0), mParent(0), mId(i), mSortOffset(o), mKey(k),
          mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0),
          mImperfectlyThreaded (true) { }
    ~SortCacheItem() { if(mUnsortedChildren) free(mUnsortedChildren); }

    SortCacheItem *parent() const { return mParent; } //can't be set, only by the parent
    /**
     * if an item is imperfectly threaded (by References or subject, not by 
     * In-Reply-To) it will be reevalutated when a new mail comes in. It could be
     * the perfect parent. */
    bool isImperfectlyThreaded() const
        { return mImperfectlyThreaded; }
    void setImperfectlyThreaded (bool val)
        { mImperfectlyThreaded = val; }
    bool hasChildren() const
        { return mSortedChildren.count() || mUnsortedCount; }
    
    /** The sorted children are an array of sortcache items we know are below the
     * current one and are already properly sorted (as read from the cache ) */
    const QPtrList<SortCacheItem> *sortedChildren() const
        { return &mSortedChildren; }
 
    /** The unsorted children are an array of sortcache items we know are below the
     * current one, but are yet to be threaded and sorted properly. */
    SortCacheItem **unsortedChildren(int &count) const
        { count = mUnsortedCount; return mUnsortedChildren; }
    void addSortedChild(SortCacheItem *i) {
        i->mParent = this;
        mSortedChildren.append(i);
    }
    void addUnsortedChild(SortCacheItem *i) {
        i->mParent = this;
        if(!mUnsortedChildren)
            mUnsortedChildren = (SortCacheItem **)malloc((mUnsortedSize = 25) * sizeof(SortCacheItem *));
        else if(mUnsortedCount >= mUnsortedSize)
            mUnsortedChildren = (SortCacheItem **)realloc(mUnsortedChildren,
                                                            (mUnsortedSize *= 2) * sizeof(SortCacheItem *));
        mUnsortedChildren[mUnsortedCount++] = i;
    }

    /** Clear the sorted and unsorted children datastructures. */
    void clearChildren() {
      mSortedChildren.clear();
      free( mUnsortedChildren );
      mUnsortedChildren = 0;
      mUnsortedCount = mUnsortedSize = 0;
    }

    /** the corresponding HeaderItem */
    HeaderItem *item() const { return mItem; }
    void setItem(HeaderItem *i) { Q_ASSERT(!mItem); mItem = i; }

    /** sort key as used by the listview */
    const QString &key() const { return mKey; }
    void setKey(const QString &key) { mKey = key; }

    int id() const { return mId; }
    void setId(int id) { mId = id; }

    /** offset in the cache file stream */
    int offset() const { return mSortOffset; }
    void setOffset(int x) { mSortOffset = x; }

    void updateSortFile( FILE *sortStream, KMFolder *folder,
                         bool waiting_for_parent = false,
                         bool update_discovered_count = false);
private:
    HeaderItem *mItem;
    SortCacheItem *mParent;
    int mId, mSortOffset;
    QString mKey;

    QPtrList<SortCacheItem> mSortedChildren;
    int mUnsortedCount, mUnsortedSize;
    SortCacheItem **mUnsortedChildren;
    bool mImperfectlyThreaded;
};


/**
 * Visual representation of a member of the set of displayables (mails in 
 * the current folder). Each item is paired with a sort cache item. See above
 * how they are meant to cooperate. This should be about the visual aspects of
 * displaying an entry only. */
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

  void setSortCacheItem( SortCacheItem *item ) { mSortCacheItem = item; }
  SortCacheItem* sortCacheItem() const { return mSortCacheItem; }

private:
  int mMsgId;
  QString mKey;
  bool mAboutToBeDeleted;
  SortCacheItem *mSortCacheItem;
}; // End of class HeaderItem

} // End of namespace KMail


#endif // HEADERITEM_H
