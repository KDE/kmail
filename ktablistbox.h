/* This file is part of the KDE libraries
    Copyright (C) 1997 The KDE Team

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
/* Written by Stefan Taferner <taferner@kde.org>
 *            Alessandro Russo <axela@bigfoot.com>
 * A multi column listbox. Requires the Qt widget set.
 */
#ifndef KTabListBox_h
#define KTabListBox_h

#ifdef __GNUC__
#warning This is obsolete, use QListView instead
#endif

#undef del_item
#include <qdict.h>
#include <qtableview.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <stdlib.h>
#include <qstrlist.h>

#define MAX_SEP_CHARS 16

class KTabListBoxColumn;
class KTabListBoxTable;
class KTabListBoxItem;
class KTabListBox;
class KNumCheckButton;

typedef QDict<QPixmap> KTabListBoxDict;

//-----------------------------------------------------
/**
* Provides a different type of Check button.
* 
* @internal
* @deprecated
*/
class KNumCheckButton : public QWidget
{
    Q_OBJECT
public:
    KNumCheckButton( QWidget *_parent = 0L, const char *name = 0L );
    ~KNumCheckButton() {};
    void setText( const QString& );
     
signals:
    void        selected();
    void        deselected();    
    /** leftbutton=true if is a leftbutton, =false if is a rightbutton
    *   centerbutton doubleclick events aren't emitted.
    */
    void	doubleclick(bool leftbutton);

protected:
    virtual void leaveEvent( QEvent *_ev );
    virtual void enterEvent( QEvent *_ev );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseDoubleClickEvent( QMouseEvent * );
    virtual void paintEvent( QPaintEvent *event);    
    
 private:    
    bool raised;
    QString     btext;
};


//--------------------------------------------------
#define KTabListBoxTableInherited QTableView

/**
 * @internal
 * @deprecated
 */
class KTabListBoxTable: public QTableView
{
  Q_OBJECT
  friend KTabListBox;
public:
  KTabListBoxTable(KTabListBox *owner=0);
  virtual ~KTabListBoxTable();
  void enableKey();
  int findRealCol(int x);

protected:
  virtual void focusInEvent(QFocusEvent*);
  virtual void focusOutEvent(QFocusEvent*);
  virtual void mouseDoubleClickEvent (QMouseEvent*);
  virtual void mousePressEvent (QMouseEvent*);
  virtual void mouseReleaseEvent (QMouseEvent*);
  virtual void mouseMoveEvent (QMouseEvent*);
  virtual void doItemSelection (QMouseEvent*, int idx);

  virtual void paintCell (QPainter*, int row, int col);
  virtual int cellWidth (int col);

  void reconnectSBSignals ();

  QPoint dragStartPos;
  int dragCol;
  int dragRow;
  int selIdx;
};


//--------------------------------------------------
/** A multi column listbox
 * Features:
 *  - User resizeable columns.
 *  - The order of columns can be changed with drag&drop. (Alex)
 *  - 3 modes: Standard, SimpleOrder, ComplexOrder. (Alex)
 * ToDo: 
 *  - Configurable vertical column divisor lines. 
 *  - Save all setting to config file.
 *  - fix flickering into column headers.
 *  @deprecated
 */
class KTabListBox : public QWidget
{
  Q_OBJECT
  friend KTabListBoxTable;
  friend KTabListBoxColumn;

public:
  enum ColumnType { TextColumn, PixmapColumn, MixedColumn };
  enum OrderMode { Ascending, Descending };
  enum OrderType { NoOrder, SimpleOrder, ComplexOrder };

  KTabListBox (QWidget *parent=0, const char *name=0, 
	       int columns=1, WFlags f=0);
  virtual ~KTabListBox();

  /** This enable the key-bindings (and set StrongFocus!)
   * if you don't want StrongFocus you can implement your own keyPressEvent
   * and send an event to KTabListBox from there... */
  void enableKey() { lbox.enableKey(); }
  
  /** Returns the number of rows */
  uint count () const { return numRows(); };
  
  /** Insert a line before given index, using the separator character to separate the fields. If no index is given the line is appended at the end. Returns index of inserted item. */
  virtual void insertItem (const QString& string, int itemIndex=-1);

  /** Append a QStrList */
  void appendStrList( QStrList const *strLst );

  /** Same as insertItem, but always appends the new item. */
  void appendItem (const QString& string) { insertItem(string); }

  /** Change contents of a line using the separator character to separate the fields. */
  virtual void changeItem (const QString& string, int itemIndex);

