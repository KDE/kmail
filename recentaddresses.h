/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2001-2003 Carsten Pfeiffer <pfeiffer@kde.org>
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef KMRECENTADDR_H
#define KMRECENTADDR_H

#include <qstringlist.h>
#include <kabc/addressee.h>

class KConfig;
namespace KMail {

/**
 * Handles a list of "recent email-addresses". Simply set a max-count and
 * call @ref add() to add entries.
 *
 * @author Carsten Pfeiffer <pfeiffer@kde.org>
 */

class RecentAddresses
{
    friend class foobar; // private destructor

public:
    /**
     * @returns the only possible instance of this class.
     */
    static RecentAddresses * self();

    /**
     * @returns the list of recent addresses.
     * Note: an entry doesn't have to be one email address, it can be multiple,
     * like "Foo <foo@bar.org>, Bar Baz <bar@baz.org>".
     */
    QStringList     addresses() const;
    const KABC::Addressee::List& kabcAddresses() const { return m_addresseeList; }

    /**
     * Adds an entry to the list.
     * Note: an entry doesn't have to be one email address, it can be multiple,
     * like "Foo <foo@bar.org>, Bar Baz <bar@baz.org>".
     */
    void add( const QString& entry );

    /**
     * Sets the maximum number, the list can hold. The list adjusts to this
     * size if necessary. Default maximum is 40.
     */
    void setMaxCount( int count );

    /**
     * @returns the current maximum number of entries.
     */
    uint maxCount() const { return m_maxCount; }

    /**
     * Loads the list of recently used addresses from the configfile.
     * Automatically done on startup.
     */
    void load( KConfig * );

    /**
     * Saves the list of recently used addresses to the configfile.
     * Make sure to call KGlobal::config()->sync() afterwards, to really save.
     */
    void save( KConfig * );

private:
    RecentAddresses();
    ~RecentAddresses();

    KABC::Addressee::List m_addresseeList;

    void adjustSize();

    uint m_maxCount;

    static RecentAddresses *s_self;
};

};

#endif // KMRECENTADDR_H
