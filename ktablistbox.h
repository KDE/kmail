/* $Id$
 * A multi column listbox. Requires the Qt widget set.
 */
#ifndef KTabListBox_h
#define KTabListBox_h

#undef del_item
#include <qdict.h>
#include <qtablevw.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <drag.h>

#define MAX_SEP_CHARS 16

class KTabListBoxColumn;
class KTabListBoxTable;
class KTabListBoxItem;
class KTabListBox;

typedef QDict<QPixmap> KTabListBoxDict;
//typedef void* KTabListBoxDict;

//--------------------------------------------------
#define KTabListBoxTableInherited QTableView
class KTabListBoxTable: public QTableView
{
  Q_OBJECT;
  friend KTabListBox;
public:
  KTabListBoxTable(KTabListBox *owner=0);
  virtual ~KTabListBoxTable();

protected:
  virtual void mouseDoubleClickEvent (QMouseEvent*);
  virtual void mousePressEvent (QMouseEvent*);
  virtual void mouseReleaseEvent (QMouseEvent*);
  virtual void mouseMoveEvent (QMouseEvent*);
  virtual void doItemSelection (QMouseEvent*, int idx);

  virtual void paintCell (QPainter*, int row, int col);
  virtual int cellWidth (int col);

  void reconnectSBSignals (void);

  static QPoint dragStartPos;
  static int dragCol, dragRow;
  static int selIdx;
  bool dragging;
};


//--------------------------------------------------
#define KTabListBoxInherited KDNDWidget
class KTabListBox : public KDNDWidget
{
  Q_OBJECT;
  friend KTabListBoxTable;
public:
  enum ColumnType { TextColumn, PixmapColumn };

  KTabListBox (QWidget *parent=0, const char *name=0, 
	       int columns=1, WFlags f=0);
  virtual ~KTabListBox();

  uint count (void) const { return numRows(); }

  virtual void insertItem (const char* string, int itemIndex=-1);
	// Insert a line before given index, using the separator character
	// to separate the fields. If no index is given the line is 
        // appended at the end. Returns index of inserted item.

  virtual void changeItem (const char* string, int itemIndex);
	// Change contents of a line using the separator character
	// to separate the fields

  virtual void changeItem (const char* string, int itemIndex, int column);
	// Change part of the contents of a line

  virtual void changeItemColor (const QColor& color, int itemIndex=-1);
	// change color of line

  virtual void removeItem (int itemIndex);

  virtual void clear (void);
	// remove contents of listbox

  int currentItem (void) const { return current; }
  virtual void setCurrentItem (int idx, int colId=-1);
	// set the current (selected) column. colId is the value that
	// is transfered with the selected() signal that is emited.

  virtual void unmarkAll (void);
	// unmark all items
  virtual void markItem (int idx, int colId=-1);
  virtual void unmarkItem (int idx);

  int findItem (int yPos) const { return (lbox.findRow(yPos)); }

  int topItem (void) const { return (lbox.topCell()); }
  void setTopItem (int idx) { lbox.setTopCell(idx); }

  virtual void setNumCols (int);
	// Set number of columns. Warning: this *deletes* the contents
	// of the listbox.

  virtual void setNumRows (int);
  int numRows (void) const { return lbox.numRows(); }
  int numCols (void) const { return lbox.numCols(); }
  int cellWidth (int col) { return lbox.cellWidth(col); }
  int totalWidth (void) { return lbox.totalWidth(); }
  int cellHeight (int row) { return lbox.cellHeight(row); }
  int totalHeight (void) { return lbox.totalHeight(); }
  int topCell (void) const { return lbox.topCell(); }
  int leftCell (void) const { return lbox.leftCell(); }
  int lastColVisible (void) const { return lbox.lastColVisible(); }
  int lastRowVisible (void) const { return lbox.lastRowVisible(); }
  bool autoUpdate (void) const { return lbox.autoUpdate(); }
  void setAutoUpdate (bool upd) { lbox.setAutoUpdate(upd); }

