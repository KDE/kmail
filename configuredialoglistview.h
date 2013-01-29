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

Q_SIGNALS:
  void addHeader();
  void removeHeader();

protected:
  void resizeEvent( QResizeEvent *e );
  void showEvent( QShowEvent *e );

private Q_SLOTS:
  void slotContextMenu(const QPoint& pos);

private:
  void resizeColums();

};

#endif // configuredialoglistview_h
