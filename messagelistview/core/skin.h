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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_SKIN_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_SKIN_H__

#include <QList>
#include <QPair>
#include <QString>
#include <QFont>
#include <QString>
#include <QFontMetrics>
#include <QSize>
#include <QColor>

#include "messagelistview/core/optionset.h"
#include "messagelistview/core/aggregation.h" // for Aggregation::MessageSorting

class QPaintDevice;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

/**
 * The Skin class defines the visual appearance of the MessageListView.
 *
 * The core structure of the skin is made up of Column objects which
 * are mapped to View (QTreeView) columns. Each skin must provide at least one column.
 *
 * Each column contains a set of Row objects dedicated to message items
 * and a set of Row objects dedicated to group header items. There must be at least
 * one message row and one group header row in each column. Rows are visually
 * ordered from top to bottom.
 * 
 * Each Row contains a set of left aligned and a set of right aligned ContentItem objects.
 * The right aligned items are painted from right to left while the left aligned
 * ones are painted from left to right. In Right-To-Left mode the SkinDelegate
 * follows the exact opposite convention.
 *
 * Each ContentItem object specifies a visual element to be displayed in a View
 * row. The visual elements may be pieces of text (Subject, Date) or icons.
 *
 * The Skin is designed to strictly interoperate with the SkinDelegate class
 * which takes care of rendering the contents when attached to an QAbstractItemView.
 */
class Skin : public OptionSet
{
public:
  /**
   * The ContentItem class defines a content item inside a Row.
   * Content items are data items extracted from a message or a group header:
   * they can be text, spacers, separators or icons.
   */
  class ContentItem
  {
  private:
    /**
     * Bits for composing the Type enumeration members.
     * We'll be able to test these bits to quickly figure out item properties.
     */
    enum TypePropertyBits
    {
      /**
       * Item can use the custom color property
       */
      CanUseCustomColor            = (1 << 16),
      /**
       * Item can be in a disabled state (for example the attachment icon when there is no attachment)
       */
      CanBeDisabled                = (1 << 17),
      /**
       * Item displays some sort of text
       */
      DisplaysText                 = (1 << 18),
      /**
       * Item makes sense (and can be applied) for messages
       */
      ApplicableToMessageItems     = (1 << 19),
      /**
       * Item makes sense (and can be applied) for group headers
       */
      ApplicableToGroupHeaderItems = (1 << 20),
      /**
       * The item takes more horizontal space than the other text items (at the time of writing it's only the subject)
       */
      LongText                     = (1 << 21),
      /**
       * The item displays an icon
       */
      IsIcon                       = (1 << 22),
      /**
       * The item is a small spacer
       */
      IsSpacer                     = (1 << 23),
      /**
       * The item is clickable
       */
      IsClickable                  = (1 << 24)
    };

