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

protected:
  void resizeEvent( QResizeEvent *e );
  void showEvent( QShowEvent *e );

private:
  void resizeColums();

};

#endif // configuredialoglistview_h
