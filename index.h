#ifndef LPC_INDEX_H1110724080_INCLUDE_GUARD_
#define LPC_INDEX_H1110724080_INCLUDE_GUARD_

/* This file is part of KMail
 * Copyright (C) 2005 Lu� Pedro Coelho <luis@luispedro.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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


#include <QObject>
#include <q3cstring.h>

#include <QTimer>
#include <config.h>
#include <config-kmail.h>
#ifdef HAVE_INDEXLIB
#include <indexlib/index.h>
#include <indexlib/lockfile.h>
#endif
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
		/**
		 * Stops a query. nop if the query isn't running anymore.
		 *
		 * @return true if the query was found
		 */
		bool stopQuery( KMSearch* );

		/**
		 * Just return all the uids where the pattern exists
		 */
		std::vector<quint32> simpleSearch( QString, bool* ) const;

		/**
		 * Returns whether the folder is indexable. Only local and dimap
		 * folders are currently indexable.
		 *
		 * Note that a folder might be indexable and not indexed if the user has
		 * disabled it. @see isIndexed
		 */
		bool isIndexable( KMFolder* folder ) const;

		/**
		 * Returns whether the folder has indexing enabled.
		 *
		 * This returns true immediatelly after indexing has been enabled
		 * even though the folder is probably still being indexed in the background.
		 */
		bool isIndexed( KMFolder* folder ) const;

		/**
		 * Returns whether the index is enabled
		 */
		bool isEnabled() const { return mState != s_disabled; }
	public slots:
		/**
		 * Either enable or disable indexing.
		 *
		 * Calling setEnabled( true ) will start building the whole index,
		 * which is an expensive operation (time and disk-space).
		 *
		 * Calling setEnabled( false ) will remove the index immediatelly,
		 * freeing up disk-space.
		 */
		void setEnabled( bool );

		/**
		 * Change the indexing override for a given folder
		 *
		 * If called with true, will start indexing all the messages in the folder
		 * If called with false will remove all the messages in the folder
		 */
		void setIndexingEnabled( KMFolder*, bool );

	private slots:
		/**
		 * Removes the index.
		 *
		 * If the index is enabled, then creation will start on the next time
		 * KMMsgIndex is constructed.
		 */
		void clear();
		void create();
		void maintenance();

		void act();
		void removeSearch( QObject* );

		void continueCreation();

		void slotAddMessage( quint32 message );
		void slotRemoveMessage( quint32 message );
	private:
		static QString defaultPath();

		bool canHandleQuery( const KMSearchPattern* ) const;
		int addMessage( quint32 );
		void removeMessage( quint32 );

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

		std::vector<quint32> mPendingMsgs;
		std::vector<KMFolder*> mPendingFolders;
		std::vector<quint32> mAddedMsgs;
		std::vector<quint32> mRemovedMsgs;
		std::vector<quint32> mExisting;

		enum e_state {
			s_idle, // doing nothing, index waiting
			s_willcreate, // just constructed, create() scheduled (mIndex == 0)
			s_creating, // creating the index from the messages
			s_processing, // has messages to process
			s_error, // an error occurred
			s_disabled // disabled: the index is not working
		} mState;

		unsigned mMaintenanceCount;

#ifdef HAVE_INDEXLIB
		/**
		 * The lock below should be moved down into libindex itself
		 * where things can be handled with a lot more granularity
		 */
		indexlib::detail::lockfile mLockFile;
		//enum e_syncState { ss_none, ss_started, ss_synced } mSyncState;
		//QTimer* mSyncTimer;

		indexlib::index* mIndex;
#endif
		std::set<KMFolder*> mOpenedFolders;
		std::vector<Search*> mSearches;
		QByteArray mIndexPath;
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
		void found( quint32 );
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
		std::vector<quint32> mValues;
		enum { s_none = 0, s_starting, s_emitting, s_emitstopped, s_done } mState;
};

#endif /* LPC_INDEX_H1110724080_INCLUDE_GUARD_ */
