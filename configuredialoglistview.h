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

  QSize sizeHint() const;

protected:
  void resizeEvent( QResizeEvent *e );
  void showEvent( QShowEvent *e );

};

#endif // configuredialoglistview_h
