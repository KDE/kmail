#ifndef _OBSERVER
#define _OBSERVER

namespace KMail {

  class ISubject;

  class IObserver
  {
    // Destructor
    public :
      virtual ~IObserver();

      // Methods
    public :
      virtual bool update( ISubject * ) = 0;

      // Protected constructor
    protected :
      IObserver();
  };

};

#endif