  public:
    /**
     * The available ContentItem types.
     * Note that the values in this enum are unique values or'ed with the TypePropertyBits above.
     */
    enum Type
    {
      /**
       * Display the subject of the message item. This is a long text.
       */
      Subject                      = 1 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems | LongText,
      /**
       * Formatted date time of the message/group
       */
      Date                         = 2 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems | ApplicableToGroupHeaderItems,
      /**
       * From: or To: strip, depending on the folder settings
       */
      SenderOrReceiver             = 3 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems,
      /**
       * From: strip, always
       */
      Sender                       = 4 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems,
      /**
       * To: strip, always
       */
      Receiver                     = 5 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems,
      /**
       * Formatted size of the message
       */
      Size                         = 6 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems,
      /**
       * The icon that displays the new/unread/read state (never disabled)
       */
      ReadStateIcon                = 7 | ApplicableToMessageItems | IsIcon,
      /**
       * The icon that displays the atachment state (may be disabled)
       */
      AttachmentStateIcon          = 8 | CanBeDisabled | ApplicableToMessageItems | IsIcon,
      /**
       * The icon that displays the replied/forwarded state (may be disabled)
       */
      RepliedStateIcon             = 9 | CanBeDisabled | ApplicableToMessageItems | IsIcon,
      /**
       * The group header label
       */
      GroupHeaderLabel             = 10 | DisplaysText | CanUseCustomColor | ApplicableToGroupHeaderItems,
      /**
       * The ToDo state icon. May be disabled. Clickable (cycles todo->nothing)
       */
      ToDoStateIcon                = 11 | CanBeDisabled | ApplicableToMessageItems | IsIcon | IsClickable,
      /**
       * The Important tag icon. May be disabled. Clickable (cycles important->nothing)
       */
      ImportantStateIcon           = 12 | CanBeDisabled | ApplicableToMessageItems | IsIcon | IsClickable,
      /**
       * The Spam/Ham state icon. May be disabled. Clickable (cycles spam->ham->nothing)
       */
      SpamHamStateIcon             = 13 | CanBeDisabled | ApplicableToMessageItems | IsIcon | IsClickable,
      /**
       * The Watched/Ignored state icon. May be disabled. Clickable (cycles watched->ignored->nothing)
       */
      WatchedIgnoredStateIcon      = 14 | CanBeDisabled | ApplicableToMessageItems | IsIcon | IsClickable,
      /**
       * The Expanded state icon for group headers. May be disabled. Clickable (expands/collapses the group)
       */
      ExpandedStateIcon            = 15 | CanBeDisabled | ApplicableToGroupHeaderItems | IsIcon | IsClickable,
      /**
       * The Encryption state icon for messages. May be disabled (no encryption).
       */
      EncryptionStateIcon          = 16 | CanBeDisabled | ApplicableToMessageItems | IsIcon,
      /**
       * The Signature state icon for messages. May be disabled (no signature)
       */
      SignatureStateIcon           = 17 | CanBeDisabled | ApplicableToMessageItems | IsIcon,
      /**
       * A vertical separation line.
       */
      VerticalLine                 = 18 | CanUseCustomColor | ApplicableToMessageItems | ApplicableToGroupHeaderItems | IsSpacer,
      /**
       * A small empty spacer usable as separator.
       */
      HorizontalSpacer             = 19 | ApplicableToMessageItems | ApplicableToGroupHeaderItems | IsSpacer,
      /**
       * The date of the most recent message in subtree
       */
      MostRecentDate               = 20 | DisplaysText | CanUseCustomColor | ApplicableToMessageItems | ApplicableToGroupHeaderItems,
      /**
       * The combined icon that displays the new/unread/read/replied/forwarded state (never disabled)
       */
      CombinedReadRepliedStateIcon = 21 | ApplicableToMessageItems | IsIcon,
      /**
       * The list of MessageItem::Tag entries
       */
      TagList                      = 22 | ApplicableToMessageItems | IsIcon
#if 0
      TotalMessageCount
      UnreadMessageCount
      NewMessageCount
#endif
    };

    enum Flags
    {
      HideWhenDisabled = 1,                    ///< In disabled state the icon should take no space (overrides SoftenByBlendingWhenDisabled)
      SoftenByBlendingWhenDisabled = (1 << 1), ///< In disabled state the icon should be still shown, but made very soft by alpha blending
      UseCustomColor = (1 << 2),               ///< For text and vertical line. If set then always use a custom color, otherwise use default text color
      UseCustomFont = (1 << 3),                ///< For text items. If set then always use a custom font, otherwise default to the global font.
      SoftenByBlending = (1 << 4)              ///< For text items: use 60% opacity.
    };

  private:
     Type mType;                      ///< The type of item
     unsigned int mFlags;             ///< The flags of the item

     QFont mFont;                     ///< The font to use with this content item, meaningful only if displaysText() returns true.
     QColor mCustomColor;             ///< The color to use with this content item, meaningful only if canUseCustomColor() return true.
     // Cache stuff
     QPaintDevice * mLastPaintDevice; ///< The last paint device that used this content item (this is usually set only once)
     QFontMetrics mFontMetrics;       ///< Our font metrics cache (kept updated to the last QPaintDevice that uses this ContentItem)
     int mLineSpacing;                ///< The line spacing in mFontMetrics, this is a value we use a lot (so by caching it we can avoid a function call)

  public:
    /**
     * Creates a ContentItem with the specified type.
     * A content item must be added to a skin Row.
     */
    ContentItem( Type type );
    /**
     * Creates a ContentItem that is a copy of the content item src.
     * A content item must be added to a skin Row.
     */
    ContentItem( const ContentItem &src );

  public:
    /**
     * Returns the type of this content item
     */
    Type type() const
      { return mType; };

