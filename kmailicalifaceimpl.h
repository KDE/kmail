#ifndef KMAILICALIFACEIMPL_H
#define KMAILICALIFACEIMPL_H

#include "kmailicalIface.h"

class KMGroupware;

class KMailICalIfaceImpl : public QObject, virtual public KMailICalIface {
  Q_OBJECT
public:
  KMailICalIfaceImpl( KMGroupware* gw );
  
  virtual bool addIncidence( const QString& folder, const QString& uid, 
			     const QString& ical );
  virtual bool deleteIncidence( const QString& folder, const QString& uid );
  virtual QStringList incidences( const QString& folder );
public slots:
  void slotIncidenceAdded( const QString& folder, const QString& ical );
  void slotIncidenceDeleted( const QString& folder, const QString& uid );
private:
  KMGroupware* mGroupware;
};

#endif // KMAILICALIFACEIMPL_H
