/* This file is part of KMail
 * Copyright (C) 2005 Luís Pedro Coelho <luis@luispedro.org>
 *
 * KMail is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 * 
 * KMail is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of this program with any edition of
 * the Qt library by Trolltech AS, Norway (or with modified versions
 * of Qt that use the same license as Qt), and distribute linked
 * combinations including the two.  You must obey the GNU General
 * Public License in all respects for all of the code used other than
 * Qt.  If you modify this file, you may extend this exception to
 * your version of the file, but you are not obligated to do so.  If
 * you do not wish to do so, delete this exception statement from
 * your version.
 */

#include "index.h"

#include "kmkernel.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmfolder.h"
#include "kmsearchpattern.h"
#include "kmfoldersearch.h"

#include <kdebug.h>
#include <kapplication.h>
#include <qfile.h>
#include <qtimer.h>
#include <qvaluestack.h>
#include <qfileinfo.h>
#include <index/create.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <algorithm>
#include <cstdlib>

const unsigned int MaintenanceLimit = 1000;

static
QValueList<int> vectorToQValueList( const std::vector<Q_UINT32>& input ) {
	QValueList<int> res;
	std::copy( input.begin(), input.end(), std::back_inserter( res ) );
	return res;
}


static
std::vector<Q_UINT32> QValueListToVector( const QValueList<int>& input ) {
	std::vector<Q_UINT32> res;
	// res.assign( input.begin(), input.end() ) doesn't work for some reason
	for ( QValueList<int>::const_iterator first = input.begin(), past = input.end(); first != past; ++first ) {
		res.push_back( *first );
	}
	return res;
}

KMMsgIndex::KMMsgIndex( QObject* parent ):
	QObject( parent, "index" ),
	mIndex( 0 ),
	mState( s_idle ),
	mIndexPath( QFile::encodeName( defaultPath() ) ),
	mTimer( new QTimer( this ) ),
	mLockFile( std::string( static_cast<const char*>( QFile::encodeName( defaultPath() ) + "/lock" ) ) ),
	//mSyncState( ss_none ),
	//mSyncTimer( new QTimer( this ) ),
	mSlowDown( false ) {
	kdDebug( 5006 ) << "KMMsgIndex::KMMsgIndex()" << endl;
	
	if ( !QFileInfo( mIndexPath ).exists() ) {
		::mkdir( mIndexPath, S_IRWXU );
	}
	
	connect( kmkernel->folderMgr(), SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ), SLOT( slotRemoveMessage( KMFolder*, Q_UINT32 ) ) );
	connect( kmkernel->folderMgr(), SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ), SLOT( slotAddMessage( KMFolder*, Q_UINT32 ) ) );
	connect( kmkernel->dimapFolderMgr(), SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ), SLOT( slotRemoveMessage( KMFolder*, Q_UINT32 ) ) );
	connect( kmkernel->dimapFolderMgr(), SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ), SLOT( slotAddMessage( KMFolder*, Q_UINT32 ) ) );

	connect( mTimer, SIGNAL( timeout() ), SLOT( act() ) );
	//connect( mSyncTimer, SIGNAL( timeout() ), SLOT( syncIndex() ) );

	if ( !mLockFile.trylock() ) {
		indexlib::remove( mIndexPath );
		mLockFile.force_unlock();
		mLockFile.trylock();
	} else {
		mIndex = indexlib::open( mIndexPath, indexlib::open_flags::fail_if_nonexistant ).release();
	}
	if ( !mIndex ) {
		QTimer::singleShot( 8000, this, SLOT( create() ) );
		mState = s_willcreate;
	} else {
		KConfigGroup cfg( KMKernel::config(), "text-index" );
		if ( cfg.readBoolEntry( "creating" ) ) {
			QTimer::singleShot( 8000, this, SLOT( continueCreation() ) );
			mState = s_creating;
		} else {
			mPendingMsgs = QValueListToVector( cfg.readIntListEntry( "pending" ) );
			mRemovedMsgs = QValueListToVector( cfg.readIntListEntry( "removed" ) );
		}
	}
	//if ( mState == s_idle ) mSyncState = ss_synced;
}


