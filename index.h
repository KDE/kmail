#ifndef LPC_INDEX_H1110724080_INCLUDE_GUARD_
#define LPC_INDEX_H1110724080_INCLUDE_GUARD_

/* This file is part of KMail
 * Copyright (C) 2005 Lu�s Pedro Coelho <luis@luispedro.org>
 * (based on the old kmmsgindex by Sam Magnuson)
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


#include <qobject.h>
#include <qcstring.h>
#include <qvaluelist.h>
#include <qtimer.h>
#include <index/index.h>
#include <index/lockfile.h>
#include <vector>
#include <set>

class KMFolder;

class KMSearch;
class KMSearchRule;
class KMSearchPattern;

class KMMsgIndex : public QObject {
	Q_OBJECT
	public:
		explicit KMMsgIndex( QObject* parent );
		~KMMsgIndex();

	public:
		
		/**
		 * Starts a query.
		 * Results will be returned assyncronously by signals.
		 * 
		 * @return false if the query cannot be handled
		 */
		bool startQuery( KMSearch* );
		bool stopQuery( KMSearch* );

		/**
		 * Just return all the uids where the pattern exists
		 */
		std::vector<Q_UINT32> simpleSearch( QString ) const;
	public slots:
		void clear();
		void create();
		void cleanUp();

	private slots:
		void act();
		void removeSearch( QObject* );

		//void syncIndex();
		//void startSync();
		void continueCreation();

		void slotAddMessage( KMFolder*, Q_UINT32 message );
		void slotRemoveMessage( KMFolder*, Q_UINT32 message );
	private:
		static QString defaultPath();

		bool canHandleQuery( const KMSearchPattern* ) const;
		bool isIndexed( const KMFolder* ) const;
		int addMessage( Q_UINT32 );
		void removeMessage( Q_UINT32 );

		void scheduleAction();
		bool creating() const;

	public:
		/**
		 * @internal
		 * DO NOT USE THIS CLASS
		 *
		 * It is conceptually a private class.
		 * Just needs to be public because of moc limitations
		 */
		class Search;
	private:

		std::vector<Q_UINT32> mPendingMsgs;
		std::vector<KMFolder*> mPendingFolders;
		std::vector<Q_UINT32> mAddedMsgs;
		std::vector<Q_UINT32> mRemovedMsgs;
		std::vector<Q_UINT32> mExisting;

		enum e_state {
			s_idle, // doing nothing
			s_willcreate, // just constructed, create() scheduled (mIndex == 0)
			s_creating, // creating the index from the messages
			s_processing // has messages to process
		} mState;

		unsigned mMaintenanceCount;

		/**
		 * The lock below should be moved down into libindex itself
		 * where things can be handled with a lot more granularity
		 */
		indexlib::detail::lockfile mLockFile;
		//enum e_syncState { ss_none, ss_started, ss_synced } mSyncState;
		//QTimer* mSyncTimer;

		std::set<KMFolder*> mOpenedFolders;
		std::vector<Search*> mSearches;
		indexlib::index* mIndex;
		QCString mIndexPath;
		QTimer* mTimer;
		bool mSlowDown;
};


class KMMsgIndex::Search : public QObject {
	Q_OBJECT
	public:
		explicit Search( KMSearch* s );
		~Search();
		KMSearch* search() const { return mSearch; }
	signals:
		void found( Q_UINT32 );
		void finished( bool );
	private slots:
		void act();
	private:
		KMSearch* mSearch;
		QTimer* mTimer;
		/**
		 * mResidual is the part of the pattern which is not
		 * handled by the index
		 */
		KMSearchPattern* mResidual;
		std::vector<Q_UINT32> mValues;
		enum { s_none = 0, s_starting, s_emitting, s_emitstopped, s_done } mState;
};

#endif /* LPC_INDEX_H1110724080_INCLUDE_GUARD_ */
