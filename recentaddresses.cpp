#include <kconfig.h>
#include <kglobal.h>

#include <kdebug.h>
#include "recentaddresses.h"

KMail::RecentAddresses * KMail::RecentAddresses::s_self = 0;

KMail::RecentAddresses * KMail::RecentAddresses::self()
{
    if ( !s_self )
        s_self = new KMail::RecentAddresses();
    return s_self;
}

KMail::RecentAddresses::RecentAddresses()
{
    load( KGlobal::config() );
}

KMail::RecentAddresses::~RecentAddresses()
{
    // if you want this destructor to get called, use a KStaticDeleter
    // on s_self
}

void KMail::RecentAddresses::load( KConfig *config )
{
    QStringList addresses;
    QString name;
    QString email;

    m_addresseeList.clear();
    KConfigGroupSaver cs( config, "General" );
    m_maxCount = config->readNumEntry( "Maximum Recent Addresses", 40 );
    addresses = config->readListEntry( "Recent Addresses" );
    for ( QStringList::Iterator it = addresses.begin(); it != addresses.end(); ++it ) {
        KABC::Addressee::parseEmailAddress( *it, name, email );
        if ( !email.isEmpty() ) {
            KABC::Addressee addr;
            addr.setNameFromString( name );
            addr.insertEmail( email, true );
            m_addresseeList.append( addr );
        }
    }

    adjustSize();
}

void KMail::RecentAddresses::save( KConfig *config )
{
    KConfigGroupSaver cs( config, "General" );
    config->writeEntry( "Recent Addresses", addresses() );
}

void KMail::RecentAddresses::add( const QString& entry )
{
    if ( !entry.isEmpty() && m_maxCount > 0 ) {
        QString email;
        QString fullName;
        KABC::Addressee addr;

        KABC::Addressee::parseEmailAddress( entry, fullName, email );

        for ( KABC::Addressee::List::Iterator it = m_addresseeList.begin();
              it != m_addresseeList.end(); ++it )
        {
            if ( email == (*it).preferredEmail() )
                return;//already inside
        }
        addr.setNameFromString( fullName );
        addr.insertEmail( email, true );
        m_addresseeList.prepend( addr );
        adjustSize();
    }
}

void KMail::RecentAddresses::setMaxCount( int count )
{
    m_maxCount = count;
    adjustSize();
}

void KMail::RecentAddresses::adjustSize()
{
    while ( m_addresseeList.count() > m_maxCount )
        m_addresseeList.remove( m_addresseeList.fromLast() );
}

QStringList KMail::RecentAddresses::addresses() const
{
    QStringList addresses;
    for ( KABC::Addressee::List::ConstIterator it = m_addresseeList.begin();
          it != m_addresseeList.end(); ++it )
    {
        addresses.append( (*it).fullEmail() );
    }
    return addresses;
}
