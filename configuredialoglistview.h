/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 */
#ifndef configuredialoglistview_h
#define configuredialoglistview_h

#include <QTreeWidget>

class ListView : public QTreeWidget {
  Q_OBJECT
public:
  explicit ListView( QWidget *parent=0 );
  void resizeColums();

  virtual QSize sizeHint() const;

protected:
  virtual void resizeEvent( QResizeEvent *e );
  virtual void showEvent( QShowEvent *e );

private:
  int mVisibleItem;
};

#endif // configuredialoglistview_h