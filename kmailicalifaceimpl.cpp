#include "kmailicalifaceimpl.h"

#include <kdebug.h>

#include "kmgroupware.h"

KMailICalIfaceImpl::KMailICalIfaceImpl( KMGroupware* gw )
  : QObject( gw, "KMailICalIfaceImpl"),
    DCOPObject( "KMailICalIface" ),
    mGroupware(gw)
{
  connect( gw, SIGNAL( incidenceAdded( const QString&, const QString& ) ),
	   this, SLOT( slotIncidenceAdded( const QString&, const QString& ) ) );

  connect( gw, SIGNAL( incidenceDeleted( const QString&, const QString& ) ),
	   this, SLOT( slotIncidenceDeleted( const QString&, const QString& ) ) );
}

bool KMailICalIfaceImpl::addIncidence( const QString& folder, 
				       const QString& uid, 
				       const QString& ical )
{
  kdDebug() << "KMailICalIfaceImpl::addIncidence( " << folder << ", "
	    << uid << ", " << ical << " )" << endl;
  return mGroupware->addIncidence( folder, uid, ical );
}

bool KMailICalIfaceImpl::deleteIncidence( const QString& folder, 
					  const QString& uid )
{
  kdDebug() << "KMailICalIfaceImpl::deleteIncidence( " << folder << ", "
	    << uid << " )" << endl;
  return mGroupware->deleteIncidence( folder, uid );
}

QStringList KMailICalIfaceImpl::incidences( const QString& folder )
{
  kdDebug() << "KMailICalIfaceImpl::incidences( " << folder << " )" << endl;
  return mGroupware->incidences( folder );
}

void KMailICalIfaceImpl::slotIncidenceAdded( const QString& folder, const QString& ical )
{
  QByteArray data;
  QDataStream arg(data, IO_WriteOnly );
  arg << folder << ical;
  emitDCOPSignal( "incidenceAdded(QString,QString)", data );
}

void KMailICalIfaceImpl::slotIncidenceDeleted( const QString& folder, const QString& uid )
{
  QByteArray data;
  QDataStream arg(data, IO_WriteOnly );
  arg << folder << uid;
  emitDCOPSignal( "incidenceDeleted(QString,QString)", data );
}
