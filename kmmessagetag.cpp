
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

#include <config-kmail.h>

#include "kmkernel.h"

#include <QStringList>
#include <QColor>
#include <QFont>
#include <QString>
#include <QRegExp>

#include <kapplication.h>
#include <klocale.h>
#include <krandom.h>

#include <Nepomuk/Tag>
#include <nepomuk/resourcemanager.h>

//----------------------------KMMessageTagDescription------------------------------------
KMMessageTagDescription::KMMessageTagDescription(
                            const Nepomuk::Tag &tag,
                            const int aPriority, const QColor &aTextColor, 
                            const QColor &aBackgroundColor, 
                            const QFont &aTextFont, const bool aInToolbar, 
                            const KShortcut &aShortcut ) : mTag( tag ) {
  if ( !tag.isValid() ) {
    mEmpty = true;
    return;
  }
  mPriority = aPriority;
  mTextColor = aTextColor;
  mBackgroundColor = aBackgroundColor;
  mTextFont = aTextFont;
  mInToolbar = aInToolbar;
  mShortcut = aShortcut;
  mEmpty = false;
}

KMMessageTagDescription::KMMessageTagDescription( const Nepomuk::Tag &tag, const KConfigGroup& aGroup )
            : mTag( tag ), mPriority( -1 ), mEmpty( false )
{
  readConfig(aGroup);
}

const QString KMMessageTagDescription::toolbarIconName() const
{
  if ( mTag.symbols().isEmpty() )
    return QLatin1String( "mail-tagged" );
  return mTag.symbols().first();
}

void KMMessageTagDescription::setName( const QString& aName )
{
  mTag.setLabel( aName );
}

void KMMessageTagDescription::setBackgroundColor( const QColor& aBackgroundColor )
{
  mBackgroundColor = aBackgroundColor; 
}
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
  mTag.setSymbols( QStringList( aIconName ) ); 
}
void KMMessageTagDescription::setShortcut( const KShortcut &aShortcut )
{
  mShortcut = aShortcut;
}

void KMMessageTagDescription::readConfig( const KConfigGroup& group ) 
{
  mEmpty = false;
  mPriority = group.readEntry( "priority", -1 );

  //Empty is fine, uses default color
  mTextColor = group.readEntry( "text-color", QColor()); 
  mBackgroundColor = group.readEntry( "background-color", QColor() );
  //Empty uses current standard font
  mTextFont = group.readEntry( "text-font", QFont() );

  mInToolbar = group.readEntry( "toolbar", false );

  QString shortcut( group.readEntry( "shortcut", QString() ) );
  if ( !shortcut.isEmpty() )
    mShortcut = KShortcut( shortcut );

  purify();
}

void KMMessageTagDescription::writeConfig( KConfigGroup &group ) const 
{
  group.writeEntry( "priority", mPriority );
  if ( mTextColor.isValid() ) 
    group.writeEntry( "text-color", mTextColor );
  if ( mBackgroundColor.isValid() )
    group.writeEntry( "background-color", mBackgroundColor );

  if ( mTextFont != QFont() ) 
    group.writeEntry( "text-font", mTextFont );
  //Try to use isCopyOf later for the standard font. An alternative
  //which doesn't work:
  //if (! mTextFont.isCopyOf(KGlobalSettings::generalFont()))
  if ( !mShortcut.isEmpty() )
    group.writeEntry( "shortcut", mShortcut.toString() );

  group.writeEntry( "toolbar", mInToolbar );
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
  Nepomuk::ResourceManager::instance()->init(); 
}

KMMessageTagMgr::~KMMessageTagMgr() 
{
  writeConfig( false );
  clear();
  delete mTagDict;
  delete mTagList;
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
    Nepomuk::Tag tag;
    tag.setLabel( TagNames[i].name );
    KMMessageTagDescription* tmp_ptr = new KMMessageTagDescription( tag, i, TagNames[i].color );
    addTag( tmp_ptr, false );
  }
}

void KMMessageTagMgr::addTag( KMMessageTagDescription *aDesc, bool emitChange )
{
  if (! aDesc)
    return;
  if ( aDesc->priority() < 0 )
    aDesc->setPriority( mTagList->size() );
  mTagDict->insert( aDesc->tag().resourceUri().toString(), aDesc );
  QList<KMMessageTagDescription *>::iterator it = mTagList->begin(); 
  while ( it != mTagList->end() && (*it)->priority() < aDesc->priority() )
    it++;
  mTagList->insert( it, aDesc );

  if ( emitChange )
    emit msgTagListChanged();

  mDirty = true;
}

void KMMessageTagMgr::readConfig() 
{
  KSharedConfig::Ptr config = KMKernel::config();
  QString grpName;

  clear();

  KConfigGroup group( config, "General" );

  QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
  const int numTags = tags.size();

  if ( numTags <= 0 ) {
    //Key doesn't exist, which probably means first run with the tag feature
    //Create some tags, put them into dictionary, set dirty
    createInitialTags(); 
  } else {
    foreach( const Nepomuk::Tag &nepomukTag, tags ) {
      grpName = QString::fromLatin1( "MessageTag #%1" ).arg( nepomukTag.resourceUri().toString() );
      if ( !config->hasGroup( grpName ) ) {
        //Something wrong, set dirty so the tag set is correctly written back
        mDirty = true;
      }
      KConfigGroup group( config, grpName );
      KMMessageTagDescription *tag = new KMMessageTagDescription( nepomukTag, group );
      kDebug() << "loading tag" << nepomukTag.resourceUri() << tag->isEmpty();
      if ( tag->isEmpty() ) {
        delete tag;
      } else {
        addTag( tag, false );
      }
    }
  }
}

void KMMessageTagMgr::writeConfig( bool withSync ) 
{
  KSharedConfig::Ptr config = KMKernel::config();

  //first, delete all groups:
  QStringList tagGroups 
      = config->groupList().filter( QRegExp( "MessageTag #.+" ) );
  foreach ( const QString& group, tagGroups )
    config->deleteGroup( group );

  // Now, write out the new stuff:
  foreach ( KMMessageTagDescription *description, *mTagList ) {
    if ( ! description->isEmpty() ) {
      const QString grpName = QString::fromLatin1( "MessageTag #%1" ).arg( description->tag().resourceUri().toString() );
      KConfigGroup group( config, grpName );
      description->writeConfig( group );
    }
  }

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
