#ifndef bodyiterator_h
#define bodyiterator_h

#include <qptrlist.h>
#include <qstringlist.h>

class KMMessagePart;

namespace KMail {

  class AttachmentStrategy;

  class BodyVisitor
  {
    public:
      BodyVisitor();
      virtual ~BodyVisitor();

      void visit( KMMessagePart * part );
      void visit( QPtrList<KMMessagePart> & list );

      virtual QPtrList<KMMessagePart> partsToLoad() = 0;

    protected:
      QPtrList<KMMessagePart> mParts;
      QStringList mBasicList;
  };

  class BodyVisitorFactory
  {
    public:
      static BodyVisitor* getVisitor( const AttachmentStrategy* strategy );
  };

  class BodyVisitorSmart: public BodyVisitor
  {
    public:
      BodyVisitorSmart();

      QPtrList<KMMessagePart> partsToLoad();
  };
  
  class BodyVisitorInline: public BodyVisitor
  {
    public:
      BodyVisitorInline();

      QPtrList<KMMessagePart> partsToLoad();
  };
  
  class BodyVisitorHidden: public BodyVisitor
  {
    public:
      BodyVisitorHidden();

      QPtrList<KMMessagePart> partsToLoad();
  };
  
};

#endif
