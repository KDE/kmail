#ifndef LPC_KLISTVIEWINDEXEDSEARCHLINE_H1107549660_INCLUDE_GUARD_
#define LPC_KLISTVIEWINDEXEDSEARCHLINE_H1107549660_INCLUDE_GUARD_

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


#include <klineedit.h>
#include <k3listviewsearchline.h>
#include "headerlistquicksearch.h"
#include <q3hbox.h>
#include <QToolBar>

#include <vector>

class K3ListView;
class Q3ListViewItem;
class QToolButton;
class K3ListViewSearchLine;

/**
 * Extends HeaderListQuickSearch to also search inside message bodies using KMMsgIndex
 */

class KDEUI_EXPORT KListViewIndexedSearchLine: public KMail::HeaderListQuickSearch
{
    Q_OBJECT

public:

    explicit KListViewIndexedSearchLine( QToolBar *parent, K3ListView *listView, KActionCollection* action );
    ~KListViewIndexedSearchLine();

public slots:
    /**
     * Updates search to only make visible the items that match \a s.  If
     * \a s is null then the line edit's text will be used.
     */
    virtual void updateSearch(const QString &s = QString());

protected:
    virtual bool itemMatches(const Q3ListViewItem *item, const QString &s) const;

private:
    std::vector<unsigned> mResults;
    bool mFiltering;

};



#endif /* LPC_KLISTVIEWINDEXEDSEARCHLINE_H1107549660_INCLUDE_GUARD_ */
