#ifndef _SUBJECT
#define _SUBJECT

#include <qptrlist.h>

namespace KMail {

  class IObserver;

  class ISubject
  {
    public :
      virtual ~ISubject();

    public :
      /** Attach the observer if it is not in the list yet */ 
      virtual void attach( IObserver * );

      /** Detach the observer */ 
      virtual void detach( IObserver * );

      /** Notify all observers ( call observer->update() ) */ 
      virtual void notify();

    // Protected constructor and attribute(s)
    protected :
      ISubject();
      QPtrList<IObserver> mObserverList;
  };

}; // namespace KMail

#endif