  /** Change part of the contents of a line. */
  virtual void changeItemPart (const QString& string, int itemIndex, int column);

  /** Change color of line. Changes last inserted item when itemIndex==-1 */
  virtual void changeItemColor (const QColor& color, int itemIndex=-1);

  /** Get number of pixels one tab character stands for. Default: 10 */
  int tabWidth() const { return tabPixels; }
  /** Set number of pixels one tab character stands for. Default: 10 */
  virtual void setTabWidth(int);

  /** Returns contents of given row/column. If col is not set the
   contents of the whole row is returned, seperated with the current 
   seperation character. In this case the string returned is a 
   temporary string that will change on the next text() call on any
   KTabListBox object. */
  const QString& text(int idx, int col=-1) const;

  /** Remove one item from the list. */
  virtual void removeItem (int itemIndex);

  /** Remove contents of listbox */
  virtual void clear ();

  /** Return index of current item */
  int currentItem () const { return current; }

  /** Set the current (selected) column. colId is the value that
    is transfered with the selected() signal that is emited. */
  virtual void setCurrentItem (int idx, int colId=-1);

  /** Unmark all items */
  virtual void unmarkAll ();

  /** Mark/unmark item with index idx. */
  virtual void markItem (int idx, int colId=-1);
  virtual void unmarkItem (int idx);

  /** Returns TRUE if item with given index is marked. */
  virtual bool isMarked (int idx) const;

  /** Find item at given screen y position. */
  int findItem (int yPos) const { return (itemShowList[lbox.findRow(yPos)]); }

  /** Returns first item that is currently displayed in the widget. */
  int topItem () const { return (itemShowList[lbox.topCell()]); }

  /** Change first displayed item by repositioning the visible part
    of the list. */
  void setTopItem (int idx) { lbox.setTopCell(itemPosList(idx)); }

  /** Set number of columns. Warning: this *deletes* the contents
    of the listbox. */
  virtual void setNumCols (int);

  /** Set number of rows in the listbox. The contents stays as it is. */
  virtual void setNumRows (int);

  /** See the docs for the QTableView class. */
  int numRows () const { return lbox.numRows(); }
  /** See the docs for the QTableView class. */
  int numCols () const { return lbox.numCols(); }
  /** See the docs for the QTableView class. */
  int cellWidth (int col) { return lbox.cellWidth(col); }
  /** See the docs for the QTableView class. */
  int totalWidth () { return lbox.totalWidth(); }
  /** See the docs for the QTableView class. */
  int cellHeight (int row) { return lbox.cellHeight(row); }
  /** See the docs for the QTableView class. */
  int totalHeight () { return lbox.totalHeight(); }
  /** See the docs for the QTableView class. */
  int topCell () const { return itemShowList[lbox.topCell()]; }
  /** See the docs for the QTableView class. */
  int leftCell () const { return colShowList[lbox.leftCell()]; }
  /** See the docs for the QTableView class. */
  int lastColVisible () const { return colShowList[lbox.lastColVisible()]; }
  /** See the docs for the QTableView class. */
  int lastRowVisible () const { return itemShowList[lbox.lastRowVisible()]; }
  /** See the docs for the QTableView class. */
  bool autoUpdate () const { return lbox.autoUpdate(); }
  /** See the docs for the QTableView class. */
  void setAutoUpdate (bool upd) { lbox.setAutoUpdate(upd); }
  /** See the docs for the QTableView class. */
  void clearTableFlags(uint f=~0) { lbox.clearTableFlags(f); }
  /** See the docs for the QTableView class. */
  uint tableFlags() { return lbox.tableFlags(); }
  /** See the docs for the QTableView class. */
  bool testTableFlags(uint f) { return lbox.testTableFlags(f); }
  /** See the docs for the QTableView class. */
  void setTableFlags(uint f) { lbox.setTableFlags(f); }
  /** See the docs for the QTableView class. */
  int findCol(int x) { return lbox.findRealCol(x); }
  /** See the docs for the QTableView class. */
  int findRow(int y) { return itemShowList[lbox.findRow(y)]; }
  /** See the docs for the QTableView class. */
  bool colXPos(int col, int* x) { return lbox.colXPos(colPosList(col),x); }
  /** See the docs for the QTableView class. */
  bool rowYPos(int row, int* y) { return lbox.rowYPos(itemPosList(row),y); }

