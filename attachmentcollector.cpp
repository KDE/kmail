/*  -*- c++ -*-
    attachmentcollector.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "attachmentcollector.h"

#include "partNode.h"

static partNode * next( partNode * n, bool allowChildren=true ) {
  if ( !n )
    return 0;
  if ( allowChildren )
    if ( partNode * c = n->firstChild() )
      return c;
  if ( partNode * s = n->nextSibling() )
    return s;
  for ( partNode * p = n->parentNode() ; p ; p = p->parentNode() )
    if ( partNode * s = p->nextSibling() )
      return s;
  return 0;
}

static inline partNode * nextNonChild( partNode * node ) {
  return next( node, false );
}

static bool isInSkipList( partNode * ) {
  return false;
}

static bool isInExclusionList( const partNode * node ) {
  if ( !node )
    return true;

  switch ( node->type() ) {
  case DwMime::kTypeApplication:
    switch ( node->subType() ) {
    case DwMime::kSubtypePkcs7Mime:
    case DwMime::kSubtypePkcs7Signature:
    case DwMime::kSubtypePgpSignature:
    case DwMime::kSubtypePgpEncrypted:
      return true;
    }
    break;
  case DwMime::kTypeMultipart:
    return true;
  }
  return false;
}


void KMail::AttachmentCollector::collectAttachmentsFrom( partNode * node ) {
  while ( node ) {
    if ( isInSkipList( node ) ) {
      node = nextNonChild( node ); // skip even the children
      continue;
    }
    if ( isInExclusionList( node ) ) {
      node = next( node );
      continue;
    }

    if ( node->isHeuristicalAttachment() ) {
      mAttachments.push_back( node );
      node = nextNonChild( node ); // just make double sure
      continue;
    }

    node = next( node );
  }
}