  virtual void setColumn (int col, const char* caption, 
			  int width=0, ColumnType type=TextColumn);
	// set column caption and width

  virtual void setColumnWidth (int col, int width=0);
  int columnWidth (int col) { return lbox.cellWidth(col); }

  virtual void setSeparator (char sep);
	// set separator characters (maximum: 16 characters)

  virtual char separator (void) const { return sepChar; }
	// returns string of separator characters

  KTabListBoxDict& dict (void) { return pixDict; }

  void repaint (void) { QWidget::repaint(); lbox.repaint(); }

  bool startDrag(int col, int row, const QPoint& mousePos);
	// Indicates that a drag has started with given item.
	// Returns TRUE if we are dragging, FALSE if drag-start failed.

  QPixmap& dndPixmap(void) { return dndDefaultPixmap; }

signals:
  void highlighted (int Index, int column);
	// emited when the current item changes (either via setCurrentItem()
	// or via mouse single-click).

  void selected (int Index, int column);
	// emitted when the user double-clicks into a line

protected slots:
  void horSbValue(int val);
  void horSbSlidingDone();

protected:
  bool itemVisible (int idx) { return lbox.rowIsVisible(idx); }
  void updateItem (int idx, bool clear = TRUE);
  bool needsUpdate (int id) { return (lbox.autoUpdate() && itemVisible(id)); }

  KTabListBoxItem* getItem (int idx);

  virtual void resizeEvent (QResizeEvent*);
  virtual void paintEvent (QPaintEvent*);

  virtual void resizeList (int newNumItems=-1);
	// Resize item array. Per default enlarge it to double size

  virtual bool prepareForDrag (int col, int row, char** data, int* size, 
			       int* type);
	// Called to set drag data, size, and type. If this method
	// returns FALSE then no drag occurs.

  KTabListBoxColumn*	colList;
  KTabListBoxItem*	itemList;
  int			maxItems, numColumns;
  int			current;
  char			sepChar;
  KTabListBoxDict	pixDict;
  KTabListBoxTable	lbox;
  int			labelHeight;
  QPixmap		dndDefaultPixmap;

private:		// Disabled copy constructor and operator=
  KTabListBox (const KTabListBox &) {}
  KTabListBox& operator= (const KTabListBox&) { return *this; }
};


//--------------------------------------------------
class KTabListBoxItem
{
public:
  KTabListBoxItem(int numColumns=1);
  virtual ~KTabListBoxItem();

  virtual const char *text(int column)   const { return txt[column]; }
  void setText (int column, const char *text) { txt[column] = text; }
  virtual void setForeground (const QColor& color);
  const QColor& foreground (void) { return fgColor; }

  KTabListBoxItem& operator= (const KTabListBoxItem&);

  int marked (void) const { return mark; }
  bool isMarked (void) const { return (mark >= -1); }
  virtual void setMarked (int mark);

private:
  QString* txt;
  int columns;
  QColor fgColor;
  int mark;

  friend class KTabListBox;
};

typedef KTabListBoxItem* KTabListBoxItemPtr;


//--------------------------------------------------
class KTabListBoxColumn: public QObject
{
  Q_OBJECT;

public:
  KTabListBoxColumn (KTabListBox* parent, int w=0);
  virtual ~KTabListBoxColumn();

  int width (void) const { return iwidth; }
  virtual void setWidth (int w);

  virtual void setType (KTabListBox::ColumnType);
  KTabListBox::ColumnType type (void) const { return colType; }

  virtual void paintCell (QPainter*, const QString& string);
  virtual void paint (QPainter*);
protected:
  int iwidth;
  KTabListBox::ColumnType colType;
  KTabListBox* parent;
};



inline KTabListBoxItem* KTabListBox :: getItem (int idx)
{
  return ((idx>=0 && idx<maxItems) ? &itemList[idx] : (KTabListBoxItem*)NULL);
}

#endif /*KTabListBox_h*/
