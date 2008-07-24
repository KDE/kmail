/*
 * kmail: KDE mail client
 * Copyright (c) 2007 Ismail Onur Filiz <onurf@su.sabanciuniv.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
/** @file
    This file defines KMMessageTagDescription, KMMessageTagMgr and KMMessageTagList classes. They
    are used for adding custom tagging support to kmail.

    @author Ismail Onur Filiz
*/

#ifndef KMMESSAGETAG_H
#define KMMESSAGETAG_H

#include <QObject>
#include <QHash>
#include <QColor>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>

#include <KShortcut>
class KConfigGroup;

/** This is the class holding information about how the tag modifies appearance
of the header line.*/
class KMMessageTagDescription
{
  /*Label and priority are not that commonly used, might be able to remove later*/
  public:
    /** Constructor used for reading the tag description from a KConfigGroup
        @p aGroup . Calls readConfig( @p aGroup ) .It defaults to certain values
        for values that are not defined in the configuration.

      @param aGroup The object the description is to be read from
    */
    explicit KMMessageTagDescription( const KConfigGroup &aGroup );

    /** Constructor using a set of given parameters
      @param aLabel 10 letter random label that uniquely identifies the tag
      @param aName Visible name for the tag
      @param aPriority Priority of the tag. 0 is the highest. Used in defining
            which tag to modify the appearance if the message has multiple
            tags. Note that Important and Todo flags still dominate tags.
      @param aTextColor Text color
      @param aBackgroundColor Background color.
      @param aTextFont Font of the text
      @param aInToolbar Whether the toggle button appears in the toolbar
      @param aIconName The name for the corresponding icon that will appear in
              the menus and toolbar. The default value is an icon packaged with
              akregator.
      */
    KMMessageTagDescription( const QString &aLabel, const QString &aName,
                  const int aPriority = -1,
                  const QColor &aTextColor = QColor(),
                  const QColor &aBackgroundColor = QColor(),
                  const QFont &aTextFont = QFont(),
                  const bool aInToolbar = false,
                  const QString &aIconName = "feed-subscribe",
                  const KShortcut &aShortcut = KShortcut() );

    /** Accessor functions */
    const QString label() const { return mLabel; }
    const QString name() const { return mName; }
    int priority() const { return mPriority; }
    const QColor textColor() const { return mTextColor; }
    const QColor backgroundColor() const { return mBackgroundColor; }
    const QFont textFont() const { return mTextFont; }
    bool inToolbar() const { return mInToolbar; }
    const QString toolbarIconName() const { return mIconName; }
    bool isEmpty() const { return mEmpty; }
    const KShortcut shortcut() { return mShortcut; }

    void setLabel( const QString & );
    void setName( const QString & );
    void setPriority( unsigned int aPriority ) { mPriority = aPriority; }
    void setBackgroundColor( const QColor & );
    void setTextColor( const QColor & );
    void setTextFont( const QFont & );
    void setInToolbar( bool aInToolbar ) { mInToolbar = aInToolbar; }
    void setIconName( const QString & );
    void setShortcut( const KShortcut & );

    /** Sanitizes the properties. Currently empty*/
    void purify( void );
    void readConfig( const KConfigGroup & );
    void writeConfig( KConfigGroup & ) const;

  private:
    int mPriority;
    QString mName;
    QString mLabel;

    QColor mBackgroundColor;
    QColor mTextColor;

    QFont mTextFont;

    bool mInToolbar;
    QString mIconName;

    KShortcut mShortcut;

    bool mEmpty;
};

/** Manages the set of tag descriptions. The descriptions are stored in a
dictionary for fast access. Allows updating of the GUI after modifications to
the tags by emitting msgTagListChanged() . All changes to tags should be done
through this class*/
class KMMessageTagMgr : public QObject
{
  Q_OBJECT

  public:
    /** Creates the dictionary and sets up the necessary environment*/
    KMMessageTagMgr();
    ~KMMessageTagMgr();

    /** Creates some hard-coded tags in case it is the first run after
        introduction of custom tags.*/
    void createInitialTags( void );
    /** Managing function that creates individual tag descriptions and calls
        their readConfig functions */
    void readConfig( void );
    /** Function that calls each tag's writeConfig with the proper group*/
    void writeConfig( bool );
    /** Searches for an exact match to the given @p aLabel (not the visible
        name). Returns 0 if not found.
        @param aLabel 10 letter label to be searched for in the dictionary
    */
    const KMMessageTagDescription *find( const QString &aLabel ) const;
    /**Returns a pointer to the dictionary itself. Used to iterate over all
       tags. Shouldn't be used to modify tags.*/
    const QHash<QString, KMMessageTagDescription*> *msgTagDict(void) const { return mTagDict; }
    /**Returns a pointer to the priority ordered list*/
    const QList<KMMessageTagDescription *> *msgTagList(void) const { return mTagList; }
    /** @return @c true if the tags are modified after the initial config read*/
    bool isDirty( void ) const { return mDirty; }
    /**Fills the tag dictionary from the given pointer list @p aTagList . The
       dictionary is cleared first, so @p aTagList should be a complete set of
       tags ordered in decreasing priority. The tag descriptions that @p aTagList
       points to are copied, so it is safe to delete them afterwards.
       @param aTagList Pointer list that contains a full set of tags and is
                       priority ordered*/
    void fillTagsFromList( const QList<KMMessageTagDescription*> *aTagList );

  signals:
    /**Emitted when the tag set changes in any way*/
    void msgTagListChanged(void);

  private:
    //This function shouldn't be public since it doesn't emit msgTagListChanged
    void clear(void);

    //! Add the tag. This method takes ownership of @p aDesc.
    void addTag( KMMessageTagDescription *aDesc, bool emitChange = true );

    QHash<QString,KMMessageTagDescription*> *mTagDict;
    QList<KMMessageTagDescription *> *mTagList;
    bool mDirty;
};

/** A thin wrapper around QStringList for implementing ordering of tag
labels according to their priorities. The sorting algorithm is bubble sorting*/
class KMMessageTagList : public QStringList
{
  public:
    KMMessageTagList();
    /**Copying constructor that upgrades @p aList to KMMessageTagList
      @param aList QStringList to be upgraded
    */
    explicit KMMessageTagList( const QStringList &aList );
    /**Sorts the labels according to priorities. Uses bubble sort. Uses the tag
      manager in the process*/
    void prioritySort();
    /**Splits the given string into a KMMessageTagList
    @param aSep Separating string
    @param aStr String to be separated*/
    static const KMMessageTagList split( const QString &aSep, const QString &aStr );
  private:
    /*Internal function that implements less than by looking at priorities.
      Uses the tag manager in the process*/
    bool compareTags( const QString &lhs, const QString &rhs );
};

#endif //KMMESSAGETAG_H
