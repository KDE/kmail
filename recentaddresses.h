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