    /**
     * Returns true if this ContentItem can be in a "disabled" state.
     * The attachment state icon, for example, can be disabled when the related
     * message has no attachments. For such items the HideWhenDisabled
     * and SoftenByBlendingWhenDisabled flags are meaningful.
     */
    bool canBeDisabled() const
      { return ( static_cast< int >( mType ) & CanBeDisabled ) != 0; };

    /**
     * Returns true if this ContentItem can make use of a custom color.
     */
    bool canUseCustomColor() const
      { return ( static_cast< int >( mType ) & CanUseCustomColor ) != 0; };

    /**
     * Returns true if this item displays some kind of text.
     * Items that display text make use of the customFont() setting.
     */
    bool displaysText() const
      { return ( static_cast< int >( mType ) & DisplaysText ) != 0; };

    /**
     * Returns true if this item displays a long text.
     * The returned value makes sense only if displaysText() returned true.
     */
    bool displaysLongText() const
      { return ( static_cast< int >( mType ) & LongText ) != 0; };

    /**
     * Returns true if this item displays an icon.
     */
    bool isIcon() const
      { return ( static_cast< int >( mType ) & IsIcon ) != 0; };

    /**
     * Returns true if clicking on this kind of item can perform an action
     */
    bool isClickable() const
      { return ( static_cast< int >( mType ) & IsClickable ) != 0; };

    /**
     * Returns true if this item is a small spacer
     */
    bool isSpacer() const
      { return ( static_cast< int >( mType ) & IsSpacer ) != 0; };

    /**
     * Static test that returns true if an instance of ContentItem with the
     * specified type makes sense in a Row for message items.
     */
    static bool applicableToMessageItems( Type type );

    /**
     * Static test that returns true if an instance of ContentItem with the
     * specified type makes sense in a Row for group header items.
     */
    static bool applicableToGroupHeaderItems( Type type );

    /**
     * Returns a descriptive name for the specified content item type
     */
    static QString description( Type type );

    /**
     * Returns true if this item uses a custom color.
     * The return value of this function is valid only if canUseCustomColor() returns true.
     */
    bool useCustomColor() const
      { return mFlags & UseCustomColor; };

    /**
     * Makes this item use the custom color that can be set by setCustomColor().
     * The custom color is meaningful only if canUseCustomColor() returns true.
     */
    void setUseCustomColor( bool useCustomColor )
      { if ( useCustomColor )mFlags |= UseCustomColor; else mFlags &= ~UseCustomColor; };

    /**
     * Returns true if this item uses a custom font.
     * The return value of this function is valid only if displaysText() returns true.
     */
    bool useCustomFont() const
      { return mFlags & UseCustomFont; };

    /**
     * Makes this item use the custom font that can be set by setCustomFont().
     * The custom font is meaningful only if canUseCustomFont() returns true.
     */
    void setUseCustomFont( bool useCustomFont )
      { if ( useCustomFont )mFlags |= UseCustomFont; else mFlags &= ~UseCustomFont; };

    /**
     * Returns true if this item should be hidden when in disabled state.
     * Hidden content items simply aren't painted and take no space.
     * This flag has meaning only on items for that canBeDisabled() returns true.
     */
    bool hideWhenDisabled() const
      { return mFlags & HideWhenDisabled; };

    /**
     * Sets the flag that causes this item to be hidden when disabled.
     * Hidden content items simply aren't painted and take no space.
     * This flag overrides the setSoftenByBlendingWhenDisabled() setting.
     * This flag has meaning only on items for that canBeDisabled() returns true.
     */
    void setHideWhenDisabled( bool hideWhenDisabled )
      { if ( hideWhenDisabled )mFlags |= HideWhenDisabled; else mFlags &= ~HideWhenDisabled; };

    /**
     * Returns true if this item should be painted in a "soft" fashion when
     * in disabled state. Soft icons are painted with very low opacity.
     * This flag has meaning only on items for that canBeDisabled() returns true.
     */
    bool softenByBlendingWhenDisabled() const
      { return mFlags & SoftenByBlendingWhenDisabled; };

    /**
     * Sets the flag that causes this item to be painted "softly" when disabled.
     * Soft icons are painted with very low opacity.
     * This flag may be overridden by the setHideWhenDisabled() setting.
     * This flag has meaning only on items for that canBeDisabled() returns true.
     */
    void setSoftenByBlendingWhenDisabled( bool softenByBlendingWhenDisabled )
      { if ( softenByBlendingWhenDisabled )mFlags |= SoftenByBlendingWhenDisabled; else mFlags &= ~SoftenByBlendingWhenDisabled; };

