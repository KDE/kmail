
/*******************************************************************************
**
** Filename   : kmmessagetag.cpp
** Created on : 29 June 2007
** Copyright  : (c) 2007 Ismail Onur Filiz
** Email      : onurf@su.sabanciuniv.edu
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/

#include "kmmessagetag.h"
#include "kmkernel.h"

#include <QStringList>
#include <QColor>
#include <QFont>
#include <QString>
#include <QRegExp>

#include <kapplication.h>
#include <klocale.h>
#include <krandom.h>

//----------------------------KMMessageTagDescription------------------------------------
KMMessageTagDescription::KMMessageTagDescription( const QString &aLabel, 
                             const QString &aName, 
                            const int aPriority, const QColor &aTextColor, 
                            const QColor &aBackgroundColor, 
                            const QFont &aTextFont, const bool aInToolbar, 
                            const QString &aIconName, 
                            const KShortcut &aShortcut ) {
  if ( aLabel.isEmpty() || aName.isEmpty() ) {
    mEmpty = true;
    return;
  }
  mLabel = aLabel;
  mName = aName;
  mPriority = aPriority;
  mTextColor = aTextColor;
  mBackgroundColor = aBackgroundColor;
  mTextFont = aTextFont;
  mInToolbar = aInToolbar;
  mIconName = aIconName;
  mShortcut = aShortcut;
  mEmpty = false;
}

KMMessageTagDescription::KMMessageTagDescription( const KConfigGroup& aGroup ) 
            : mPriority( -1 ), mEmpty( false ) 
{
  readConfig(aGroup);
}

void KMMessageTagDescription::setLabel( const QString& aLabel ) { mLabel = aLabel; }
void KMMessageTagDescription::setName( const QString& aName ) { mName = aName; }
/*void KMMessageTagDescription::setBackgroundColor( const QColor& aBackgroundColor )
{
  mBackgroundColor = aBackgroundColor; 
}*/
void KMMessageTagDescription::setTextColor( const QColor& aTextColor ) 
{
  mTextColor = aTextColor; 
}
void KMMessageTagDescription::setTextFont( const QFont& aTextFont ) 
{
  mTextFont = aTextFont; 
}
void KMMessageTagDescription::setIconName( const QString& aIconName ) 
{
  mIconName = aIconName; 
}
void KMMessageTagDescription::setShortcut( const KShortcut &aShortcut )
{
  mShortcut = aShortcut;
}

void KMMessageTagDescription::readConfig( const KConfigGroup& group ) 
{
  mEmpty = true;

  mLabel = group.readEntry( "Label", QString() );
  if ( mLabel.isEmpty() )
    return;
  mName = group.readEntry( "Name", QString() );
  if ( mName.isEmpty() )
    return;

  mEmpty = false;

  //Empty is fine, uses default color
  mTextColor = group.readEntry( "text-color", QColor()); 
  mBackgroundColor = group.readEntry( "background-color", QColor() );
  //Empty uses current standard font
  mTextFont = group.readEntry( "text-font", QFont() );

  mInToolbar = group.readEntry( "toolbar", false );
  mIconName = group.readEntry( "icon", "rss_tag" );

  QString shortcut( group.readEntry( "shortcut", QString() ) );
  if ( !shortcut.isEmpty() )
    mShortcut = KShortcut( shortcut );

  purify();
}

void KMMessageTagDescription::writeConfig( KConfigGroup &group ) const 
{
  group.writeEntry( "Label", mLabel );
  group.writeEntry( "Name", mName );
  if ( mTextColor.isValid() ) 
    group.writeEntry( "text-color", mTextColor );

  if ( mTextFont != QFont() ) 
    group.writeEntry( "text-font", mTextFont );
  //Try to use isCopyOf later for the standard font. An alternative
  //which doesn't work:
  //if (! mTextFont.isCopyOf(KGlobalSettings::generalFont()))
  if ( !mShortcut.isEmpty() )
    group.writeEntry( "shortcut", mShortcut.toString() );

  group.writeEntry( "toolbar", mInToolbar );
  group.writeEntry( "icon", mIconName );
}

void KMMessageTagDescription::purify() 
{ 
  //fill as necessary
}
//---------------------------EO KMMessageTagDescription-----------------------------------

//-----------------------------KMMessageTagMgr-------------------------------------

KMMessageTagMgr::KMMessageTagMgr() : mDirty( 0 ) 
{
  mTagDict = new QHash<QString,KMMessageTagDescription *>();
  mTagList = new QList<KMMessageTagDescription *>();
}

KMMessageTagMgr::~KMMessageTagMgr() 
{
  writeConfig( false );
  clear();
  delete mTagDict;
}

const KMMessageTagDescription *KMMessageTagMgr::find( const QString &aLabel ) const 
{
  QHash< QString, KMMessageTagDescription* >::iterator it = mTagDict->find( aLabel );
  return it != mTagDict->end() ? it.value() : 0;

}

void KMMessageTagMgr::clear() 
{
  //Technically we are safe with true always, since there
  //is supposed to be only one Manager hence dictionary
  qDeleteAll(*mTagDict);
  mTagDict->clear();
  //TODO: Should I delete the items as well?
  mTagList->clear();
  mDirty = true;
}

