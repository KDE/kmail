// $Id$

#include "ktablistbox.h"
#include <qfontmet.h>
#include <qpainter.h>
#include <qkeycode.h>
#include <qpixmap.h>
#include <qapp.h>
#include <qdrawutl.h>

#define INIT_MAX_ITEMS 16

#include "ktablistbox.moc"

//=============================================================================
//
//  C L A S S   KTabListBoxItem
//
//=============================================================================
KTabListBoxItem :: KTabListBoxItem (int aColumns)
{
  columns = aColumns;
  txt = new QString[columns];
}


KTabListBoxItem :: ~KTabListBoxItem ()
{
  if (txt) delete[] txt;
  txt = NULL;
}


void KTabListBoxItem :: setForeground (const QColor& fg)
{
  fgColor = fg;
}


KTabListBoxItem& KTabListBoxItem :: operator= (const KTabListBoxItem& from)
{
  int i;

  for (i=0; i<columns; i++)
    txt[i] = from.txt[i];

  fgColor = from.fgColor;

  return *this;
}


//=============================================================================
//
//  C L A S S   KTabListBoxColumn
//
//=============================================================================
//-----------------------------------------------------------------------------
KTabListBoxColumn :: KTabListBoxColumn (KTabListBox* pa, int w): QObject()
{
  initMetaObject();
  iwidth = w;
  colType = KTabListBox::TextColumn;
  parent = pa;
}