    /**
     * Returns true if this item should be always painted in a "soft" fashion.
     * Meaningful only for text items.
     */
    bool softenByBlending() const
      { return mFlags & SoftenByBlending; };

    /**
     * Sets the flag that causes this item to be painted "softly".
     * Meaningful only for text items.
     */
    void setSoftenByBlending( bool softenByBlending )
      { if ( softenByBlending )mFlags |= SoftenByBlending; else mFlags &= ~SoftenByBlending; };

    /**
     * Sets the custom font to be used with this item.
     * The font is meaningful only for items for that displaysText() returns true.
     * You must also call setUseCustomFont() in order for this setting to be effective.
     */
    void setFont( const QFont &font );

    /**
     * Returns the font used by this item. It may be a custom font set by setFont()
     * or the default application font (returned by KGlobalSettings::generalFont()).
     * This setting is valid as long as you have called updateFontMetrics()
     * with the appropriate paint device.
     */
    const QFont & font() const
      { return mFont; };

    /**
     * Returns the custom color set for this item.
     * The return value is meaningful only if canUseCustomColor() returns true
     * returns true and setUseCustomColor( true ) has been called.
     */
    const QColor & customColor() const
      { return mCustomColor; };

    /**
     * Sets the custom color for this item. Meaningful only if canUseCustomColor()
     * returns true and you call setUseCustomColor( true )
     */
    void setCustomColor( const QColor &clr )
      { mCustomColor = clr; };

    // Stuff used by SkinDelegate. This section should be protected but some gcc
    // versions seem to get confused with nested class and friend declarations
    // so for portability we're using a public interface also here.

    /**
     * The last QPaintDevice that made use of this item.
     * This function is used by SkinDelegate for QFontMetrics caching purposes.
     */
    const QPaintDevice * lastPaintDevice() const
      { return mLastPaintDevice; };

    /**
     * Updates the font metrics cache for this item by recreating them
     * for the currently set font and the specified QPaintDevice.
     * This function will also reset the font to the KGlobalSettings::generalFont()
     * if you haven't called setUseCustomFont().
     * This function is used by SkinDelegate, you usually don't need to care.
     */
    void updateFontMetrics( QPaintDevice * device );

    /**
     * Returns the cached font metrics attacched to this content item.
     * The font metrics must be kept up-to-date by the means of the updateFontMetrics()
     * function whenever the QPaintDevice that this ContentItem is painted
     * on is different than lastPaintDevice(). This is done by SkinDelegate
     * and in fact you shouldn't care.
     */
    const QFontMetrics & fontMetrics() const
      { return mFontMetrics; };

    /**
     * Returns the cached font metrics line spacing for this content item.
     * The line spacing is used really often in SkinDelegate so this
     * inlineable getter will help the compiler in optimizing stuff.
     */
    int lineSpacing() const
      { return mLineSpacing; };

    /**
     * Resets the cache of this content item.
     * This is called by the Skin::Row resetCache() method.
     */
    void resetCache();

    /**
     * Handles content item saving (used by Skin::Row::save())
     */
    void save( QDataStream &stream ) const;

    /**
     * Handles content item loading (used by Skin::Row::load())
     */
    bool load( QDataStream &stream );

  };

  /**
   * The Row class defines a row of items inside a Column.
   * The Row has a list of left aligned and a list of right aligned ContentItems.
   */
  class Row
  {
  public:
    Row();
    Row( const Row &src );
    ~Row();

  private:
    QList< ContentItem * > mLeftItems;   ///< The list of left aligned items
    QList< ContentItem * > mRightItems;  ///< The list of right aligned items
    QSize mSizeHint;                     ///< The size hint for this row: the height is the sufficient minimum, the width is a guess, invalid size when not computed

  public:
    /**
     * Returns the list of left aligned items for this row
     */
    const QList< ContentItem * > & leftItems() const
      { return mLeftItems; };

    /**
     * Removes all the left items from this row: the items are deleted.
     */
    void removeAllLeftItems();

    /**
     * Adds a left aligned item to this row. The row takes the ownership
     * of the ContentItem pointer.
     */
    void addLeftItem( ContentItem * item )
      { mLeftItems.append( item ); };

