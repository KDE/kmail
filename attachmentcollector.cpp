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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <messageviewer/nodehelper.h>

#include <kdebug.h>
#include <kmime/kmime_content.h>

static bool isInSkipList( KMime::Content * ) {
  return false;
}

static bool isInExclusionList( KMime::Content * node ) {
  if ( !node )
    return true;

  if ( node->contentType()->mediaType() == "application" ) {
    QString subType = node->contentType()->subType();
    if ( subType == "pkcs7-mime"  || subType == "pkcs7-signature" || subType == "pgp-signature" || subType == "pgp-encrypted" ) {
      return true;
    }
  }
  if ( node->contentType()->isMultipart()) {
    return true;
  }
  return false;
}


void KMail::AttachmentCollector::collectAttachmentsFrom( KMime::Content * node )
{
  KMime::Content *parent;

  while ( node ) {
    parent = node->parent();

    if ( node->topLevel()->textContent() == node ) {
      node = MessageViewer::NodeHelper::next( node );
      continue;
    }

    if ( isInSkipList( node ) ) {
      node = MessageViewer::NodeHelper::next( node, false ); // skip even the children
      continue;
    }

    if ( isInExclusionList( node ) ) {
      node = MessageViewer::NodeHelper::next( node );
      continue;
    }

    if ( parent && parent->contentType()->isMultipart() &&
         parent->contentType()->subType() == "related" ) {
      node = MessageViewer::NodeHelper::next( node );  // skip embedded images
      continue;
    }

    if ( MessageViewer::NodeHelper::isHeuristicalAttachment( node ) ) {
      mAttachments.push_back( node );
      node = MessageViewer::NodeHelper::next( node, false ); // just make double sure
      continue;
    }

    node = MessageViewer::NodeHelper::next( node );
  }
}

