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

#include <stdio.h>
#include <stdlib.h>

#include <k3listview.h> // include for the base class
#include <QPixmap>
#include <QList>

class KMMsgBase;
namespace KPIM {
  struct KPaintInfo;
}
class KMFolder;
class KMHeaders;

namespace KMail
{
class HeaderItem; // forward declaration

/**
 * @short Represents an item in the set of mails to be displayed but only as far
 * as sorting, threading and reading/writing of the current sort order to
 * a disk cache is concerned.
 *
 * Each such item is paired with a KMail::HeaderItem, which holds the graphical
 * representation of each item (mail). This is what the threading trees are
 * built of.
 */
class SortCacheItem {

public:
    SortCacheItem() : mItem(0), mParent(0), mId(-1), mSortOffset(-1),
        mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0),
        mImperfectlyThreaded (true), mSubjThreadingList(0) { }
    SortCacheItem(int i, QString k, int o=-1)
        : mItem(0), mParent(0), mId(i), mSortOffset(o), mKey(k),
          mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0),
          mImperfectlyThreaded (true), mSubjThreadingList(0) { }
    ~SortCacheItem() { if(mUnsortedChildren) free(mUnsortedChildren); }

    /** The parent node of the item in the threading hierarchy. 0 if the item
     * is at top level, which is the default. Can only be set by parents. */
    SortCacheItem *parent() const { return mParent; }
    /**
     * Returs whether the item is so far imperfectly threaded.
     * If an item is imperfectly threaded (by References or subject, not by
     * In-Reply-To) it will be reevalutated when a new mail comes in. It could be
     * the perfect parent. */
    bool isImperfectlyThreaded() const
        { return mImperfectlyThreaded; }
    /** Set whether the item is currently imperfectly threaded (by References
     * or Subject, not by In-Reply-To). */
    void setImperfectlyThreaded (bool val)
        { mImperfectlyThreaded = val; }
    /** Returns whether the item has other items below it. */
    bool hasChildren() const
        { return mSortedChildren.count() || mUnsortedCount; }
    /** The sorted children are an array of sortcache items we know are below the
     * current one and are already properly sorted (as read from the cache ) */
    const QList<SortCacheItem*> *sortedChildren() const
        { return &mSortedChildren; }
    /** The unsorted children are an array of sortcache items we know are below the
     * current one, but are yet to be threaded and sorted properly. */
    SortCacheItem **unsortedChildren(int &count) const
        { count = mUnsortedCount; return mUnsortedChildren; }
    /** Add an item to this itme's list of already sorted children. */
    void addSortedChild(SortCacheItem *i) {
        i->mParent = this;
        mSortedChildren.append(i);
    }
    /** Add an item to this itme's list of unsorted children. */
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

    /** The corresponding KMail::HeaderItem */
    HeaderItem *item() const { return mItem; }
    /** Set the corresponding KMail::HeaderItem */
    void setItem(HeaderItem *i) { Q_ASSERT(!mItem); mItem = i; }

    /** sort key as used by the listview */
    const QString &key() const { return mKey; }
    /** Set the sort key used by the list view. */
    void setKey(const QString &key) { mKey = key; }

    int id() const { return mId; }
    void setId(int id) { mId = id; }

    /** offset in the cache file stream */
    int offset() const { return mSortOffset; }
    void setOffset(int x) { mSortOffset = x; }

    void updateSortFile( FILE *sortStream, KMFolder *folder,
                         bool waiting_for_parent = false,
                         bool update_discovered_count = false);

    /** Set the list of mails with a certain subject that this item is on.
     * Used to remove the item from that list on deletion. */
    void setSubjectThreadingList( QList<SortCacheItem*> *list ) { mSubjThreadingList = list; }
    /** The list of mails with a certain subject that this item is on. */
    QList<SortCacheItem*>* subjectThreadingList() const { return mSubjThreadingList; }

private:
    HeaderItem *mItem;
    SortCacheItem *mParent;
    int mId, mSortOffset;
    QString mKey;

    QList<SortCacheItem*> mSortedChildren;
    int mUnsortedCount, mUnsortedSize;
    SortCacheItem **mUnsortedChildren;
    bool mImperfectlyThreaded;
    // pointer to the list it might be on so it can be remove from it
    // when the item goes away.
    QList<SortCacheItem*>* mSubjThreadingList;
};


/**
 * Visual representation of a member of the set of displayables (mails in
 * the current folder). Each item is paired with a KMail::SortCacheItem. See there as to
 * how they are meant to cooperate. This should be about the visual aspects of
 * displaying an entry only. */
class HeaderItem : public K3ListViewItem
{
public:
  HeaderItem( Q3ListView* parent, int msgId, const QString& key = QString() );
  HeaderItem( Q3ListViewItem* parent, int msgId, const QString& key = QString() );
  ~HeaderItem ();

  /** Set the message id of this item, which is the offset/index in the folder
   * currently displayed by the KMHeaders list view. */
  void setMsgId( int aMsgId );

  // Profiling note: About 30% of the time taken to initialize the
  // listview is spent in this function. About 60% is spent in operator
  // new and QListViewItem::QListViewItem.
  void irefresh();

  /** Return the msgId of the message associated with this item. */
  int msgId() const;

  // Return the serial number of the message associated with this item;
  quint32 msgSerNum() const;

  /** Expands all children of the list view item. */
  void setOpenRecursive( bool open );

  /** Returns the text of the list view item. */
  QString text( int col) const;

  void setup();

  typedef QList<QPixmap> PixmapList;

  QPixmap pixmapMerge( PixmapList pixmaps ) const;

  const QPixmap *cryptoIcon(KMMsgBase *msgBase) const;
  const QPixmap *signatureIcon(KMMsgBase *msgBase) const;
  const QPixmap *statusIcon(KMMsgBase *msgBase) const;

  const QPixmap *pixmap(int col) const;

  void paintCell( QPainter * p, const QColorGroup & cg,
                                int column, int width, int align );

  static QString generate_key( KMHeaders *headers, KMMsgBase *msg, const KPIM::KPaintInfo *paintInfo, int sortOrder );

  virtual QString key( int column, bool /*ascending*/ ) const;

  void setTempKey( const QString &key );

  int compare( Q3ListViewItem *i, int col, bool ascending ) const;

  Q3ListViewItem* firstChildNonConst(); /* Non const! */

  /** Returns whether the item is about to be removed from the list view as a
   * result of some user action. Such items are not selectable and painted with
   * a strike-through decoration. */
  bool aboutToBeDeleted() const { return mAboutToBeDeleted; }
  /** Set the item to be in about-to-be-deleted state, which means it
   * cannot be selected and will be painted with a strike-through decoration. */
  void setAboutToBeDeleted( bool val ) { mAboutToBeDeleted = val; }

  /** Associate a KMail::SortCacheItem with this item. This is the structure used to
   * represent the mail during sorting and threading calculation. */
  void setSortCacheItem( SortCacheItem *item ) { mSortCacheItem = item; }
  /** Returns the KMail::SortCacheItem associated with this display item. */
  SortCacheItem* sortCacheItem() const { return mSortCacheItem; }

private:
  int mMsgId;
  quint32 mSerNum;
  QString mKey;
  bool mAboutToBeDeleted;
  SortCacheItem *mSortCacheItem;
}; // End of class HeaderItem

} // End of namespace KMail


#endif // HEADERITEM_H