    /**
     * Adds a left aligned item at the specified position in this row. The row takes the ownership
     * of the ContentItem pointer.
     */
    void insertLeftItem( int idx, ContentItem * item );

    /**
     * Removes the specified left aligned content item from this row.
     * The item is NOT deleted.
     */
    void removeLeftItem( ContentItem * item )
      { mLeftItems.removeAll( item ); };

    /**
     * Returns the list of right aligned items for this row
     */
    const QList< ContentItem * > & rightItems() const
      { return mRightItems; };

    /**
     * Removes all the right items from this row. The items are deleted.
     */
    void removeAllRightItems();

    /**
     * Adds a right aligned item to this row. The row takes the ownership
     * of the ContentItem pointer. Please note that the first right aligned item
     * will start at the right edge, the second right aligned item will come after it etc...
     */
    void addRightItem( ContentItem * item )
      { mRightItems.append( item ); };

    /**
     * Adds a right aligned item at the specified position in this row. The row takes the ownership
     * of the ContentItem pointer. Remember that right item positions go from right to left.
     */
    void insertRightItem( int idx, ContentItem * item );

    /**
     * Removes the specified right aligned content item from this row.
     * The item is NOT deleted.
     */
    void removeRightItem( ContentItem * item )
      { mRightItems.removeAll( item ); };

    /**
     * Returns the cached size hint for this row. The returned size is invalid
     * if no cached size hint has been set yet.
     */
    QSize sizeHint() const
      { return mSizeHint; };

    /**
     * Sets the cached size hint for this row.
     */
    void setSizeHint( const QSize &s )
      { mSizeHint = s; };

    /**
     * Returns true if this row contains text items.
     * This is useful if you want to know if the column should just get
     * its minimum allowable space or it should get more.
     */
    bool containsTextItems() const;

    /**
     * Called from the Column's resetCache() method. You shouldn't need to care.
     */
    void resetCache();

    /**
     * Handles row saving (used by Skin::Column::save())
     */
    void save( QDataStream &stream ) const;

    /**
     * Handles row loading (used by Skin::Column::load())
     */
    bool load( QDataStream &stream );

  };

  /**
   * The Column class defines a view column available inside this skin.
   * Each Column has a list of Row items that define the visible rows.
   */
  class Column
  {
  public:
    Column();
    Column( const Column &src );
    ~Column();

  private:
    QString mLabel;                                   ///< The label visible in the column header
    bool mVisibleByDefault;                           ///< Is this column visible by default ?
    bool mIsSenderOrReceiver;                         ///< If this column displays the sender/receiver field then we will update its label on the fly
    Aggregation::MessageSorting mMessageSorting;      ///< The message sort order we switch to when clicking on this column
    QList< Row * > mGroupHeaderRows;                  ///< The list of rows we display in this column for a GroupHeaderItem
    QList< Row * > mMessageRows;                      ///< The list of rows we display in this column for a MessageItem
    // cache
    QSize mGroupHeaderSizeHint;                       ///< The cached size hint for group header rows: invalid if not computed yet
    QSize mMessageSizeHint;                           ///< The cached size hint for message rows: invalid if not computed yet

  public:
    /**
     * Returns the label set for this column
     */
    const QString & label() const
      { return mLabel; };

    /**
     * Sets the label for this column
     */
    void setLabel( const QString &label )
      { mLabel = label; };

    /**
     * Returns true if this column is marked as "sender/receiver" and we should
     * update its label on-the-fly.
     */
    bool isSenderOrReceiver() const
      { return mIsSenderOrReceiver; };

    /**
     * Marks this column as containing the "sender/receiver" field.
     * Such columns will have the label automatically updated.
     */
    void setIsSenderOrReceiver( bool sor )
      { mIsSenderOrReceiver = sor; };

    /**
     * Returns true if this column has to be shown by default
     */
    bool visibleByDefault() const
      { return mVisibleByDefault; };

    /**
     * Sets the "visible by default" tag for this column.
     */
    void setVisibleByDefault( bool vbd )
      { mVisibleByDefault = vbd; };

    /**
     * Returns the sort order for messages that we should switch to
     * when clicking on this column's header (if visible at all).
     */
    Aggregation::MessageSorting messageSorting() const
      { return mMessageSorting; };

    /**
     * Sets the sort order for messages that we should switch to
     * when clicking on this column's header (if visible at all).
     */
    void setMessageSorting( Aggregation::MessageSorting ms )
      { mMessageSorting = ms; };

