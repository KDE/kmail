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
public:
  KMMimePartTree( KMReaderWin* readerWin,
                  QWidget* parent,
                  const char* name = 0 );

protected slots:
  void itemClicked( QListViewItem* );
  void itemRightClicked( QListViewItem*, const QPoint& );
  void slotSaveAs();
  void slotSaveAsEncoded();
    
protected:
  KMReaderWin* mReaderWin;
  KMMimePartTreeItem* mCurrentContextMenuItem;
};

class KMMimePartTreeItem :public QListViewItem
{
public:
  KMMimePartTreeItem( KMMimePartTree& parent,
                      partNode* node,
                      const QString& labelDescr,
                      QString labelCntType  = QString::null,
                      QString labelEncoding = QString::null,
                      KIO::filesize_t size=0 );
  KMMimePartTreeItem( KMMimePartTreeItem& parent,
                      partNode* node,
                      const QString& labelDescr,
                      QString labelCntType  = QString::null,
                      QString labelEncoding = QString::null,
                      KIO::filesize_t size=0  );
  partNode* node() const { return mPartNode; }

private:
  partNode* mPartNode;
};

#endif // KMMIMEPARTTREE_H
