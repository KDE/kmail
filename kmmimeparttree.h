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

  void saveOneFile( QListViewItem* item, bool encoded );
  void saveMultipleFiles( const QPtrList<QListViewItem>& selected, bool encoded );
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