  /** This call the 'compar' functions if they were been defined in 
  * setColumn or else use strcmp. (i.e. if you want a case-insensitive sort
  * put strcasecmp in setColumn call).
  * That compar function must take as arguments two char *, and must return
  * an integer less  than, equal  to,  or  greater than zero if the first
  * argument is considered to be respectively  less  than,  equal  to, 
  * or greater than the second. */
  virtual void reorderRows();
  
  /** Set column caption, width, type,order-type and order-mode */
  virtual void setColumn (int col, const QString& caption, 
			  int width=0, ColumnType type=TextColumn,
			  OrderType ordt=NoOrder,
			  OrderMode omode=Descending,
			  bool verticalLine=false,
			  int (*compar)(const QString&, const QString&)=0L);

  /** Set column width. */
  virtual void setColumnWidth (int col, int width=0);
  /** Get column width. */
  int columnWidth (int col) { return lbox.cellWidth(col); }

  /** Set default width of all columns. */
  virtual void setDefaultColumnWidth(int width0, ...);

  /** change the Ascending/Descending mode of column col.*/
  void changeMode(int col);
  /** Clear all number-check-buttons (ComplexOrder only) */
  void clearAllNum();
  
  /** Set separator character, e.g. '\t'. */
  virtual void setSeparator (char sep) { sepChar = sep; } 

  /** Return separator character. */
  virtual char separator () const { return sepChar; }

  /** For convenient access to the dictionary of pictures that this listbox understands. */
  KTabListBoxDict& dict () { return pixDict; }

  void repaint ();

  QPixmap& dndPixmap() { return dndDefaultPixmap; }

  /** Read the config file entries in the group with the name of the listbox and set the default column widths and those. */
  virtual void readConfig();

  /** Write the config file entries in the group with the name of the listbox*/
  virtual void writeConfig();
  
  /** Return the actual position of the colum in the table.*/
  int colPosList(int num);
  
  /** Return the actual positon of the row number num.*/
  int itemPosList(int num);

  /** Get/set font of the table. font() and setFont() apply to the
    caption only. */
  QFont tableFont() const { return lbox.font(); }
  void setTableFont(const QFont& fnt) { lbox.setFont(fnt); }
  
signals:
  /** emited when the current item changes (either via setCurrentItem() or via mouse single-click). */
  void highlighted (int Index, int column);

  /** emitted when the user double-clicks into a line. */
  void selected (int Index, int column);

  /** emitted when the user presses the right mouse button over a line. */
  void popupMenu (int Index, int column);

  /** emitted when the user presses the middle mouse button over a line. */
  void midClick (int Index, int column);

  /** emitted when the user clicks on a column header. */
  void headerClicked (int column);

protected slots:
  void horSbValue(int val);
  void horSbSlidingDone();

protected:
  /** Used to create new column objects. Overwrite this method
   * in a subclass to have your own column objects (e.g. with custom
   * data in it). You will then also need customData()/setCustomData()
   * methods in here that access the elememts in itemList[]. */
  virtual KTabListBoxColumn* newKTabListBoxColumn();

  bool itemVisible (int idx) { return lbox.rowIsVisible(idx); }
  void updateItem (int idx, bool clear = TRUE);
  bool needsUpdate (int id);

  /// Internal method called by keyPressEvent.
  void setCItem  (int idx);
  /// Adjust the number in the number check boxes.
  void adjustNumber(int num);
  // This should really go into kdecore and should be used by all
  // apps that have long scrollable widgets.
  void flushKeys();
  
  /// For internal use.
  bool recursiveSort(int level,int n,KTabListBoxColumn **c,int *iCol);
  
  KTabListBoxItem* getItem (int idx);
  const KTabListBoxItem* getItem (int idx) const;

  virtual void keyPressEvent(QKeyEvent*);
  virtual void resizeEvent (QResizeEvent*);
  virtual void paintEvent (QPaintEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);

  /** Resize item array. Per default enlarge it to double size. */
  virtual void resizeList (int newNumItems=-1);

  /** Internal method that handles resizing of columns with the mouse. */
  virtual void doMouseResizeCol(QMouseEvent*);

  /** Internal method that handles moving of columns with the mouse. */
  virtual void doMouseMoveCol(QMouseEvent*);

  /** sets up a drag object and starts a drag operation. */
  bool startDrag(int aRow, int aCol);

  // This array contain the list of columns as they are inserted.
  KTabListBoxColumn**	colList;
  // This array contain the column numbers as they are shown.
  int *colShowList;
  // This array contain the row numbers as they are shown.
  int *itemShowList;
  KTabListBoxItem**	itemList;
  int			maxItems;
  int           numColumns;
  int			current;
  char			sepChar;
  KTabListBoxDict	pixDict;
  KTabListBoxTable	lbox;
  int			labelHeight;
  
