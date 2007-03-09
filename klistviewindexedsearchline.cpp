
/* This file is part of KMail
 * Copyright (C) 2005 Lu� Pedro Coelho <luis@luispedro.org>
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

#include "config.h"

#include "klistviewindexedsearchline.h"
#include <kdebug.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "headeritem.h"
#include "kmheaders.h"
#include "kmfolder.h"
#include "index.h"

using KMail::HeaderListQuickSearch;

KListViewIndexedSearchLine::KListViewIndexedSearchLine( QToolBar* parent, K3ListView* listView, KActionCollection* actionCollection ):
	HeaderListQuickSearch( parent, listView, actionCollection ),
	mFiltering( false )
{
}

KListViewIndexedSearchLine::~KListViewIndexedSearchLine() {
}


void KListViewIndexedSearchLine::updateSearch( const QString& s ) {
	kDebug( 5006 ) << "updateSearch( -" << s << "- )" << endl;
	mFiltering = false;
	if ( !s.isNull() && !s.isEmpty() ) {
		bool ok = false;
		KMMsgIndex* index = kmkernel->msgIndex();
		if ( index ) {
			mResults = index->simpleSearch( s, &ok );
			std::sort( mResults.begin(), mResults.end() );
			mFiltering = ok;
		}
	}
	K3ListViewSearchLine::updateSearch( s );
}

bool KListViewIndexedSearchLine::itemMatches( const Q3ListViewItem* item, const QString& s ) const {
	if ( mFiltering &&
			std::binary_search( mResults.begin(), mResults.end(), static_cast<const KMail::HeaderItem*>( item )->msgSerNum() ) )
		return true;
	return KMail::HeaderListQuickSearch::itemMatches( item, s );
}

#include "klistviewindexedsearchline.moc"

