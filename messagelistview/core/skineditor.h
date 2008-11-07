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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_SKINEDITOR_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_SKINEDITOR_H__

#include "messagelistview/core/optionseteditor.h"
#include "messagelistview/core/skindelegate.h"
#include "messagelistview/core/skin.h"

#include <QTreeWidget>
#include <QLabel>
#include <QRect>

#include <KDialog>

class QComboBox;
class QPaintDevice;
class QCheckBox;

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

class SkinColumnPropertiesDialog : public KDialog
{
  Q_OBJECT
public:
  SkinColumnPropertiesDialog( QWidget * parent, Skin::Column * column, const QString &title );

protected:
  Skin::Column * mColumn;
  QLineEdit * mNameEdit;
  QCheckBox * mVisibleByDefaultCheck;
  QCheckBox * mIsSenderOrReceiverCheck;
  QComboBox * mMessageSortingCombo;

protected slots:
  void slotOkButtonClicked();
};

class SkinPreviewDelegate : public SkinDelegate
{
  Q_OBJECT
public:
  SkinPreviewDelegate( QAbstractItemView * parent, QPaintDevice * paintDevice );
  ~SkinPreviewDelegate();

private:
  GroupHeaderItem * mSampleGroupHeaderItem;
  MessageItem * mSampleMessageItem;
  ModelInvariantRowMapper * mRowMapper; // needed for the MessageItem above to be valid
public:
  virtual Item * itemFromIndex( const QModelIndex &index ) const;
};

class SkinPreviewWidget : public QTreeWidget
{
  Q_OBJECT
public:
  SkinPreviewWidget( QWidget * parent );
  ~SkinPreviewWidget();

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
  SkinPreviewDelegate * mDelegate;
  QTreeWidgetItem * mGroupHeaderSampleItem;
  QRect mSkinSelectedContentItemRect;
  Skin::ContentItem * mSelectedSkinContentItem;
  Skin::Column * mSelectedSkinColumn;
  QPoint mMouseDownPoint;
  Skin * mSkin;
  RowInsertPosition mRowInsertPosition;
  ItemInsertPosition mItemInsertPosition;
  QPoint mDropIndicatorPoint1;
  QPoint mDropIndicatorPoint2;
  bool mFirstShow;
public:
  QSize sizeHint() const;
  void setSkin( Skin * skin );

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
  bool computeContentItemInsertPosition( const QPoint &pos, Skin::ContentItem::Type type );

  void applySkinColumnWidths();

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

class SkinContentItemSourceLabel : public QLabel
{
  Q_OBJECT
public:
  SkinContentItemSourceLabel( QWidget * parent, Skin::ContentItem::Type type );
  ~SkinContentItemSourceLabel();
private:
  Skin::ContentItem::Type mType;
  QPoint mMousePressPoint;
public:
  Skin::ContentItem::Type type() const
    { return mType; };
  void startDrag();
protected:
  void mousePressEvent( QMouseEvent * e );
  void mouseMoveEvent( QMouseEvent * e );
};


class SkinEditor : public OptionSetEditor
{
  Q_OBJECT
public:
  SkinEditor( QWidget *parent );
  ~SkinEditor();

private:
  Skin * mCurrentSkin; // shallow, may be null!

  // Appearance tab
  SkinPreviewWidget * mPreviewWidget;

  // Advanced tab
  QComboBox * mViewHeaderPolicyCombo;
public:
  /**
   * Sets the option set to be edited.
   * Saves and forgets any previously option set that was being edited.
   * The set parameter may be 0: in this case the editor is simply disabled.
   */
  void editSkin( Skin *set );

  Skin * editedSkin() const
    { return mCurrentSkin; };

  void commit();
signals:
  void skinNameChanged();

private:
  void fillViewHeaderPolicyCombo();

protected slots:
  void slotNameEditTextEdited( const QString &newName );
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_SKINEDITOR_H__