  QPixmap		dndDefaultPixmap;
  QPixmap		upPix;
  QPixmap       downPix;
  QPixmap		disabledUpPix;
  QPixmap       disabledDownPix;
  int			columnPadding;
  QColor		highlightColor;
  int			tabPixels;
  bool			mResizeCol;
  bool			stopOrdering;
  bool			needsSort;
  /// contain the number of the last column where the user clicked on checkbutton.
  int                   lastSelectedColumn;
  int			nMarked; //number of marked rows.
  int			mSortCol;  // selected column for sorting order or -1

  int		mMouseCol; // column where the mouse action started
			   // (resize, click, reorder)
  int		mMouseColLeft; // left offset of mouse column
  int		mLastX; // Used for drawing the XORed rect in column-drag.
  int		mMouseColWidth; // width of mouse column
  QPoint		mMouseStart;
  bool		mMouseAction;
  bool		mMouseDragColumn; // true when dragging the header of a column.

private:
  // Disabled copy constructor and operator=
  KTabListBox (const KTabListBox &);
  KTabListBox& operator= (const KTabListBox&);
};


//--------------------------------------------------
/**
 * @internal
 * @deprecated
 */
class KTabListBoxItem
{
public:
  KTabListBoxItem(int numColumns=1);
  virtual ~KTabListBoxItem();

  virtual const QString& text(int column) const { return txt[column]; }
  void setText (int column, const QString& text) { txt[column] = text; }
  virtual void setForeground (const QColor& fg ) { fgColor = fg; }
  const QColor& foreground () { return fgColor; }

  KTabListBoxItem& operator= (const KTabListBoxItem&);

  int marked () const { return mark; }
  bool isMarked () const { return (mark >= -1); }
  virtual void setMarked (int m) { mark = m; }
 
private:
  QString* txt;
  int columns;
  QColor fgColor;
  int mark;

  friend class KTabListBox;
};

typedef KTabListBoxItem* KTabListBoxItemPtr;


//--------------------------------------------------

/**
 * @internal
 * @deprecated
 */
class KTabListBoxColumn: public QObject
{
  Q_OBJECT

public:
  KTabListBoxColumn (KTabListBox* parent, int w=0);
  virtual ~KTabListBoxColumn();

  int width () const { return iwidth; }
  virtual void setWidth (int w) { iwidth = w; }

  int defaultWidth () const { return idefwidth; }
  virtual void setDefaultWidth (int w) { idefwidth = w; }

  virtual void setType (KTabListBox::ColumnType lbt) { colType = lbt; }
  KTabListBox::ColumnType type () const { return colType; }

  /// @return true if need repaint.
  virtual bool changeMode ();
  
  virtual void setNumber(int num);
  int number () const { return inumber;}
  void hideCheckButton() { if(mbut) mbut->hide(); }
  virtual void setOrder (KTabListBox::OrderType type,
                             KTabListBox::OrderMode mode);
  KTabListBox::OrderType orderType () const {return ordtype;}
  KTabListBox::OrderMode orderMode () const {return ordmode;}
  
  virtual void paintCell (QPainter*, int row, const QString& string, 
			  bool marked);
  virtual void paint (QPainter*);

protected slots:
  virtual void setButton();
  virtual void resetButton();
  virtual void clearAll(bool leftbutton);
  
protected:
  int iwidth;
  int idefwidth;
  int inumber;
  KTabListBox::OrderMode ordmode;
  KTabListBox::OrderType ordtype;
  KTabListBox::ColumnType colType;
  KTabListBox* parent;
  KNumCheckButton *mbut;
  QString caption;
 
public:
  int (*columnSort)(const QString&, const QString&);
  bool vline; // if true print a vertical line to the end of column.

  const QString& getCaption() const { return caption; }
  void setCaption(const QString& c) { caption = c; }
};

typedef KTabListBoxColumn* KTabListBoxColumnPtr;

/*
inline KTabListBoxItem* KTabListBox :: getItem (int idx)
{
    return ((idx>=0 && idx<maxItems) ? itemList[idx] : (KTabListBoxItem*)0L);
}

inline const KTabListBoxItem* KTabListBox :: getItem (int idx) const
{
  return ((idx>=0 && idx<maxItems) ? itemList[idx] : (KTabListBoxItem*)0L);
}
*/

#endif /*KTabListBox_h*/
