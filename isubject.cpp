
#include "isubject.h"
#include "interfaces/observer.h"

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
    QVector<Interface::Observer*>::iterator it = qFind( mObserverList.begin(), mObserverList.end(), pObserver );
    if ( it != mObserverList.end() )
      mObserverList.erase( it );
  }

  void ISubject::notify()
  {
    kDebug(5006) << mObserverList.size();

    // iterate over a copy (to prevent crashes when
    // {attach(),detach()} is called from an Observer::update()
    const QVector<Interface::Observer*> copy = mObserverList;
    for ( QVector<Interface::Observer*>::const_iterator it = copy.begin() ; it != copy.end() ; ++it ) {
      if ( (*it) ) {
        (*it)->update( this );
      }
    }
  }

}

