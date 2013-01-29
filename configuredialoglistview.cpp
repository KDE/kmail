#include "configuredialoglistview.h"

ListView::ListView( QWidget *parent )
  : QTreeWidget( parent )
{
  setAllColumnsShowFocus( true );
  setAlternatingRowColors( true );
  setSelectionMode( QAbstractItemView::SingleSelection );
  setRootIsDecorated( false );
}


void ListView::resizeEvent( QResizeEvent *e )
{
  QTreeWidget::resizeEvent(e);
  resizeColums();
}

void ListView::showEvent( QShowEvent *e )
{
  QTreeWidget::showEvent(e);
  resizeColums();
}


void ListView::resizeColums()
{
  const int c = columnCount();
  if( c == 0 ) {
    return;
  }

  const int w1 = viewport()->width();
  const int w2 = w1 / c;
  const int w3 = w1 - (c-1)*w2;

  for ( int i=0; i<c-1; i++ ) {
    setColumnWidth( i, w2 );
  }
  setColumnWidth( c-1, w3 );
}


#include "configuredialoglistview.moc"