KMMsgIndex::~KMMsgIndex() {
	kdDebug( 5006 ) << "KMMsgIndex::~KMMsgIndex()" << endl;
	KConfigGroup cfg( KMKernel::config(), "text-index" );
	cfg.writeEntry( "creating", mState == s_creating );
	QValueList<int> pendingMsg;
	if ( mState == s_processing ) {
		Q_ASSERT( mAddedMsgs.empty() );
		pendingMsg = vectorToQValueList( mPendingMsgs );
	}
	cfg.writeEntry( "pending", pendingMsg );
	cfg.writeEntry( "removed", vectorToQValueList( mRemovedMsgs ) );
	delete mIndex;
}

void KMMsgIndex::clear() {
	delete mIndex;
	mLockFile.force_unlock();
	mIndex = 0;
	indexlib::remove( mIndexPath );
}

void KMMsgIndex::cleanUp() {
	if ( mState != s_idle || kapp->hasPendingEvents() ) {
		QTimer::singleShot( 8000, this, SLOT( cleanUp() ) );
		return;
	}
	mIndex->maintenance();
}

int KMMsgIndex::addMessage( Q_UINT32 serNum ) {
	kdDebug( 5006 ) << "KMMsgIndex::addMessage( " << serNum << " )" << endl;
	assert( mIndex );
	if ( !mExisting.empty() && std::binary_search( mExisting.begin(), mExisting.end(), serNum ) ) return 0;

	int idx = -1;
	KMFolder* folder = 0;
	KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
	if ( !folder || idx == -1 ) return -1;
	if ( !mOpenedFolders.count( folder ) ) {
		mOpenedFolders.insert( folder );
		folder->open();
	}
	KMMessage* msg = folder->getMsg( idx );
	/* I still don't know whether we should allow decryption or not.
	 * Setting to false which makes more sense.
	 * We keep signature to get the person's name
	 */
	QString body = msg->asPlainText( false, false );
	if ( !body.isEmpty() && static_cast<const char*>( body.latin1() ) ) {
		mIndex->add( body.latin1(), QString::number( serNum ).latin1() );
	} else {
		kdDebug( 5006 ) << "Funny, no body" << endl;
	}
	folder->unGetMsg( idx );
	return 0;
}

void KMMsgIndex::act() {
	kdDebug( 5006 ) << "KMMsgIndex::act()" << endl;
	if ( kapp->hasPendingEvents() ) {
		//nah, some other time..
		mTimer->start( 500 );
		mSlowDown = true;
		return;
	}
	if ( mSlowDown ) {
		mSlowDown = false;
		mTimer->start( 0 );
	}
	if ( !mPendingMsgs.empty() ) {
		addMessage( mPendingMsgs.back() );
		mPendingMsgs.pop_back();
		return;
	}
	if ( !mPendingFolders.empty() ) {
		KMFolder *f = mPendingFolders.back();
		mPendingFolders.pop_back();
		if ( !mOpenedFolders.count( f ) ) {
			mOpenedFolders.insert( f );
			f->open();
		}
		KMMsgDict* dict = KMMsgDict::instance();
		for ( int i = 0; i < f->count(); ++i) {
			mPendingMsgs.push_back( dict->getMsgSerNum( f, i ) );
		}
		return;
	}
	if ( !mAddedMsgs.empty() ) {
		std::swap( mAddedMsgs, mPendingMsgs );
		mState = s_processing;
		return;
	}
	for ( std::set<KMFolder*>::const_iterator first = mOpenedFolders.begin(), past = mOpenedFolders.end();
			first != past;
			++first ) {
		( *first )->close();
	}
	mOpenedFolders.clear();
	mState = s_idle;
	mTimer->stop();
}

void KMMsgIndex::continueCreation() {
	kdDebug( 5006 ) << "KMMsgIndex::continueCreation()" << endl;
	create();
	unsigned count = mIndex->ndocs();
	mExisting.clear();
	mExisting.reserve( count );
	for ( unsigned i = 0; i != count; ++i ) {
		mExisting.push_back( std::atoi( mIndex->lookup_docname( i ).c_str() ) );
	}
	std::sort( mExisting.begin(), mExisting.end() );
}

