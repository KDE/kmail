/* Copyright 2010 Thomas McGuire <mcguire@kde.org>
   Copyright 2011-2012-2013 Laurent Montel <montel@kde.org>

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

#include "mailcommon/tag/addtagdialog.h"

#include <KAction>
#include <KActionCollection>
#include <KToggleAction>
#include <KXMLGUIClient>
#include <KActionMenu>
#include <KMenu>
#include <KLocalizedString>
#include <KJob>
#include <Akonadi/Monitor>

#include <QSignalMapper>
#include <QPointer>

#include <Akonadi/TagFetchJob>
#include <Akonadi/TagFetchScope>
#include <Akonadi/TagAttribute>

using namespace KMail;

static int s_numberMaxTag = 10;

TagActionManager::TagActionManager( QObject *parent, KActionCollection *actionCollection,
                                    MessageActions *messageActions, KXMLGUIClient *guiClient )
    : QObject( parent ),
      mActionCollection( actionCollection ),
      mMessageActions( messageActions ),
      mMessageTagToggleMapper( 0 ),
      mGUIClient( guiClient ),
      mSeparatorMoreAction( 0 ),
      mSeparatorNewTagAction( 0 ),
      mMoreAction( 0 ),
      mNewTagAction( 0 ),
      mTagFetchInProgress( false ),
      mMonitor(new Akonadi::Monitor(this))
{
    mMessageActions->messageStatusMenu()->menu()->addSeparator();

    mMonitor->setTypeMonitored(Akonadi::Monitor::Tags);
    mMonitor->tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(mMonitor, SIGNAL(tagAdded(Akonadi::Tag)), this, SLOT(onTagAdded(Akonadi::Tag)));
    connect(mMonitor, SIGNAL(tagRemoved(Akonadi::Tag)), this, SLOT(onTagRemoved(Akonadi::Tag)));
    connect(mMonitor, SIGNAL(tagChanged(Akonadi::Tag)), this, SLOT(onTagChanged(Akonadi::Tag)));
}

TagActionManager::~TagActionManager()
{
}

void TagActionManager::clearActions()
{
    //Remove the tag actions from the toolbar
    if ( !mToolbarActions.isEmpty() ) {
        if ( mGUIClient->factory() ) {
            mGUIClient->unplugActionList( QLatin1String("toolbar_messagetag_actions") );
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

    if ( mSeparatorMoreAction ) {
        mMessageActions->messageStatusMenu()->removeAction( mSeparatorMoreAction );
    }

    if ( mSeparatorNewTagAction ) {
        mMessageActions->messageStatusMenu()->removeAction( mSeparatorNewTagAction );
    }

    if ( mNewTagAction ) {
        mMessageActions->messageStatusMenu()->removeAction( mNewTagAction );
    }

    if ( mMoreAction ) {
        mMessageActions->messageStatusMenu()->removeAction( mMoreAction );
    }


    mTagActions.clear();
    delete mMessageTagToggleMapper;
    mMessageTagToggleMapper = 0;
}

void TagActionManager::createTagAction( const MailCommon::Tag::Ptr &tag, bool addToMenu )
{
    QString cleanName( i18n("Message Tag %1", tag->tagName ) );
    cleanName.replace(QLatin1Char('&'), QLatin1String("&&"));
    KToggleAction * const tagAction = new KToggleAction( KIcon( tag->iconName ),
                                                         cleanName, this );
    tagAction->setShortcut( tag->shortcut );
    tagAction->setIconText( tag->name() );
    tagAction->setChecked( tag->id() == mNewTagId );

    mActionCollection->addAction( tag->name(), tagAction );
    connect( tagAction, SIGNAL(triggered(bool)),
             mMessageTagToggleMapper, SLOT(map()) );

    // The shortcut configuration is done in the config dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the tag descriptions correctly.
    tagAction->setShortcutConfigurable( false );
    mMessageTagToggleMapper->setMapping( tagAction, QString::number(tag->tag().id()) );

    mTagActions.insert( tag->id(), tagAction );
    if ( addToMenu )
        mMessageActions->messageStatusMenu()->menu()->addAction( tagAction );

    if ( tag->inToolbar ) {
        mToolbarActions.append( tagAction );
    }
}

void TagActionManager::createActions()
{
    if ( mTagFetchInProgress )
      return
    clearActions();

    if ( mTags.isEmpty() ) {
        mTagFetchInProgress = true;
        Akonadi::TagFetchJob *fetchJob = new Akonadi::TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(finishedTagListing(KJob*)));
    } else {
        createTagActions(mTags);
    }
}

void TagActionManager::finishedTagListing(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
    }
    Akonadi::TagFetchJob *fetchJob = static_cast<Akonadi::TagFetchJob*>(job);
    foreach (const Akonadi::Tag &result, fetchJob->tags()) {
        mTags.append( MailCommon::Tag::fromAkonadi( result ) );
    }
    mTagFetchInProgress = false;
    qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
    createTagActions(mTags);
}

void TagActionManager::onSignalMapped(const QString& tag)
{
    emit tagActionTriggered( Akonadi::Tag( tag.toLongLong() ) );
}

void TagActionManager::createTagActions(const QList<MailCommon::Tag::Ptr> &tags)
{
    //Use a mapper to understand which tag button is triggered
    mMessageTagToggleMapper = new QSignalMapper( this );
    connect( mMessageTagToggleMapper, SIGNAL(mapped(QString)),
             this, SLOT(onSignalMapped(QString)) );

    // Create a action for each tag and plug it into various places
    int i = 0;
    bool needToAddMoreAction = false;
    const int numberOfTag(tags.size());
    //It is assumed the tags are sorted
    foreach( const MailCommon::Tag::Ptr &tag, tags ) {
        if ( i< s_numberMaxTag )
            createTagAction( tag,true );
        else
        {
            if ( tag->inToolbar || !tag->shortcut.isEmpty() ) {
                createTagAction( tag, false );
            }

            if ( i == s_numberMaxTag && i < numberOfTag )
            {
                needToAddMoreAction = true;
            }
        }
        ++i;
    }


    if(!mSeparatorNewTagAction) {
        mSeparatorNewTagAction = new QAction( this );
        mSeparatorNewTagAction->setSeparator( true );
    }
    mMessageActions->messageStatusMenu()->menu()->addAction( mSeparatorNewTagAction );

    if (!mNewTagAction) {
        mNewTagAction = new KAction( i18n( "Add new tag..." ), this );
        connect( mNewTagAction, SIGNAL(triggered(bool)),
                 this, SLOT(newTagActionClicked()) );
    }
    mMessageActions->messageStatusMenu()->menu()->addAction( mNewTagAction );

    if (needToAddMoreAction) {
        if(!mSeparatorMoreAction) {
            mSeparatorMoreAction = new QAction( this );
            mSeparatorMoreAction->setSeparator( true );
        }
        mMessageActions->messageStatusMenu()->menu()->addAction( mSeparatorMoreAction );

        if (!mMoreAction) {
            mMoreAction = new KAction( i18n( "More..." ), this );
            connect( mMoreAction, SIGNAL(triggered(bool)),
                     this, SIGNAL(tagMoreActionClicked()) );
        }
        mMessageActions->messageStatusMenu()->menu()->addAction( mMoreAction );
    }

    if ( !mToolbarActions.isEmpty() && mGUIClient->factory() ) {
        mGUIClient->plugActionList( QLatin1String("toolbar_messagetag_actions"), mToolbarActions );
    }
}

void TagActionManager::updateActionStates( int numberOfSelectedMessages,
                                           const Akonadi::Item &selectedItem )
{
    mNewTagId = -1;
    QMap<qint64,KToggleAction*>::const_iterator it = mTagActions.constBegin();
    QMap<qint64,KToggleAction*>::const_iterator end = mTagActions.constEnd();
    if ( numberOfSelectedMessages >= 1 ) {
        Q_ASSERT( selectedItem.isValid() );
        for ( ; it != end; ++it ) {
            //FIXME Not very performant tag label retrieval
            QString label(QLatin1String("not found"));
            foreach (const MailCommon::Tag::Ptr &tag, mTags) {
                if (tag->id() == it.key()) {
                    label = tag->name();
                    break;
                }
            }

            it.value()->setEnabled( true );
            if (numberOfSelectedMessages == 1) {
                const bool hasTag = selectedItem.hasTag(Akonadi::Tag(it.key()));
                it.value()->setChecked( hasTag );
                it.value()->setText( i18n("Message Tag %1", label ) );
            } else {
                it.value()->setChecked( false );
                it.value()->setText( i18n("Toggle Message Tag %1", label ) );
            }
        }
    } else {
        for ( ; it != end; ++it ) {
            it.value()->setEnabled( false );
        }
    }
}

void TagActionManager::onTagAdded(const Akonadi::Tag &akonadiTag)
{
    const QList<qint64> checked = checkedTags();

    clearActions();
    mTags.append( MailCommon::Tag::fromAkonadi( akonadiTag ) );
    qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
    createTagActions(mTags);

    checkTags( checked );
}

void TagActionManager::onTagRemoved(const Akonadi::Tag &akonadiTag)
{
    foreach( const MailCommon::Tag::Ptr &tag, mTags ) {
        if(tag->id() == akonadiTag.id()) {
            mTags.removeAll(tag);
            break;
        }
    }

    fillTagList();
}

void TagActionManager::onTagChanged(const Akonadi::Tag& akonadiTag)
{
    foreach( const MailCommon::Tag::Ptr &tag, mTags ) {
        if(tag->id() == akonadiTag.id()) {
            mTags.removeAll(tag);
            break;
        }
    }
    mTags.append( MailCommon::Tag::fromAkonadi( akonadiTag ) );
    fillTagList();
}

void TagActionManager::fillTagList()
{
    const QList<qint64> checked = checkedTags();

    clearActions();
    qSort( mTags.begin(), mTags.end(), MailCommon::Tag::compare );
    createTagActions( mTags );

    checkTags( checked );
}

void TagActionManager::newTagActionClicked()
{
    QPointer<MailCommon::AddTagDialog> dialog = new MailCommon::AddTagDialog(QList<KActionCollection*>() << mActionCollection, 0);
    dialog->setTags(mTags);
    if ( dialog->exec() == QDialog::Accepted ) {
        mNewTagId = dialog->tag().id();
        // Assign tag to all selected items right away
        emit tagActionTriggered( dialog->tag() );
    }
    delete dialog;
}

void TagActionManager::checkTags(const QList<qint64>& tags)
{
    foreach( const qint64 &id, tags ) {
        if ( mTagActions.contains(id) ) {
            mTagActions[id]->setChecked( true );
        }
    }
}

QList<qint64> TagActionManager::checkedTags() const
{
    QMap<qint64,KToggleAction*>::const_iterator it = mTagActions.constBegin();
    QMap<qint64,KToggleAction*>::const_iterator end = mTagActions.constEnd();
    QList<qint64> checked;
    for ( ; it != end; ++it ) {
        if ( it.value()->isChecked() ) {
            checked << it.key();
        }
    }
    return checked;
}
