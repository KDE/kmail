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

// -*- c++ -*-
#ifndef KMMIMEPARTTREE_H
#define KMMIMEPARTTREE_H

#include <kio/global.h>

#include <libkdepim/treewidget.h>
#include <QString>

class partNode;
class KMReaderWin;
class KMMimePartTreeItem;

/**
 * @short The widget that displays the message MIME tree
 *
 * This widget displays the message tree and allows viewing
 * the contents of single nodes in the KMReaderWin.
 */
class KMMimePartTree : public KPIM::TreeWidget
{
  Q_OBJECT
  friend class ::KMReaderWin;
  friend class KMMimePartTreeItem;

public:
  KMMimePartTree( KMReaderWin *readerWin,
                  QWidget *parent );
  virtual ~KMMimePartTree();

public:
  /**
  * Clears the tree view by removing all the items.
  * Resets sort order to "insertion order".
  */
  void clearAndResetSortOrder();

protected slots:
  void slotItemClicked( QTreeWidgetItem * );
  void slotContextMenuRequested( const QPoint & );
  void slotSaveAs();
  void slotSaveAsEncoded();
  void slotSaveAll();
  void slotDelete();
  void slotEdit();
  void slotOpen();
  void slotOpenWith();
  void slotView();
  void slotProperties();
  void slotCopy();

protected:
  virtual void startDrag( Qt::DropActions actions );
  virtual void showEvent( QShowEvent* e );

  void restoreLayoutIfPresent();
  void startHandleAttachmentCommand( int action );
  void saveSelectedBodyParts( bool encoded );

protected:
  KMReaderWin* mReaderWin;         // pointer to the main reader window
  bool mLayoutColumnsOnFirstShow;  // provide layout defaults on first show ?

};


/**
 * @short The message MIME tree items used in KMMimePartTree
 *
 * Each display item has a reference to the partNode (which is a node
 * of the "real" message tree).
 */
class KMMimePartTreeItem : public QTreeWidgetItem
{
public:
  KMMimePartTreeItem( KMMimePartTree *parent,
                      partNode *node,
                      const QString &labelDescr,
                      const QString &labelCntType  = QString(),
                      const QString &labelEncoding = QString(),
                      KIO::filesize_t size = 0 );

  KMMimePartTreeItem( KMMimePartTreeItem * parent,
                      partNode *node,
                      const QString &labelDescr,
                      const QString &labelCntType  = QString(),
                      const QString &labelEncoding = QString(),
                      KIO::filesize_t size = 0,
                      bool revertOrder = false );

public:
  /**
  * @returns a pointer to the partNode this item is displaying.
  */
  partNode* node() const
    { return mPartNode; }

  /**
  * @returns the current size of the tree node data.
  */
  KIO::filesize_t dataSize() const
    { return mDataSize; }

  /**
  * Sets the current size of the tree node data (does NOT update the item text)
  */
  void setDataSize( KIO::filesize_t size )
    { mDataSize = size; }

  /**
   * Operator used for item sorting.
   */
  virtual bool operator < ( const QTreeWidgetItem &other ) const;

private:
  void setIconAndTextForType( const QString &mimetype );
  /**
   * Correct the dataSize() for this item by accounting
   * for the size of the children items.
   */
  void correctSize();

private:
  partNode* mPartNode;
  KIO::filesize_t mDataSize;
};

#endif // KMMIMEPARTTREE_H
