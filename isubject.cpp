#include "isubject.h"
#include "iobserver.h"
#include <kdebug.h>

namespace KMail {

  ISubject::ISubject()
  {
  }

  ISubject::~ISubject()
  {
    mObserverList.clear();
  }

  void ISubject::attach( IObserver * pObserver )
  {
    if (mObserverList.find( pObserver ) == -1)
      mObserverList.append( pObserver );
  }

  void ISubject::detach( IObserver * pObserver )
  {
    mObserverList.remove( pObserver );
  }

  void ISubject::notify()
  {
    kdDebug(5006) << "ISubject::notify " << mObserverList.count() << endl;
    QPtrListIterator<IObserver> it( mObserverList );
    IObserver* observer;
    while ( (observer = it.current()) != 0 ) 
    {
      ++it;
      bool success = observer->update( this );
      if ( !success )
        kdWarning(5006) << "ISubject::notify returned false for observer " << observer << endl;
    }
  }

};

