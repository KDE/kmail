#include <kconfig.h>
#include <kglobal.h>

#include "kmrecentaddr.h"

KMRecentAddresses * KMRecentAddresses::s_self = 0L;

KMRecentAddresses * KMRecentAddresses::self()
{
    if ( !s_self )
        s_self = new KMRecentAddresses();
    return s_self;
}

KMRecentAddresses::KMRecentAddresses()
{
    load( KGlobal::config() );
}

KMRecentAddresses::~KMRecentAddresses()
{
}

void KMRecentAddresses::load( KConfig *config )
{
    m_addresses.clear();
    KConfigGroupSaver cs( config, "General" );
    m_maxCount = config->readNumEntry( "Maximum Recent Addresses", 40 );
    m_addresses = config->readListEntry( "Recent Addresses" );

    adjustSize();
}

void KMRecentAddresses::save( KConfig *config )
{
    KConfigGroupSaver cs( config, "General" );
    config->writeEntry( "Recent Addresses", m_addresses );
}

void KMRecentAddresses::add( const QString& entry )
{
    if ( !entry.isEmpty() && m_maxCount > 0 && m_addresses.first() != entry ) {
        m_addresses.remove( entry ); // removes all existing "entry" items
        m_addresses.prepend( entry );
        adjustSize();
    }
}

void KMRecentAddresses::setMaxCount( int count )
{
    m_maxCount = count;
    adjustSize();
}

void KMRecentAddresses::adjustSize()
{
    // for convenience, we don't sync the completion objects here.
    while ( m_addresses.count() > m_maxCount )
        m_addresses.remove( m_addresses.fromLast() );
}
