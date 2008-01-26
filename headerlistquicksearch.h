/*  -*- mode: C++ -*-
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Till Adam <adam@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KMAILHEADERLISTQUICKSEARCH_H
#define KMAILHEADERLISTQUICKSEARCH_H

#include "kmmsgbase.h" // for KMMsgStatus
#include "kmsearchpattern.h"
#include <klistviewsearchline.h>
#include <qvaluevector.h>
class QComboBox;
class QLabel;
class KListView;
class KActionCollection;

namespace KMail {

class HeaderListQuickSearch : public KListViewSearchLine
{
Q_OBJECT
public:
    HeaderListQuickSearch( QWidget *parent,
                                             KListView *listView,
                                             KActionCollection *actionCollection,
                                             const char *name = 0 );
    virtual ~HeaderListQuickSearch();

    /**
     * Used to disable the main window's accelerators when the search widget's
     * combo has focus
     */
    bool eventFilter( QObject *watched, QEvent *event );

    /**
     * Returns the currently entered search text.
     */
    QString currentSearchTerm() const;

    /**
     * Returns the currently selected status filter.
     */
    int currentStatus() const;

public slots:
   void reset();

signals:
    void requestFullSearch();

protected:
    /**
     * checks whether @param item contains the search string and has the status
     * currently in mStatus
     */
    virtual bool itemMatches(const QListViewItem *item, const QString &s) const;

private slots:
  /**
   * cache the status in mStatus so as to avoid having to do the comparatively
   * expensive string comparison for each item in itemMatches
   */
  void slotStatusChanged( int index );

private:
    /** Helper method for the filling of the status combo. */
    void insertStatus(KMail::StatusValueTypes which);
    QComboBox *mStatusCombo;
    KMMsgStatus mStatus;
    QValueVector<QString> statusList;
    mutable QString mCurrentSearchTerm;
};

}

#endif