    /**
     * Returns the list of rows visible in this column for a MessageItem
     */
    const QList< Row * > & messageRows() const
      { return mMessageRows; };

    /**
     * Removes all the message rows from this column.
     */
    void removeAllMessageRows();

    /**
     * Appends a message row to this skin column. The Skin takes
     * the ownership of the Row pointer.
     */
    void addMessageRow( Row * row )
      { mMessageRows.append( row ); };

    /**
     * Inserts a message row to this skin column in the specified position. The Skin takes
     * the ownership of the Row pointer.
     */
    void insertMessageRow( int idx, Row * row );

    /**
     * Removes the specified message row. The row is NOT deleted.
     */
    void removeMessageRow( Row * row )
      { mMessageRows.removeAll( row ); };

    /**
     * Returns the list of rows visible in this column for a GroupHeaderItem
     */
    const QList< Row * > & groupHeaderRows() const
      { return mGroupHeaderRows; };

    /**
     * Removes all the group header rows from this column.
     */
    void removeAllGroupHeaderRows();

    /**
     * Appends a group header row to this skin. The Skin takes
     * the ownership of the Row pointer.
     */
    void addGroupHeaderRow( Row * row )
      { mGroupHeaderRows.append( row ); };

    /**
     * Inserts a group header row to this skin column in the specified position. The Skin takes
     * the ownership of the Row pointer.
     */
    void insertGroupHeaderRow( int idx, Row * row );

    /**
     * Removes the specified group header row. The row is NOT deleted.
     */
    void removeGroupHeaderRow( Row * row )
      { mGroupHeaderRows.removeAll( row ); };

    /**
     * Returns true if this column contains text items.
     * This is useful if you want to know if the column should just get
     * its minimum allowable space or it should get more.
     */
    bool containsTextItems() const;

    /**
     * This is called by the Skin when it's resetCache() method is called.
     * You shouldn't need to care about this.
     */
    void resetCache();

    /**
     * Returns the cached size hint for group header rows in this column
     * or an invalid QSize if the cached size hint hasn't been set yet.
     */
    QSize groupHeaderSizeHint() const
      { return mGroupHeaderSizeHint; };

    /**
     * Sets the cached size hint for group header rows in this column.
     */
    void setGroupHeaderSizeHint( const QSize &sh )
      { mGroupHeaderSizeHint = sh; };

    /**
     * Returns the cached size hint for message rows in this column
     * or an invalid QSize if the cached size hint hasn't been set yet.
     */
    QSize messageSizeHint() const
      { return mMessageSizeHint; };

    /**
     * Sets the cached size hint for message rows in this column.
     */
    void setMessageSizeHint( const QSize &sh )
      { mMessageSizeHint = sh; };

    /**
     * Handles column saving (used by Skin::save())
     */
    void save( QDataStream &stream ) const;

    /**
     * Handles column loading (used by Skin::load())
     */
    bool load( QDataStream &stream );

  };

public:
  Skin();
  Skin( const QString &name, const QString &description );
  Skin( const Skin &src );
  ~Skin();

public:
  /**
   * Which color do we use to paint group header background ?
   */
  enum GroupHeaderBackgroundMode
  {
    Transparent,                  ///< No background at all: use style default
    AutoColor,                    ///< Automatically determine the color (somewhere in the middle between background and text)
    CustomColor                   ///< Use a custom color
  };

  /**
   * How do we paint group header background ?
   */
  enum GroupHeaderBackgroundStyle
  {
    PlainRect,                    ///< One plain rect per column
    PlainJoinedRect,              ///< One big plain rect for all the columns
    RoundedRect,                  ///< One rounded rect per column
    RoundedJoinedRect,            ///< One big rounded rect for all the columns
    GradientRect,                 ///< One rounded gradient filled rect per column
    GradientJoinedRect,           ///< One big rounded gradient rect for all the columns
    StyledRect,                   ///< One styled rect per column
    StyledJoinedRect              ///< One big styled rect per column
  };

  /**
   * How do we manage the QHeaderView attacched to our View ?
   */
  enum ViewHeaderPolicy
  {
    ShowHeaderAlways,
    NeverShowHeader
    //ShowWhenMoreThanOneColumn,  ///< This doesn't work at the moment (since without header we don't have means for showing columns back)
  };

private:
  QList< Column * > mColumns;                             ///< The list of columns available in this skin

