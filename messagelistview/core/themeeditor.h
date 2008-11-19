/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_THEMEEDITOR_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_THEMEEDITOR_H__

#include "messagelistview/core/optionseteditor.h"
#include "messagelistview/core/themedelegate.h"
#include "messagelistview/core/theme.h"

#include <QTreeWidget>
#include <QLabel>
#include <QRect>

#include <KDialog>

class QPaintDevice;
class QCheckBox;

class KComboBox;
class KLineEdit;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class Item;
class GroupHeaderItem;
class MessageItem;
class ModelInvariantRowMapper;

class ThemeColumnPropertiesDialog : public KDialog
{
  Q_OBJECT
public:
  ThemeColumnPropertiesDialog( QWidget * parent, Theme::Column * column, const QString &title );

protected:
  Theme::Column * mColumn;
  KLineEdit * mNameEdit;
  QCheckBox * mVisibleByDefaultCheck;
  QCheckBox * mIsSenderOrReceiverCheck;
  KComboBox * mMessageSortingCombo;

protected slots:
  void slotOkButtonClicked();
};

class ThemePreviewDelegate : public ThemeDelegate
{
  Q_OBJECT
public:
  ThemePreviewDelegate( QAbstractItemView * parent, QPaintDevice * paintDevice );
  ~ThemePreviewDelegate();

private:
  GroupHeaderItem * mSampleGroupHeaderItem;
  MessageItem * mSampleMessageItem;
  ModelInvariantRowMapper * mRowMapper; // needed for the MessageItem above to be valid
public:
  virtual Item * itemFromIndex( const QModelIndex &index ) const;
};

class ThemePreviewWidget : public QTreeWidget
{
  Q_OBJECT
public:
  ThemePreviewWidget( QWidget * parent );
  ~ThemePreviewWidget();

private:
  // DnD insert position stuff

  /**
   * The row we'll be inserting the dragged item into
   */
  enum RowInsertPosition
  {
    AboveRow,         ///< We'll insert above the currently hit row in mDelegate
    InsideRow,        ///< We'll insert inside the currently hit row in mDelegate
    BelowRow          ///< We'll insert below the currently hit row in mDelegate
  };

  /**
   * The position in row that we'll be inserting the dragged item
   */
  enum ItemInsertPosition
  {
    OnLeftOfItem,     ///< We'll insert on the left of the selected item
    OnRightOfItem,    ///< We'll insert on the right of the selected item
    AsLastLeftItem,   ///< We'll insert as last left item of the row (rightmost left item)
    AsLastRightItem,  ///< We'll insert as last right item of the row (leftmost right item)
    AsFirstLeftItem,  ///< We'll insert as first left item of the row (leftmost)
    AsFirstRightItem  ///< We'll insert as first right item of the row (rightmost)
  };

private:
  ThemePreviewDelegate * mDelegate;
  QTreeWidgetItem * mGroupHeaderSampleItem;
  QRect mThemeSelectedContentItemRect;
  Theme::ContentItem * mSelectedThemeContentItem;
  Theme::Column * mSelectedThemeColumn;
  QPoint mMouseDownPoint;
  Theme * mTheme;
  RowInsertPosition mRowInsertPosition;
  ItemInsertPosition mItemInsertPosition;
  QPoint mDropIndicatorPoint1;
  QPoint mDropIndicatorPoint2;
  bool mFirstShow;
public:
  QSize sizeHint() const;
  void setTheme( Theme * theme );

protected:
  virtual void dragMoveEvent( QDragMoveEvent * e );
  virtual void dragEnterEvent( QDragEnterEvent * e );
  virtual void dropEvent( QDropEvent * e );
  virtual void mouseMoveEvent( QMouseEvent * e );
  virtual void mousePressEvent( QMouseEvent * e );
  virtual void paintEvent( QPaintEvent * e );
  virtual void showEvent( QShowEvent * e );

private:
  void internalHandleDragMoveEvent( QDragMoveEvent * e );
  void internalHandleDragEnterEvent( QDragEnterEvent * e );

  /**
   * Computes the drop insert position for the dragged item at position pos.
   * Returns true if the dragged item can be inserted somewhere and
   * false otherwise. Sets mRowInsertPosition, mItemInsertPosition,
   * mDropIndicatorPoint1 ,mDropIndicatorPoint2.
   */
  bool computeContentItemInsertPosition( const QPoint &pos, Theme::ContentItem::Type type );

  void applyThemeColumnWidths();

protected slots:
  void slotHeaderContextMenuRequested( const QPoint &pos );
  void slotAddColumn();
  void slotColumnProperties();
  void slotDeleteColumn();
  void slotDisabledFlagsMenuTriggered( QAction * act );
  void slotForegroundColorMenuTriggered( QAction * act );
  void slotFontMenuTriggered( QAction * act );
  void slotSoftenActionTriggered( bool );
  void slotGroupHeaderBackgroundModeMenuTriggered( QAction * act );
  void slotGroupHeaderBackgroundStyleMenuTriggered( QAction * act );
};

class ThemeContentItemSourceLabel : public QLabel
{
  Q_OBJECT
public:
  ThemeContentItemSourceLabel( QWidget * parent, Theme::ContentItem::Type type );
  ~ThemeContentItemSourceLabel();
private:
  Theme::ContentItem::Type mType;
  QPoint mMousePressPoint;
public:
  Theme::ContentItem::Type type() const
    { return mType; };
  void startDrag();
protected:
  void mousePressEvent( QMouseEvent * e );
  void mouseMoveEvent( QMouseEvent * e );
};


class ThemeEditor : public OptionSetEditor
{
  Q_OBJECT
public:
  ThemeEditor( QWidget *parent );
  ~ThemeEditor();

private:
  Theme * mCurrentTheme; // shallow, may be null!

  // Appearance tab
  ThemePreviewWidget * mPreviewWidget;

  // Advanced tab
  KComboBox * mViewHeaderPolicyCombo;
public:
  /**
   * Sets the option set to be edited.
   * Saves and forgets any previously option set that was being edited.
   * The set parameter may be 0: in this case the editor is simply disabled.
   */
  void editTheme( Theme *set );

  Theme * editedTheme() const
    { return mCurrentTheme; };

  void commit();
signals:
  void themeNameChanged();

private:
  void fillViewHeaderPolicyCombo();

protected slots:
  void slotNameEditTextEdited( const QString &newName );
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_SKINEDITOR_H__
