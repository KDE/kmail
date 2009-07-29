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
  int c = columnCount();
  if( c == 0 )
  {
    return;
  }

  int w1 = viewport()->width();
  int w2 = w1 / c;
  int w3 = w1 - (c-1)*w2;

  for( int i=0; i<c-1; i++ )
  {
    setColumnWidth( i, w2 );
  }
  setColumnWidth( c-1, w3 );
}

QSize ListView::sizeHint() const
{
  QSize s = QTreeWidget::sizeHint();

  // FIXME indentation is horizontal distance
  /*int h = fontMetrics().height() + 2*indentation();
  if( h % 2 > 0 ) { h++; }

  s.setHeight( h*mVisibleItem + lineWidth()*2 + header()->sizeHint().height());*/
  return s;
}

