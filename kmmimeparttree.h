#ifndef KMMIMEPARTTREE_H
#define KMMIMEPARTTREE_H

#include <klistview.h>
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
                      QString labelSize     = QString::null );
  KMMimePartTreeItem( KMMimePartTreeItem& parent,
                      partNode* node,
                      const QString& labelDescr,
                      QString labelCntType  = QString::null,
                      QString labelEncoding = QString::null,
                      QString labelSize     = QString::null );
  partNode* node() const { return mPartNode; }

private:
  partNode* mPartNode;
};

#endif // KMMIMEPARTTREE_H
