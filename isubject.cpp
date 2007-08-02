
#include "isubject.h"
#include "interfaces/observer.h"

#include <q3tl.h>

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
    kDebug(5006) <<"ISubject::notify" << mObserverList.size();
    for ( QVector<Interface::Observer*>::iterator it = mObserverList.begin() ; it != mObserverList.end() ; ++it )
      (*it)->update( this );
  }

}

