#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "isubject.h"
#include "interfaces/observer.h"

#include <qtl.h>

#include <kdebug.h>

namespace KMail {

  ISubject::~ISubject()
  {
    mObserverList.clear();
  }

  void ISubject::attach( Interface::Observer * pObserver )
  {
    if ( qFind( mObserverList.begin(), mObserverList.end(), pObserver ) == mObserverList.end() )
      mObserverList.push_back( pObserver );
  }

  void ISubject::detach( Interface::Observer * pObserver ) {
    QValueVector<Interface::Observer*>::iterator it = qFind( mObserverList.begin(), mObserverList.end(), pObserver );
    if ( it != mObserverList.end() )
      mObserverList.erase( it );
  }

  void ISubject::notify()
  {
    kdDebug(5006) << "ISubject::notify " << mObserverList.size() << endl;

    // iterate over a copy (to prevent crashes when
    // {attach(),detach()} is called from an Observer::update()
    const QValueVector<Interface::Observer*> copy = mObserverList;
    for ( QValueVector<Interface::Observer*>::const_iterator it = copy.begin() ; it != copy.end() ; ++it ) {
      if ( (*it) ) {
        (*it)->update( this );
      }
    }
  }

}