  GroupHeaderBackgroundMode mGroupHeaderBackgroundMode;   ///< How do we paint group header background ?
  QColor mGroupHeaderBackgroundColor;                     ///< The background color of the message group, used only if CustomColor
  GroupHeaderBackgroundStyle mGroupHeaderBackgroundStyle; ///< How do we paint group header background ?
  ViewHeaderPolicy mViewHeaderPolicy;
public:
  /**
   * Returns the list of columns available in this skin
   */
  const QList< Column * > & columns() const
    { return mColumns; };

  /**
   * Returns a pointer to the column at the specified index or 0 if there is no such column
   */
  Column * column( int idx ) const
    { return mColumns.count() > idx ? mColumns.at( idx ) : 0; };

  /**
   * Removes all columns from this skin
   */
  void removeAllColumns();

  /**
   * Appends a column to this skin
   */
  void addColumn( Column * column )
    { mColumns.append( column ); };

  /**
   * Inserts a column to this skin at the specified position.
   */
  void insertColumn( int idx, Column * column );

  /**
   * Removes the specified message row. The row is NOT deleted.
   */
  void removeColumn( Column * col )
    { mColumns.removeAll( col ); };

  /**
   * Returns the group header background mode for this skin.
   */
  GroupHeaderBackgroundMode groupHeaderBackgroundMode() const
    { return mGroupHeaderBackgroundMode; };

  /**
   * Sets the group header background mode for this skin.
   * If you set it to CustomColor then please also setGroupHeaderBackgroundColor()
   */
  void setGroupHeaderBackgroundMode( GroupHeaderBackgroundMode bm );

  /**
   * Returns the group header background color for this skin.
   * This color is used only if groupHeaderBackgroundMode() is set to CustomColor.
   */
  const QColor & groupHeaderBackgroundColor() const
    { return mGroupHeaderBackgroundColor; };

  /**
   * Sets the group header background color for this skin.
   * This color is used only if groupHeaderBackgroundMode() is set to CustomColor.
   */
  void setGroupHeaderBackgroundColor( const QColor &clr )
    { mGroupHeaderBackgroundColor = clr; };

  /**
   * Returns the group header background style for this skin.
   * The group header background style makes sense only if groupHeaderBackgroundMode() is
   * set to something different than Transparent.
   */
  GroupHeaderBackgroundStyle groupHeaderBackgroundStyle() const
    { return mGroupHeaderBackgroundStyle; };

  /**
   * Sets the group header background style for this skin.
   * The group header background style makes sense only if groupHeaderBackgroundMode() is
   * set to something different than Transparent.
   */
  void setGroupHeaderBackgroundStyle( GroupHeaderBackgroundStyle groupHeaderBackgroundStyle )
    { mGroupHeaderBackgroundStyle = groupHeaderBackgroundStyle; };

  /**
   * Enumerates the available group header background styles.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   */
  static QList< QPair< QString, int > > enumerateGroupHeaderBackgroundStyles();

  /**
   * Returns the currently set ViewHeaderPolicy
   */
  ViewHeaderPolicy viewHeaderPolicy() const
    { return mViewHeaderPolicy; };

  /**
   * Sets the ViewHeaderPolicy for this skin
   */
  void setViewHeaderPolicy( ViewHeaderPolicy vhp )
    { mViewHeaderPolicy = vhp; };

  /**
   * Enumerates the available view header policy options.
   * The returned descriptors are pairs in that the first item is the localized description
   * of the option value and the second item is the integer option value itself.
   */
  static QList< QPair< QString, int > > enumerateViewHeaderPolicyOptions();

  /**
   * Resets the cache for this skin. This is called by the SkinDelegate
   * when the skin is applied and must be called before any changes to this
   * skin are going to be painted (that is, apply a chunk of changes, call
   * resetCache() then repaint.
   */
  void resetCache();

protected:
  /**
   * Pure virtual reimplemented from OptionSet.
   */
  virtual void save( QDataStream &stream ) const;

  /**
   * Pure virtual reimplemented from OptionSet.
   */
  virtual bool load( QDataStream &stream );

};

} // namespace Core

} // namespace MessageListView

} // namespace KMail


#endif //!__KMAIL_MESSAGELISTVIEW_CORE_SKIN_H__