void KMMsgIndex::create() {
	kdDebug( 5006 ) << "KMMsgIndex::create()" << endl;
	mState = s_creating;
	if ( !mIndex ) mIndex = indexlib::create( mIndexPath ).release();
	Q_ASSERT( mIndex );
	QValueStack<KMFolderDir*> folders;
	folders.push(&(kmkernel->folderMgr()->dir()));
	folders.push(&(kmkernel->dimapFolderMgr()->dir()));
	while ( !folders.empty() ) {
		KMFolderDir *dir = folders.pop();
		for(KMFolderNode *child = dir->first(); child; child = dir->next()) {
			if ( child->isDir() )
				folders.push((KMFolderDir*)child);
			else
				mPendingFolders.push_back( (KMFolder*)child );
		}
	}
	mTimer->start( 4000 ); // wait a couple of seconds before starting up...
	mSlowDown = true;
}

bool KMMsgIndex::startQuery( KMSearch* s ) {
	kdDebug( 5006 ) << "KMMsgIndex::startQuery( . )" << endl;
	if ( mState == s_creating ) return false;
	if ( !isIndexed( s->root() ) || !canHandleQuery( s->searchPattern() ) ) return false;

	kdDebug( 5006 ) << "KMMsgIndex::startQuery( . ) starting query" << endl;
	Search* search = new Search( s );
	connect( search, SIGNAL( finished( bool ) ), s, SIGNAL( finished( bool ) ) );
	connect( search, SIGNAL( finished( bool ) ), s, SLOT( indexFinished() ) );
	connect( search, SIGNAL( destroyed( QObject* ) ), SLOT( removeSearch( QObject* ) ) );
	connect( search, SIGNAL( found( Q_UINT32 ) ), s, SIGNAL( found( Q_UINT32 ) ) );
	mSearches.push_back( search );
	return true;
}


//void KMMsgIndex::startSync() {
//	switch ( mSyncState ) {
//		case ss_none:
//			mIndex->start_sync();
//			mSyncState = ss_started;
//			mSyncTimer.start( 4000, true );
//			break;
//		case ss_started:
//			mIndex->sync_now();
//			mSyncState = ss_synced;
//			mLockFile.unlock();
//			break;
//	}
//}
//
//void KMMsgIndex::finishSync() {
//	
//}

void KMMsgIndex::removeSearch( QObject* destroyed ) {
	mSearches.erase( std::find( mSearches.begin(), mSearches.end(), destroyed ) );
}

bool KMMsgIndex::isIndexed( const KMFolder* folder ) const {
	if ( mState == s_creating || mState == s_willcreate ) return false;
	const KMFolderMgr* manager = folder->parent()->manager();
	return manager == kmkernel->folderMgr() || manager == kmkernel->dimapFolderMgr();
}

bool KMMsgIndex::stopQuery( KMSearch* s ) {
	kdDebug( 5006 ) << "KMMsgIndex::stopQuery( . )" << endl;
	for ( std::vector<Search*>::iterator iter = mSearches.begin(), past = mSearches.end(); iter != past; ++iter ) {
		if ( ( *iter )->search() == s ) {
			delete *iter;
			mSearches.erase( iter );
			return true;
		}
	}
	return false;
}

std::vector<Q_UINT32> KMMsgIndex::simpleSearch( QString s ) const {
	kdDebug( 5006 ) << "KMMsgIndex::simpleSearch( -" << s.latin1() << "- )" << endl;
	std::vector<Q_UINT32> res;
	assert( mIndex );
	std::vector<unsigned> residx = mIndex->search( s.latin1() )->list();
	res.reserve( residx.size() );
	for ( std::vector<unsigned>::const_iterator first = residx.begin(), past = residx.end();first != past; ++first ) {
		res.push_back( std::atoi( mIndex->lookup_docname( *first ).c_str() ) );
	}
	return res;
}

