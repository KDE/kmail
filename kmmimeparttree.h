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



// -*- c++ -*-
#ifndef KMMIMEPARTTREE_H
#define KMMIMEPARTTREE_H

#include <klistview.h>
#include <kio/global.h>

#include <qstring.h>

class partNode;
class KMReaderWin;
class KMMimePartTreeItem;

class KMMimePartTree : public KListView
{
  Q_OBJECT
  friend class KMReaderWin;

public:
  KMMimePartTree( KMReaderWin* readerWin,
                  QWidget* parent,
                  const char* name = 0 );
  virtual ~KMMimePartTree();

  void correctSize( QListViewItem * item );

protected slots:
  void itemClicked( QListViewItem* );
  void itemRightClicked( QListViewItem*, const QPoint& );
  void slotSaveAs();
  void slotSaveAsEncoded();
  void slotSaveAll();

protected:
  /** reimplemented in order to update the frame width in case of a changed
      GUI style */
  void styleChange( QStyle& oldStyle );

  /** Set the width of the frame to a reasonable value for the current GUI
      style */
  void setStyleDependantFrameWidth();

  void saveSelectedBodyParts( bool encoded );
  void restoreLayoutIfPresent();

protected:
  KMReaderWin* mReaderWin;
  KMMimePartTreeItem* mCurrentContextMenuItem;
  int mSizeColumn;
};

class KMMimePartTreeItem :public QListViewItem
{
public:
  KMMimePartTreeItem( KMMimePartTree * parent,
                      partNode* node,
                      const QString & labelDescr,
                      const QString & labelCntType  = QString::null,
                      const QString & labelEncoding = QString::null,
                      KIO::filesize_t size=0 );
  KMMimePartTreeItem( KMMimePartTreeItem * parent,
                      partNode* node,
                      const QString & labelDescr,
                      const QString & labelCntType  = QString::null,
                      const QString & labelEncoding = QString::null,
                      KIO::filesize_t size=0,
                      bool revertOrder = false );
  partNode* node() const { return mPartNode; }

  KIO::filesize_t origSize() const { return mOrigSize; }
  void setOrigSize( KIO::filesize_t size ) { mOrigSize = size; }

private:
  void setIconAndTextForType( const QString & mimetype );

  partNode* mPartNode;
  KIO::filesize_t mOrigSize;
};

#endif // KMMIMEPARTTREE_H
