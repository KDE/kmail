/*  -*- c++ -*-
    attachmentstrategy.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

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

#include "attachmentstrategy.h"

#include "partNode.h"
#include "kmmsgpart.h"

#include <QString>

#include <kdebug.h>


namespace KMail {


  //
  // IconicAttachmentStrategy:
  //   show everything but the first text/plain body as icons
  //

  class IconicAttachmentStrategy : public AttachmentStrategy {
    friend class ::KMail::AttachmentStrategy;
  protected:
    IconicAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~IconicAttachmentStrategy() {}
    
  public:
    const char * name() const { return "iconic"; }
    const AttachmentStrategy * next() const { return smart(); }
    const AttachmentStrategy * prev() const { return hidden(); }

    bool inlineNestedMessages() const { return false; }
    Display defaultDisplay( const partNode * ) const { return AsIcon; }
  };

  //
  // SmartAttachmentStrategy:
  //   in addition to Iconic, show all body parts
  //   with content-disposition == "inline" and
  //   all text parts without a filename or name parameter inline
  //

  class SmartAttachmentStrategy : public AttachmentStrategy {
    friend class ::KMail::AttachmentStrategy;
  protected:
    SmartAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~SmartAttachmentStrategy() {}
    
  public:
    const char * name() const { return "smart"; }
    const AttachmentStrategy * next() const { return inlined(); }
    const AttachmentStrategy * prev() const { return iconic(); }

    bool inlineNestedMessages() const { return true; }
    Display defaultDisplay( const partNode * node ) const {
      if ( node->hasContentDispositionInline() )
	// explict "inline" disposition:
	return Inline;
      if ( node->isAttachment() )
	// explicit "attachment" disposition:
	return AsIcon;
      if ( node->type() == DwMime::kTypeText &&
	   node->msgPart().fileName().trimmed().isEmpty() &&
	   node->msgPart().name().trimmed().isEmpty() )
	// text/* w/o filename parameter:
	return Inline;
      return AsIcon;
    }
  };

  //
  // InlinedAttachmentStrategy:
  //   show everything possible inline
  //

  class InlinedAttachmentStrategy : public AttachmentStrategy {
    friend class ::KMail::AttachmentStrategy;
  protected:
    InlinedAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~InlinedAttachmentStrategy() {}
    
  public:
    const char * name() const { return "inlined"; }
    const AttachmentStrategy * next() const { return hidden(); }
    const AttachmentStrategy * prev() const { return smart(); }

    bool inlineNestedMessages() const { return true; }
    Display defaultDisplay( const partNode * ) const { return Inline; }
  };

  //
  // HiddenAttachmentStrategy
  //   show nothing except the first text/plain body part _at all_
  //

  class HiddenAttachmentStrategy : public AttachmentStrategy {
    friend class ::KMail::AttachmentStrategy;
  protected:
    HiddenAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~HiddenAttachmentStrategy() {}
    
  public:
    const char * name() const { return "hidden"; }
    const AttachmentStrategy * next() const { return iconic(); }
    const AttachmentStrategy * prev() const { return inlined(); }

    bool inlineNestedMessages() const { return false; }
    Display defaultDisplay( const partNode * ) const { return None; }
  };


  //
  // AttachmentStrategy abstract base:
  //

  AttachmentStrategy::AttachmentStrategy() {

  }

  AttachmentStrategy::~AttachmentStrategy() {

  }

  const AttachmentStrategy * AttachmentStrategy::create( Type type ) {
    switch ( type ) {
    case Iconic:  return iconic();
    case Smart:   return smart();
    case Inlined: return inlined();
    case Hidden:  return hidden();
    }
    kFatal( 5006 ) <<"AttachmentStrategy::create(): Unknown attachment startegy ( type =="
		    << (int)type << ") requested!";
    return 0; // make compiler happy
  }

  const AttachmentStrategy * AttachmentStrategy::create( const QString & type ) {
    QString lowerType = type.toLower();
    if ( lowerType == "iconic" )  return iconic();
    //if ( lowerType == "smart" )   return smart(); // not needed, see below
    if ( lowerType == "inlined" ) return inlined();
    if ( lowerType == "hidden" )  return hidden();
    // don't kFatal here, b/c the strings are user-provided
    // (KConfig), so fail gracefully to the default:
    return smart();
  }

  static const AttachmentStrategy * iconicStrategy = 0;
  static const AttachmentStrategy * smartStrategy = 0;
  static const AttachmentStrategy * inlinedStrategy = 0;
  static const AttachmentStrategy * hiddenStrategy = 0;

  const AttachmentStrategy * AttachmentStrategy::iconic() {
    if ( !iconicStrategy )
      iconicStrategy = new IconicAttachmentStrategy();
    return iconicStrategy;
  }

  const AttachmentStrategy * AttachmentStrategy::smart() {
    if ( !smartStrategy )
      smartStrategy = new SmartAttachmentStrategy();
    return smartStrategy;
  }

  const AttachmentStrategy * AttachmentStrategy::inlined() {
    if ( !inlinedStrategy )
      inlinedStrategy = new InlinedAttachmentStrategy();
    return inlinedStrategy;
  }

  const AttachmentStrategy * AttachmentStrategy::hidden() {
    if ( !hiddenStrategy )
      hiddenStrategy = new HiddenAttachmentStrategy();
    return hiddenStrategy;
  }

} // namespace KMail