bool KMMsgIndex::canHandleQuery( const KMSearchPattern* pat ) const {
	kdDebug( 5006 ) << "KMMsgIndex::canHandleQuery( . )" << endl;
	for ( KMSearchRule* rule = pat->first(); rule; rule = pat->next() ) {
		if ( !rule->field().isEmpty() && !rule->contents().isEmpty() &&
				rule->function() == KMSearchRule::FuncContains &&
				rule->field() == "<body>" )  return true;
	}
	return false;
}

void KMMsgIndex::slotAddMessage( KMFolder* folder, Q_UINT32 serNum ) {
	kdDebug( 5006 ) << "KMMsgIndex::slotAddMessage( . , " << serNum << " )" << endl;
	
	if ( mState == s_creating ) mAddedMsgs.push_back( serNum );
	else mPendingMsgs.push_back( serNum );

	if ( mState == s_idle ) mState = s_processing;
	scheduleAction();
}

void KMMsgIndex::slotRemoveMessage( KMFolder* folder, Q_UINT32 serNum ) {
	kdDebug( 5006 ) << "KMMsgIndex::slotRemoveMessage( . , " << serNum << " )" << endl;
	if ( mState == s_idle ) mState = s_processing;
	mRemovedMsgs.push_back( serNum );
	scheduleAction();
}

void KMMsgIndex::scheduleAction() {
	if ( mState == s_willcreate || !mIndex ) return;
	if ( !mSlowDown ) mTimer->start( 0 );
}

void KMMsgIndex::removeMessage( Q_UINT32 serNum ) {
	mIndex->remove_doc( QString::number( serNum ).latin1() );
	++mMaintenanceCount;
	if ( mMaintenanceCount > MaintenanceLimit && mRemovedMsgs.empty() ) {
		QTimer::singleShot( 100, this, SLOT( cleanUp() ) );
	}
}

QString KMMsgIndex::defaultPath() {
	return KMKernel::localDataPath() + "text-index";
}

bool KMMsgIndex::creating() const {
	return !mPendingMsgs.empty() || !mPendingFolders.empty();
}

KMMsgIndex::Search::Search( KMSearch* s ):
	mSearch( s ),
	mTimer( new QTimer( this ) ),
	mState( s_starting ),
	mResidual( new KMSearchPattern ) {
	connect( mTimer, SIGNAL( timeout() ), SLOT( act() ) );
	mTimer->start( 0 );
}

KMMsgIndex::Search::~Search() {
	delete mTimer;
}

void KMMsgIndex::Search::act() {
	switch ( mState ) {
		case s_starting: {
			KMSearchPattern* pat = mSearch->searchPattern();
			QString terms;
			for ( KMSearchRule* rule = pat->first(); rule; rule = pat->next() ) {
				Q_ASSERT( rule->function() == KMSearchRule::FuncContains );
				terms += QString::fromLatin1( " %1 " ).arg( rule->contents() );
			}

			mValues = kmkernel->msgIndex()->simpleSearch( terms );
			mState = s_emitting;
			break;
		 }
		case s_emitstopped:
			mTimer->start( 0 );
			mState = s_emitting;
			// fall throu
		case s_emitting:
			if ( kapp->hasPendingEvents() ) {
				//nah, some other time..
				mTimer->start( 250 );
				mState = s_emitstopped;
				return;
			}
			for ( int i = 0; i != 16 && !mValues.empty(); ++i ) {
				KMFolder* folder;
				int index;
				KMMsgDict::instance()->getLocation( mValues.back(), &folder, &index );
				if ( folder &&
					mSearch->inScope( folder ) &&
					( !mResidual || mResidual->matches( mValues.back() ) ) ) {

					emit found( mValues.back() );
				}
				mValues.pop_back();
			}
			if ( mValues.empty() ) {
				emit finished( true );
				mState = s_done;
				mTimer->stop();
				delete this;
			}
			break;
		default:
		Q_ASSERT( 0 );
	}
}
#include "index.moc"

