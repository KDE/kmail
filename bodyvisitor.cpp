#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
    // body text
    mBasicList += "TEXT/PLAIN";
    mBasicList += "TEXT/HTML";
    mBasicList += "MESSAGE/DELIVERY-STATUS";
    // pgp stuff
    mBasicList += "APPLICATION/PGP-SIGNATURE";
    mBasicList += "APPLICATION/PGP";
    mBasicList += "APPLICATION/PGP-ENCRYPTED";
    mBasicList += "APPLICATION/PKCS7-SIGNATURE";
    // groupware
    mBasicList += "APPLICATION/MS-TNEF";
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

  //-----------------------------------------------------------------------------
  QPtrList<KMMessagePart> BodyVisitor::partsToLoad()
  {
    QPtrListIterator<KMMessagePart> it( mParts );
    QPtrList<KMMessagePart> selected;
    KMMessagePart *part = 0;
    bool headerCheck = false;
    while ( (part = it.current()) != 0 )
    {
      ++it;
      // skip this part if the parent part is already loading
      if ( part->parent() && 
           selected.contains( part->parent() ) &&
           part->loadPart() )
        continue;

      if ( part->originalContentTypeStr().contains("SIGNED") )
      {
        // signed messages have to be loaded completely
        // so construct a new dummy part that loads the body
        KMMessagePart *fake = new KMMessagePart();
        fake->setPartSpecifier( "TEXT" );
        fake->setOriginalContentTypeStr("");
        fake->setLoadPart( true );
        selected.append( fake );
        break;
      }
        
      if ( headerCheck && !part->partSpecifier().endsWith(".HEADER") )
      {
        // this is an embedded simple message (not multipart) so we get no header part
        // from imap. As we probably need to load the header (e.g. in smart or inline mode)
        // we add a fake part that is not included in the message itself
        KMMessagePart *fake = new KMMessagePart();
        QString partId = part->partSpecifier().section( '.', 0, -2 )+".HEADER";
        fake->setPartSpecifier( partId );
        fake->setOriginalContentTypeStr("");
        fake->setLoadPart( true );
        if ( addPartToList( fake ) ) 
          selected.append( fake );
      }
      
      if ( part->originalContentTypeStr() == "MESSAGE/RFC822" )
        headerCheck = true;
      else
        headerCheck = false;
      
      // check whether to load this part or not:
      // look at the basic list, ask the subclass and check the parent
      if ( mBasicList.contains( part->originalContentTypeStr() ) ||
           parentNeedsLoading( part ) ||
           addPartToList( part ) )
      {
        if ( part->typeStr() != "MULTIPART" ||
             part->partSpecifier().endsWith(".HEADER") )
        {
          // load the part itself
          part->setLoadPart( true );
        }
      }
      if ( !part->partSpecifier().endsWith(".HEADER") &&
           part->typeStr() != "MULTIPART" )
        part->setLoadHeaders( true ); // load MIME header
      
      if ( part->loadHeaders() || part->loadPart() )
        selected.append( part );
    }
    return selected;
  }

  //-----------------------------------------------------------------------------
  bool BodyVisitor::parentNeedsLoading( KMMessagePart *msgPart )
  {
    KMMessagePart *part = msgPart;
    while ( part )
    {
      if ( part->parent() &&
           ( part->parent()->originalContentTypeStr() == "MULTIPART/SIGNED" ||
           ( msgPart->originalContentTypeStr() == "APPLICATION/OCTET-STREAM" &&
             part->parent()->originalContentTypeStr() == "MULTIPART/ENCRYPTED" ) ) )
        return true;

      part = part->parent();
    }
    return false;
  }

  //=============================================================================

  BodyVisitorSmart::BodyVisitorSmart()
    : BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  bool BodyVisitorSmart::addPartToList( KMMessagePart * part )
  {
    // header of an encapsulated message
    if ( part->partSpecifier().endsWith(".HEADER") )
      return true;

    return false;
  }

  //=============================================================================

  BodyVisitorInline::BodyVisitorInline()
    : BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  bool BodyVisitorInline::addPartToList( KMMessagePart * part )
  {
    // header of an encapsulated message
    if ( part->partSpecifier().endsWith(".HEADER") )
      return true;
    else if ( part->typeStr() == "IMAGE" ) // images
      return true;
    else if ( part->typeStr() == "TEXT" ) // text, diff and stuff
      return true;

    return false;
  }

  //=============================================================================

  BodyVisitorHidden::BodyVisitorHidden()
    : BodyVisitor()
  {
  }

  //-----------------------------------------------------------------------------
  bool BodyVisitorHidden::addPartToList( KMMessagePart * part )
  {
    // header of an encapsulated message
    if ( part->partSpecifier().endsWith(".HEADER") )
      return true;

    return false;
  }
  
}
