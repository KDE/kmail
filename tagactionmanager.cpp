/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "tagactionmanager.h"

#include "messageactions.h"
#include "tag.h"

#include "messagecore/taglistmonitor.h"

#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KMenu>
#include <KToggleAction>
#include <KXMLGUIClient>
#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Tag>
#include <Soprano/Util/SignalCacheModel>

#include <QSignalMapper>

using namespace KMail;

TagActionManager::TagActionManager( QObject *parent, KActionCollection *actionCollection,
                                    MessageActions *messageActions, KXMLGUIClient *guiClient )
  : QObject( parent ),
    mActionCollection( actionCollection ),
    mMessageActions( messageActions ),
    mMessageTagToggleMapper( 0 ),
    mGUIClient( guiClient ),
    mTagListMonitor( new MessageCore::TagListMonitor( this ) ),
    mSopranoModel( new Soprano::Util::SignalCacheModel( Nepomuk::ResourceManager::instance()->mainModel() ) )
{
  connect( mTagListMonitor, SIGNAL( tagsChanged() ), this, SLOT( tagsChanged() ) ); 
  // Listen to Nepomuk tag updates
  // ### This is way too slow for now, we use triggerUpdate() instead
  /*connect( mSopranoModel.data(), SIGNAL(statementAdded(Soprano::Statement)),
           SLOT(statementChanged(Soprano::Statement)) );
  connect( mSopranoModel.data(), SIGNAL(statementRemoved(Soprano::Statement)),
           SLOT(statementChanged(Soprano::Statement)) );*/
}

TagActionManager::~TagActionManager()
{
}

void TagActionManager::statementChanged( Soprano::Statement statement )
{
  // When a tag changes, immediatley update the actions to reflect that
  if ( statement.subject().type() == Soprano::Node::ResourceNode ) {
    Nepomuk::Resource res( statement.subject().uri() );
    if ( res.resourceType() == Nepomuk::Tag::resourceTypeUri() ) {
      createActions();
    }
  }
}

void TagActionManager::clearActions()
{
  //Remove the tag actions from the toolbar
  if ( !mToolbarActions.isEmpty() ) {
    if ( mGUIClient->factory() )
      mGUIClient->unplugActionList( "toolbar_messagetag_actions" );
    mToolbarActions.clear();
  }

  //Remove the tag actions from the status menu and the action collection,
  //then delete them.
  foreach( KAction *action, mTagActions ) {
    mMessageActions->messageStatusMenu()->removeAction( action );

    // This removes and deletes the action at the same time
    mActionCollection->removeAction( action );
  }

  mTagActions.clear();
  delete mMessageTagToggleMapper;
  mMessageTagToggleMapper = 0;
}

void TagActionManager::createActions()
{
  clearActions();

  //Use a mapper to understand which tag button is triggered
  mMessageTagToggleMapper = new QSignalMapper( this );
  connect( mMessageTagToggleMapper, SIGNAL( mapped( const QString& ) ),
           this, SIGNAL( tagActionTriggered( const QString& ) ) );

  // Build a sorted list of tags
  QList<Tag::Ptr> tagList;
  foreach( const Nepomuk::Tag &nepomukTag, Nepomuk::Tag::allTags() ) {
    tagList.append( Tag::fromNepomuk( nepomukTag ) );
  }
  qSort( tagList.begin(), tagList.end(), KMail::Tag::compare );

  // Create a action for each tag and plug it into various places
  foreach( const Tag::Ptr &tag, tagList ) {

    QString cleanName( i18n("Message Tag %1", tag->tagName ) );
    cleanName.replace('&',"&&");
    KToggleAction * const tagAction = new KToggleAction( KIcon( tag->iconName ),
                                                         cleanName, this );
    tagAction->setShortcut( tag->shortcut );
    tagAction->setIconText( tag->tagName );
    mActionCollection->addAction( tag->nepomukResourceUri.toString(), tagAction );
    connect( tagAction, SIGNAL( triggered( bool ) ),
             mMessageTagToggleMapper, SLOT( map() ) );

    // The shortcut configuration is done in the config dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the tag descriptions correctly.
    tagAction->setShortcutConfigurable( false );
    mMessageTagToggleMapper->setMapping( tagAction, tag->nepomukResourceUri.toString() );

    mTagActions.insert( tag->nepomukResourceUri.toString(), tagAction );
    mMessageActions->messageStatusMenu()->menu()->addAction( tagAction );
    if ( tag->inToolbar )
      mToolbarActions.append( tagAction );
  }

  if ( !mToolbarActions.isEmpty() && mGUIClient->factory() ) {
    mGUIClient->plugActionList( "toolbar_messagetag_actions", mToolbarActions );
  }
}

void TagActionManager::updateActionStates( int numberOfSelectedMessages,
                                           const Akonadi::Item &selectedItem )
{
  QMap<QString,KToggleAction*>::const_iterator it = mTagActions.constBegin();
  if ( 1 == numberOfSelectedMessages )
  {
    Q_ASSERT( selectedItem.isValid() );
    Nepomuk::Resource itemResource( selectedItem.url() );
    for ( ; it != mTagActions.constEnd(); ++it ) {
      const bool hasTag = itemResource.tags().contains( Nepomuk::Tag( it.key() ) );
      it.value()->setChecked( hasTag );
      it.value()->setEnabled( true );
    }
  }
  else if ( numberOfSelectedMessages > 1 ) {
    for ( ; it != mTagActions.constEnd(); ++it ) {
      Nepomuk::Tag tag( it.key() );
      it.value()->setChecked( false );
      it.value()->setEnabled( true );
      it.value()->setText( i18n("Toggle Message Tag %1", tag.label() ) );
    }
  }
  else {
    for ( ; it != mTagActions.constEnd(); ++it ) {
      it.value()->setEnabled( false );
    }
  }
}

void TagActionManager::tagsChanged()
{
  createActions();
}

