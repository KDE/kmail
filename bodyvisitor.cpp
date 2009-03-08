/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2003 Carsten Burghardt <burghardt@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#include "bodyvisitor.h"
#include "kmmsgpart.h"
#include "attachmentstrategy.h"
#include <kdebug.h>

using namespace KMail;

BodyVisitor *BodyVisitorFactory::getVisitor( const AttachmentStrategy *strategy )
{
  if ( strategy == AttachmentStrategy::smart() ) {
    return new BodyVisitorSmart();
  } else if ( strategy == AttachmentStrategy::iconic() ) {
    return new BodyVisitorHidden();
  } else if ( strategy == AttachmentStrategy::inlined() ) {
    return new BodyVisitorInline();
  } else if ( strategy == AttachmentStrategy::hidden()) {
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
  mBasicList += "TEXT/CALENDAR";
  mBasicList += "TEXT/X-VCARD";
}

//-----------------------------------------------------------------------------
BodyVisitor::~BodyVisitor()
{
}

//-----------------------------------------------------------------------------
void BodyVisitor::visit( KMMessagePart *part )
{
  mParts.append( part );
}

//-----------------------------------------------------------------------------
void BodyVisitor::visit( QList<KMMessagePart*> &list )
{
  mParts = list;
}

//-----------------------------------------------------------------------------
QList<KMMessagePart*> BodyVisitor::partsToLoad()
{
  QList<KMMessagePart*>::const_iterator it;
  QList<KMMessagePart*> selected;
  bool headerCheck = false;
  for ( it = mParts.constBegin(); it != mParts.constEnd(); ++it ) {
    KMMessagePart *part = (*it);
    // skip this part if the parent part is already loading
    if ( part->parent() &&
         selected.contains( part->parent() ) &&
         part->loadPart() ) {
      continue;
    }

    if ( part->originalContentTypeStr().contains("SIGNED") ) {
      // signed messages have to be loaded completely
      // so construct a new dummy part that loads the body
      KMMessagePart *fake = new KMMessagePart();
      fake->setPartSpecifier( "TEXT" );
      fake->setOriginalContentTypeStr("");
      fake->setLoadPart( true );
      selected.append( fake );
      break;
    }

    if ( headerCheck && !part->partSpecifier().endsWith(QLatin1String(".HEADER")) ) {
      // this is an embedded simple message (not multipart) so we get
      // no header part from imap. As we probably need to load the header
      // (e.g. in smart or inline mode) we add a fake part that is not
      // included in the message itself
      KMMessagePart *fake = new KMMessagePart();
      QString partId = part->partSpecifier().section( '.', 0, -2 ) + ".HEADER";
      fake->setPartSpecifier( partId );
      fake->setOriginalContentTypeStr("");
      fake->setLoadPart( true );
      if ( addPartToList( fake ) ) {
        selected.append( fake );
      }
    }

    if ( part->originalContentTypeStr() == "MESSAGE/RFC822" ) {
      headerCheck = true;
    } else {
      headerCheck = false;
    }

    // check whether to load this part or not:
    // look at the basic list, ask the subclass and check the parent
    if ( mBasicList.contains( part->originalContentTypeStr() ) ||
         parentNeedsLoading( part ) ||
         addPartToList( part ) ) {
      if ( part->typeStr() != "MULTIPART" ||
           part->partSpecifier().endsWith(QLatin1String(".HEADER")) ) {
        // load the part itself
        part->setLoadPart( true );
      }
    }
    if ( !part->partSpecifier().endsWith(QLatin1String(".HEADER")) &&
         part->typeStr() != "MULTIPART" ) {
      part->setLoadHeaders( true ); // load MIME header
    }

    if ( part->loadHeaders() || part->loadPart() ) {
      selected.append( part );
    }
  }
  return selected;
}

//-----------------------------------------------------------------------------
bool BodyVisitor::parentNeedsLoading( KMMessagePart *msgPart )
{
  KMMessagePart *part = msgPart;
  while ( part ) {
    if ( part->parent() &&
         ( part->parent()->originalContentTypeStr() == "MULTIPART/SIGNED" ||
           ( msgPart->originalContentTypeStr() == "APPLICATION/OCTET-STREAM" &&
             part->parent()->originalContentTypeStr() == "MULTIPART/ENCRYPTED" ) ) ) {
      return true;
    }

    part = part->parent();
  }
  return false;
}

//=============================================================================

BodyVisitorSmart::BodyVisitorSmart() : BodyVisitor()
{
}

//-----------------------------------------------------------------------------
bool BodyVisitorSmart::addPartToList( KMMessagePart *part )
{
  // header of an encapsulated message
  if ( part->partSpecifier().endsWith(QLatin1String(".HEADER")) ) {
    return true;
  }

  return false;
}

//=============================================================================

BodyVisitorInline::BodyVisitorInline() : BodyVisitor()
{
}

//-----------------------------------------------------------------------------
bool BodyVisitorInline::addPartToList( KMMessagePart *part )
{
  if ( part->partSpecifier().endsWith(QLatin1String(".HEADER")) ) {
    // header of an encapsulated message
    return true;
  } else if ( part->typeStr() == "IMAGE" ) {
    // images
    return true;
  } else if ( part->typeStr() == "TEXT" ) {
    // text, diff and stuff
    return true;
  }

  return false;
}

//=============================================================================

BodyVisitorHidden::BodyVisitorHidden() : BodyVisitor()
{
}

//-----------------------------------------------------------------------------
bool BodyVisitorHidden::addPartToList( KMMessagePart *part )
{
  // header of an encapsulated message
  if ( part->partSpecifier().endsWith(QLatin1String(".HEADER")) ) {
    return true;
  }

  return false;
}