void KMMessageTagMgr::fillTagsFromList( const QList<KMMessageTagDescription*> *aTagList ) 
{
  clear();
  //Assumes the list is ordered
  if ( aTagList ) {
    int i = 0;
    QListIterator<KMMessageTagDescription*> it ( *aTagList );
    while ( it.hasNext() ) {
      KMMessageTagDescription *tmp_ptr = new KMMessageTagDescription( *it.next() );
      tmp_ptr->setPriority( i );
      addTag( tmp_ptr, false );
      ++i;
    }
  }
  emit msgTagListChanged();
}

void KMMessageTagMgr::createInitialTags() 
{
  //Some predefined structures for the very first initialization of tags
  static const struct { 
    QString name;
    QString color;
  } TagNames[] = { 
    { i18n("Friend"), "blue" },
    { i18n("Business"), "green" },
    { i18n("Later"), "darkCyan"}
  };
  const int n = sizeof TagNames / sizeof *TagNames;
  for ( int i = 0; i < n; ++i ) {
    KMMessageTagDescription* tmp_ptr = new KMMessageTagDescription( KRandom::randomString(10),
                                       TagNames[i].name, i, TagNames[i].color );
    addTag( tmp_ptr, false );
  }
}

void KMMessageTagMgr::addTag( KMMessageTagDescription *aDesc, bool emitChange )
{
  if (! aDesc)
    return;
  KMMessageTagDescription *tmp_ptr = new KMMessageTagDescription( *aDesc );
  mTagDict->insert( tmp_ptr->label(), tmp_ptr );
  QList<KMMessageTagDescription *>::iterator it = mTagList->begin(); 
  while ( it != mTagList->end() && (*it)->priority() < tmp_ptr->priority() )
    it++;
  mTagList->insert( it, tmp_ptr );

  if ( emitChange )
    emit msgTagListChanged();

  mDirty = true;
}

void KMMessageTagMgr::readConfig() 
{
  KConfig *config = KMKernel::config();
  int numTags = 0;
  QString grpName;

  clear();

  KConfigGroup group( config, "General" );
  numTags = group.readEntry( "messagetags", -1 );

  if ( numTags < 0 ) {
    //Key doesn't exist, which probably means first run with the tag feature
    //Create some tags, put them into dictionary, set dirty
    createInitialTags(); 
  } else {
    //j used to handle priorities properly, in case a tag definition is missing
    int j = 0;
    for ( int i=0; i < numTags; ++i ) {
      grpName.sprintf( "MessageTag #%d", i );
      if (! config->hasGroup( grpName ) ) {
        //Something wrong, set dirty so the tag set is correctly written back
        mDirty = true;
        continue;
      }
      KConfigGroup group( config, grpName );
      KMMessageTagDescription *tag = new KMMessageTagDescription( group );
      if ( tag->isEmpty() ) {
        delete tag;
      } else {
        tag->setPriority(j);
        addTag( tag, false );
        ++j;
      }
    }
  }
}

void KMMessageTagMgr::writeConfig( bool withSync ) 
{
  KConfig *config = KMKernel::config();

  //first, delete all groups:
  QStringList tagGroups 
      = config->groupList().filter( QRegExp( "MessageTag #\\d+" ) );
  for ( QStringList::Iterator it = tagGroups.begin(); 
        it != tagGroups.end(); ++it )
    config->deleteGroup( *it );

  // Now, write out the new stuff:
  int i = 0;
  QString grpName;

  QListIterator<KMMessageTagDescription *> it ( *mTagList );
  while (it.hasNext()) {
    if ( ! it.peekNext()->isEmpty() ) {
      grpName.sprintf( "MessageTag #%d", i );
      KConfigGroup group( config, grpName );
      it.next()->writeConfig( group );
      ++i;
    }
  }

  KConfigGroup group( config, "General" );
  group.writeEntry( "messagetags", i );
  if (withSync) 
    config->sync();
  //mDirty = false;
}
//---------------------------EO KMMessageTagMgr------------------------------------

//-----------------------------KMMessageTagList------------------------------------

KMMessageTagList::KMMessageTagList() : QStringList() { }
KMMessageTagList::KMMessageTagList( const QStringList &aList ): QStringList( aList ) { }

bool KMMessageTagList::compareTags( const QString &lhs, const QString &rhs)
{
  //Implement less than or equal to
  const KMMessageTagDescription *lhsptr = kmkernel->msgTagMgr()->find( lhs );
  if ( !lhsptr )
    return false;
  const KMMessageTagDescription *rhsptr = kmkernel->msgTagMgr()->find( rhs );
  if ( !rhsptr )
    return true;
  return lhsptr->priority() < rhsptr->priority();
}

void KMMessageTagList::prioritySort() 
{
  //BLA
  //Use bubble sort for now
  int n = size();
  if ( !n )
    return;
  QString tmp, lhs, rhs;
  for ( int i=0; i< n-1; i++ ) {
    for ( int j=0; j< n-1-i; j++ ) {
      lhs = operator[]( j+1 );
      rhs = operator[]( j );
      if ( compareTags(lhs, rhs) ) {  /* compare the two neighbors */
        tmp = rhs;         /* swap a[j] and a[j+1]      */
        operator[]( j ) = lhs;
        operator[]( j+1 ) = tmp;
      }
    }
  }
}

const KMMessageTagList KMMessageTagList::split( const QString &aSep, 
                                        const QString &aStr )
{
  return KMMessageTagList( aStr.split( aSep, QString::SkipEmptyParts ) );
}
//----------------------------EO KMMessageTagList----------------------------------

#include "kmmessagetag.moc" 
