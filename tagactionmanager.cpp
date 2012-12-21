/* Copyright 2010 Thomas McGuire <mcguire@kde.org>
   Copyright 2011 Laurent Montel <montel@kde.org>

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

#include <Nepomuk2/Tag>
#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Query/QueryServiceClient>
#include <Nepomuk2/Query/Result>
#include <Nepomuk2/Query/ResourceTypeTerm>

#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KMenu>
#include <KToggleAction>
#include <KXMLGUIClient>

#include <QSignalMapper>
#include <soprano/nao.h>
#include <nepomuk2/resourcewatcher.h>

using namespace KMail;

static int s_numberMaxTag = 10;

TagActionManager::TagActionManager( QObject *parent, KActionCollection *actionCollection,
                                    MessageActions *messageActions, KXMLGUIClient *guiClient )
  : QObject( parent ),
    mActionCollection( actionCollection ),
    mMessageActions( messageActions ),
    mMessageTagToggleMapper( 0 ),
    mGUIClient( guiClient ),
    mSeparatorAction( 0 ),
    mMoreAction( 0 ),
    mTagQueryClient( 0 )
{
  mMessageActions->messageStatusMenu()->menu()->addSeparator();
  connect( Nepomuk2::ResourceManager::instance(), SIGNAL(nepomukSystemStarted()),
           SLOT(tagsChanged()) );
  connect( Nepomuk2::ResourceManager::instance(), SIGNAL(nepomukSystemStopped()),
           SLOT(tagsChanged()) );

  Nepomuk2::ResourceWatcher* watcher = new Nepomuk2::ResourceWatcher(this);
  watcher->addType(Soprano::Vocabulary::NAO::Tag());
  connect(watcher, SIGNAL(resourceCreated(Nepomuk2::Resource,QList<QUrl>)), this, SLOT(resourceCreated(Nepomuk2::Resource,QList<QUrl>)));
  connect(watcher, SIGNAL(resourceRemoved(QUrl,QList<QUrl>)),this, SLOT(resourceRemoved(QUrl,QList<QUrl>)));
  connect(watcher, SIGNAL(propertyChanged(Nepomuk2::Resource,Nepomuk2::Types::Property,QVariantList,QVariantList)),this,SLOT(propertyChanged(Nepomuk2::Resource)));
  watcher->start();
}

TagActionManager::~TagActionManager()
{
}

void TagActionManager::clearActions()
{
    //Remove the tag actions from the toolbar
  if ( !mToolbarActions.isEmpty() ) {
    if ( mGUIClient->factory() ) {
      mGUIClient->unplugActionList( "toolbar_messagetag_actions" );
    }
    mToolbarActions.clear();
  }

  //Remove the tag actions from the status menu and the action collection,
  //then delete them.
  foreach( KAction *action, mTagActions ) {
    mMessageActions->messageStatusMenu()->removeAction( action );

    // This removes and deletes the action at the same time
    mActionCollection->removeAction( action );
  }

  if ( mSeparatorAction ) {
    mMessageActions->messageStatusMenu()->removeAction( mSeparatorAction );
    mSeparatorAction = 0;
  }
  if ( mMoreAction ) {
    mMessageActions->messageStatusMenu()->removeAction( mMoreAction );
    mMoreAction = 0;
  }

  mTagActions.clear();
  delete mMessageTagToggleMapper;
  mMessageTagToggleMapper = 0;
}

void TagActionManager::createTagAction( const MailCommon::Tag::Ptr &tag, bool addToMenu )
{
  QString cleanName( i18n("Message Tag %1", tag->tagName ) );
  cleanName.replace('&',"&&");
  KToggleAction * const tagAction = new KToggleAction( KIcon( tag->iconName ),
                                                       cleanName, this );
  tagAction->setShortcut( tag->shortcut );
  tagAction->setIconText( tag->tagName );
  mActionCollection->addAction( tag->nepomukResourceUri.toString(), tagAction );
  connect( tagAction, SIGNAL(triggered(bool)),
           mMessageTagToggleMapper, SLOT(map()) );

  // The shortcut configuration is done in the config dialog.
  // The shortcut set in the shortcut dialog would not be saved back to
  // the tag descriptions correctly.
  tagAction->setShortcutConfigurable( false );
  mMessageTagToggleMapper->setMapping( tagAction, tag->nepomukResourceUri.toString() );

  mTagActions.insert( tag->nepomukResourceUri.toString(), tagAction );
  if ( addToMenu )
    mMessageActions->messageStatusMenu()->menu()->addAction( tagAction );

  if ( tag->inToolbar ) {
    mToolbarActions.append( tagAction );
  }
}

void TagActionManager::createActions()
{
  if( mTagQueryClient )
      return;
  clearActions();

  if ( mTags.isEmpty() ) {
      mTagQueryClient = new Nepomuk2::Query::QueryServiceClient(this);
      connect( mTagQueryClient, SIGNAL(newEntries(QList<Nepomuk2::Query::Result>)),
            this, SLOT(newTagEntries(QList<Nepomuk2::Query::Result>)) );
      connect( mTagQueryClient, SIGNAL(finishedListing()),
            this, SLOT(finishedTagListing()) );
      Nepomuk2::Query::ResourceTypeTerm term( Soprano::Vocabulary::NAO::Tag() );
      Nepomuk2::Query::Query query( term );
      mTagQueryClient->query(query);
  } else {
    createTagActions();
  }
}

void TagActionManager::createTagActions()
{
  //Use a mapper to understand which tag button is triggered
  mMessageTagToggleMapper = new QSignalMapper( this );
  connect( mMessageTagToggleMapper, SIGNAL(mapped(QString)),
           this, SIGNAL(tagActionTriggered(QString)) );

  // Create a action for each tag and plug it into various places
  int i = 0;
  const int numberOfTag(mTags.count());
  foreach( const MailCommon::Tag::Ptr &tag, mTags ) {
    if(tag->tagStatus)
      continue;
    if ( i< s_numberMaxTag )
      createTagAction( tag,true );
    else
    {
      if ( tag->inToolbar || !tag->shortcut.isEmpty() ) {
        createTagAction( tag, false );
      }

      if ( i == s_numberMaxTag && i < numberOfTag )
      {
        mSeparatorAction = new KAction( this );
        mSeparatorAction->setSeparator( true );
        mMessageActions->messageStatusMenu()->menu()->addAction( mSeparatorAction );

        mMoreAction = new KAction( i18n( "More..." ), this );
        mMessageActions->messageStatusMenu()->menu()->addAction( mMoreAction );
        connect( mMoreAction, SIGNAL(triggered(bool)),
                 this, SIGNAL(tagMoreActionClicked()) );
      }
    }
    ++i;
  }

  if ( !mToolbarActions.isEmpty() && mGUIClient->factory() ) {
    mGUIClient->plugActionList( "toolbar_messagetag_actions", mToolbarActions );
  }
}

void TagActionManager::newTagEntries (const QList<Nepomuk2::Query::Result> &results)
{
  foreach (const Nepomuk2::Query::Result &result, results) {
    Nepomuk2::Resource resource = result.resource();
    mTags.append( MailCommon::Tag::fromNepomuk( resource ) );
  }
}

void TagActionManager::finishedTagListing()
{
  mTagQueryClient->close();
  mTagQueryClient->deleteLater();
  mTagQueryClient = 0;
  if ( mTags.isEmpty() )
    return;
  qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
  createTagActions();
}


void TagActionManager::updateActionStates( int numberOfSelectedMessages,
                                           const Akonadi::Item &selectedItem )
{
  QMap<QString,KToggleAction*>::const_iterator it = mTagActions.constBegin();
  QMap<QString,KToggleAction*>::const_iterator end = mTagActions.constEnd();
  if ( numberOfSelectedMessages == 1 )
  {
    Q_ASSERT( selectedItem.isValid() );
    Nepomuk2::Resource itemResource( selectedItem.url() );
    for ( ; it != end; ++it ) {
      const bool hasTag = itemResource.tags().contains( Nepomuk2::Tag( it.key() ) );
      it.value()->setChecked( hasTag );
      it.value()->setEnabled( true );
    }
  }
  else if ( numberOfSelectedMessages > 1 ) {
    for ( ; it != end; ++it ) {
      Nepomuk2::Tag tag( it.key() );
      it.value()->setChecked( false );
      it.value()->setEnabled( true );
      it.value()->setText( i18n("Toggle Message Tag %1", tag.label() ) );
    }
  }
  else {
    for ( ; it != end; ++it ) {
      it.value()->setEnabled( false );
    }
  }
}

void TagActionManager::tagsChanged()
{
  mTags.clear(); // re-read the tags
  createActions();
}

void TagActionManager::resourceCreated(const Nepomuk2::Resource& res,const QList<QUrl>&)
{
    clearActions();
    mTags.append( MailCommon::Tag::fromNepomuk( res ) );
    qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
    createTagActions();
}

void TagActionManager::resourceRemoved(const QUrl& url,const QList<QUrl>&)
{
    foreach( const MailCommon::Tag::Ptr &tag, mTags ) {
        if(tag->nepomukResourceUri == url) {
            mTags.removeAll(tag);
            break;
        }
    }
    clearActions();
    qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
    createTagActions();
}

void TagActionManager::propertyChanged(const Nepomuk2::Resource& res)
{
    foreach( const MailCommon::Tag::Ptr &tag, mTags ) {
        if(tag->nepomukResourceUri == res.uri()) {
            mTags.removeAll(tag);
            break;
        }
    }
    mTags.append( MailCommon::Tag::fromNepomuk( res ) );
    clearActions();
    qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
    createTagActions();
}

#include "tagactionmanager.moc"
