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

protected slots:
  void itemClicked( QListViewItem* );
  void itemRightClicked( QListViewItem*, const QPoint& );
  void slotSaveAs();
  void slotSaveAsEncoded();
  void slotSaveAll();

protected:
  void saveOneFile( QListViewItem* item, bool encoded );
  void saveMultipleFiles( const QPtrList<QListViewItem>& selected, bool encoded );
  void restoreLayoutIfPresent();

protected:
  KMReaderWin* mReaderWin;
  KMMimePartTreeItem* mCurrentContextMenuItem;
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

private:
  void setIconAndTextForType( const QString & mimetype );

  partNode* mPartNode;
};

#endif // KMMIMEPARTTREE_H