//-----------------------------------------------------------------------------
KTabListBoxColumn :: ~KTabListBoxColumn ()
{
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn :: setWidth (int w)
{
  iwidth = w;
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn :: setType (KTabListBox::ColumnType lbt)
{
  colType = lbt;
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn :: paintCell (QPainter* paint, const QString& string)
{
  const QFontMetrics* fm = &paint->fontMetrics();
  QPixmap* pix;

  switch (colType)
  {
  case KTabListBox::PixmapColumn:
    if (string) pix = parent->dict().find(string);
    else pix = NULL;

    if (pix)
    {
      paint->drawPixmap (0, 0, *pix);
      break;
    }
    /*else output as string*/

  case KTabListBox::TextColumn:
    paint->drawText (3, fm->ascent() + (fm->leading()), 
		     (const char*)string); 
    break;
  }
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn :: paint (QPainter* paint)
{
  const QFontMetrics* fm = &paint->fontMetrics();
  paint->drawText (3, fm->ascent() + (fm->leading()), (const char*)name());
}




//=============================================================================
//
//   C L A S S   KTabListBox
//
//=============================================================================
KTabListBox :: KTabListBox (QWidget *parent, const char *name, int columns,
			    WFlags f): 
  KTabListBoxInherited (parent, name, f), lbox(this)
{
  const QFontMetrics* fm = &fontMetrics();
  initMetaObject();

  maxItems = 0;
  current  = -1;
  colList  = NULL;
  itemList = NULL;
  sepChar  = '\t';
  labelHeight = fm->height() + 4;
  
  if (columns > 0) lbox.setNumCols(columns);

  lbox.setGeometry(0, labelHeight, width(), height()-labelHeight);

}


//-----------------------------------------------------------------------------
KTabListBox :: ~KTabListBox ()
{
  if (colList)  delete[] colList;
  if (itemList) delete[] itemList;
  colList  = NULL;
  itemList = NULL;
}


//-----------------------------------------------------------------------------
void KTabListBox :: setNumRows (int aRows)
{
  lbox.setNumRows(aRows);
}


//-----------------------------------------------------------------------------
void KTabListBox :: setNumCols (int aCols)
{
  maxItems = 0;

  if (colList) delete[] colList;
  if (itemList) delete[] itemList;
  colList  = NULL;
  itemList = NULL;

  if (aCols < 0) aCols = 0;
  lbox.setNumCols(aCols);
  if (aCols <= 0) return;

  colList  = new KTabListBoxColumn[aCols](this);
  itemList = new KTabListBoxItem[INIT_MAX_ITEMS](aCols);
  maxItems = INIT_MAX_ITEMS;
}


//-----------------------------------------------------------------------------
void KTabListBox :: setColumn (int col, const char* aName, int aWidth,
			       ColumnType aType)
{
  if (col<0 || col>=numCols()) return;

  colList[col].setWidth(aWidth);
  colList[col].setName(aName);
  colList[col].setType(aType);
  update();
}


//-----------------------------------------------------------------------------
void KTabListBox :: setCurrentItem (int idx, int colId)
{
  int i;

  if (idx>=numRows()) return;

  if (idx != current)
  {
    i = current;
    current = idx;

    updateItem(i);
  }

  if (current >= 0)
  {
    updateItem(current);
    emit highlighted(current, colId);
  }
}


//-----------------------------------------------------------------------------
void KTabListBox :: insertItem (const char* aStr, int row)
{
  int i;

  if (row < 0) row = numRows();
  if (row >= maxItems) resizeList();

  for (i=numRows()-1; i>=row; i--)
    itemList[i+1] = itemList[i];

  if (current >= row) current++;

  setNumRows (numRows()+1);
  changeItem (aStr, row);

  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox :: changeItem (const char* aStr, int row)
{
  char* str;
  char  sepStr[2];
  char* pos;
  int   i;
  KTabListBoxItem* item;

  if (row < 0 || row >= numRows()) return;

  str = new char[strlen(aStr)+2];
  strcpy (str, aStr);

  sepStr[0] = sepChar;
  sepStr[1] = '\0';

  item = &itemList[row];

  pos = strtok (str, sepStr);
  for (i=0; pos && *pos && i<numCols(); i++)
  {
    item->setText (i, pos);
    pos = strtok (NULL, sepStr);
  }
  item->setForeground (black);

  if (needsUpdate(row)) lbox.repaint();

  delete str;
}


//-----------------------------------------------------------------------------
void KTabListBox :: changeItem (const char* aStr, int row, int col)
{
  if (row < 0 || row >= numRows()) return;
  if (col < 0 || col >= numCols()) return;

  itemList[row].setText (col, aStr);
  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox :: changeItemColor (const QColor& newColor, int row)
{
  if (row >= numRows()) return;
  if (row < 0) row = numRows()-1;

  itemList[row].setForeground (newColor);
  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox :: removeItem (int row)
{
  int i, nr;

  if (row < 0 || row >= numRows()) return;
  if (current > row) current--;

  nr = numRows()-1;
  for (i=row; i<nr; i++)
    itemList[i] = itemList[i+1];

  setNumRows(nr);
  if (nr==0) current = -1;

  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox :: updateItem (int row, bool erase)
{
  int i;

  for (i=numCols()-1; i>=0; i--)
    lbox.updateCell (row, i, erase);
}


//-----------------------------------------------------------------------------
void KTabListBox :: clear (void)
{
  setNumRows(0);
  current = -1;
}


//-----------------------------------------------------------------------------
void KTabListBox :: setSeparator (char sep)
{
  sepChar = sep;
}


//-----------------------------------------------------------------------------
void KTabListBox :: resizeList (int newNumItems)
{
  KTabListBoxItem* newItemList;
  int i, ih;

  if (newNumItems < 0) newNumItems = (maxItems << 1);
  if (newNumItems < INIT_MAX_ITEMS) newNumItems = INIT_MAX_ITEMS;

  newItemList = new KTabListBoxItem[newNumItems](numCols());

  ih = newNumItems<numRows() ? newNumItems : numRows();
  for (i=ih-1; i>=0; i--)
  {
    newItemList[i] = itemList[i];
  }

  delete[] itemList;
  itemList = newItemList;
  maxItems = newNumItems;

  setNumRows(ih);
}


//-----------------------------------------------------------------------------
void KTabListBox :: mouseDoubleClickEvent (QMouseEvent*)
{
}


//-----------------------------------------------------------------------------
void KTabListBox :: resizeEvent (QResizeEvent* e)
{
  KTabListBoxInherited::resizeEvent(e);

  lbox.setGeometry (0, labelHeight, e->size().width(), 
		    e->size().height()-labelHeight);
  lbox.reconnectSBSignals();
  repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox :: paintEvent (QPaintEvent* e)
{
  int i, ih, x, w;
  QPainter paint;
  QWMatrix matrix;
  QRect    clipR;
  QColorGroup colGrp;

  ih = numCols();
  x  = -lbox.xOffset();
  matrix.translate (x, 0);

  //if (!isUpdatesEnabled()) return;

  paint.begin(this);
  for (i=0; i<ih; i++)
  {
    w = colList[i].width();

    if (w + x >= 0)
    {
      clipR.setRect (x, 0, w, labelHeight);
      paint.setWorldMatrix (matrix, FALSE);
      paint.setClipRect (clipR);

      colList[i].paint (&paint);
      qDrawShadePanel (&paint, 0, 0, w, labelHeight, 
		       KTabListBoxInherited::colorGroup());
    }
    matrix.translate (w, 0);
    x += w;
  }
  paint.end();
}


//-----------------------------------------------------------------------------
void KTabListBox :: mousePressEvent (QMouseEvent* e)
{
  printf ("+%d+%d\n", e->pos().x(), e->pos().y());
}


//-----------------------------------------------------------------------------
void KTabListBox :: mouseReleaseEvent (QMouseEvent*)
{
}


//-----------------------------------------------------------------------------
void KTabListBox :: horSbValue (int val)
{
  update();
}


//-----------------------------------------------------------------------------
void KTabListBox :: horSbSlidingDone ()
{
}




//=============================================================================
//
//   C L A S S   KTabListBoxTable
//
//=============================================================================
KTabListBoxTable :: KTabListBoxTable (KTabListBox *parent):
  KTabListBoxTableInherited (parent)
{
  QFontMetrics fm = fontMetrics();

  initMetaObject();

  setTableFlags (Tbl_autoVScrollBar|Tbl_autoHScrollBar|Tbl_smoothVScrolling|
		 Tbl_clipCellPainting);

  switch (style())
  {
  case MotifStyle:
  case WindowsStyle:
    setBackgroundColor (colorGroup().base());
    setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    break;
  default:
    setLineWidth (1);
    setFrameStyle (QFrame::Panel|QFrame::Plain);
  }

  setCellWidth (0);
  setCellHeight (fm.lineSpacing() + 1);
  setNumRows (0);

  setFocusPolicy (StrongFocus);
}


//-----------------------------------------------------------------------------
KTabListBoxTable :: ~KTabListBoxTable()
{
}


//-----------------------------------------------------------------------------
void KTabListBoxTable :: paintCell (QPainter* p, int row, int col)
{
  QColor bg;
  QColorGroup g = colorGroup();
  KTabListBox* owner = (KTabListBox*)parentWidget();

  if (owner->current == row)
  {
    bg = g.background();
    p->fillRect (0, 0, cellWidth(col), cellHeight(row), bg);
  }
  p->setPen (owner->itemList[row].foreground());
  p->setBackgroundColor (g.base());

  owner->colList[col].paintCell (p, owner->itemList[row].text(col));
}


//-----------------------------------------------------------------------------
int KTabListBoxTable :: cellWidth (int col)
{
  KTabListBox* owner = (KTabListBox*)parentWidget();

  return (owner->colList ? owner->colList[col].width() : 0);
}


//-----------------------------------------------------------------------------
void KTabListBoxTable :: mouseDoubleClickEvent (QMouseEvent* e)
{
  KTabListBox* owner = (KTabListBox*)parentWidget();
  int idx, colnr;

  //mouseReleaseEvent(event);
  idx = owner->currentItem();
  colnr = findCol(e->pos().x());
  if (idx >= 0) emit owner->selected(idx,colnr);
}


//-----------------------------------------------------------------------------
void KTabListBoxTable :: mousePressEvent (QMouseEvent* e)
{
  KTabListBox* owner = (KTabListBox*)parentWidget();
  owner->setCurrentItem (findRow(e->pos().y()), findCol(e->pos().x()));
}


//-----------------------------------------------------------------------------
void KTabListBoxTable :: mouseReleaseEvent (QMouseEvent*)
{
}


//-----------------------------------------------------------------------------
void KTabListBoxTable :: reconnectSBSignals (void)
{
  QWidget* hsb = (QWidget*)horizontalScrollBar();
  KTabListBox* owner = (KTabListBox*)parentWidget();

  if (!hsb) return;

  connect(hsb, SIGNAL(valueChanged(int)), owner, SLOT(horSbValue(int)));
  connect(hsb, SIGNAL(sliderReleased()), owner, SLOT(horSbSlidingDone()));
}
