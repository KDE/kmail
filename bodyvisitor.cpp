#include "bodyvisitor.h"
#include "kmmsgpart.h"
#include "attachmentstrategy.h"
#include <kdebug.h>

namespace KMail {

  BodyVisitor* BodyVisitorFactory::getVisitor( const AttachmentStrategy* strategy )
  {
    if (strategy == AttachmentStrategy::smart())
    {
      return new BodyVisitorSmart();
    } else if (strategy == AttachmentStrategy::iconic())
    {
      return new BodyVisitorHidden();
    } else if (strategy == AttachmentStrategy::inlined())
    {
      return new BodyVisitorInline();
    } else if (strategy == AttachmentStrategy::hidden())
    {
      return new BodyVisitorHidden();
    }
    // default
    return new BodyVisitorSmart();
  }

  //=============================================================================

  BodyVisitor::BodyVisitor()
  {
    // parts that are probably always ok to load
    mBasicList.clear();
    mBasicList += "TEXT/PLAIN";
    mBasicList += "TEXT/HTML";
    mBasicList += "APPLICATION/PGP-SIGNATURE";
  }

  //-----------------------------------------------------------------------------
  BodyVisitor::~BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  void BodyVisitor::visit( KMMessagePart * part )
  {
    mParts.append( part );
  }

  //-----------------------------------------------------------------------------
  void BodyVisitor::visit( QPtrList<KMMessagePart> & list )
  {
    mParts = list;
  }

  //=============================================================================

  BodyVisitorSmart::BodyVisitorSmart()
    : BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  QPtrList<KMMessagePart> BodyVisitorSmart::partsToLoad()
  {
    QPtrListIterator<KMMessagePart> it( mParts );
    QPtrList<KMMessagePart> selected;
    KMMessagePart * part;
    while ( (part = it.current()) != 0 )
    {
      ++it;
      // basic parts
      if ( mBasicList.contains( part->originalContentTypeStr() ) )
        selected.append( part );
      // header of an encapsulated message
      if ( part->partSpecifier().endsWith(".HEADER") )
        selected.append( part );
    }
    return selected;
  }

  //=============================================================================

  BodyVisitorInline::BodyVisitorInline()
    : BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  QPtrList<KMMessagePart> BodyVisitorInline::partsToLoad()
  {
    QPtrListIterator<KMMessagePart> it( mParts );
    QPtrList<KMMessagePart> selected;
    KMMessagePart * part;
    while ( (part = it.current()) != 0 )
    {
      ++it;
      // basics
      if ( mBasicList.contains( part->originalContentTypeStr() ) )
        selected.append( part );
      // header of an encapsulated message
      if ( part->partSpecifier().endsWith(".HEADER") )
        selected.append( part );
      // images
      if ( part->typeStr() == "IMAGE" )
        selected.append( part );
    }
    return selected;
  }

  //=============================================================================

  BodyVisitorHidden::BodyVisitorHidden()
    : BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  QPtrList<KMMessagePart> BodyVisitorHidden::partsToLoad()
  {
    QPtrListIterator<KMMessagePart> it( mParts );
    QPtrList<KMMessagePart> selected;
    KMMessagePart * part;
    while ( (part = it.current()) != 0 )
    {
      ++it;
      // basics
      if ( mBasicList.contains( part->originalContentTypeStr() ) )
        selected.append( part );
    }
    return selected;
  }

};
