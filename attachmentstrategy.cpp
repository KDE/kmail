/*  -*- c++ -*-
    attachmentstrategy.cpp

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "attachmentstrategy.h"

#include <kdebug.h>

#include <qstring.h>

namespace KMail {


  //
  // IconicAttachmentStrategy:
  //   show everything but the first text/plain body as icons
  //

  class IconicAttachmentStrategy : public AttachmentStrategy {
    friend class AttachmentStrategy;
  protected:
    IconicAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~IconicAttachmentStrategy() {}
    
  public:
    const char * name() const { return "iconic"; }
    const AttachmentStrategy * next() const { return smart(); }
    const AttachmentStrategy * prev() const { return hidden(); }
  };

  //
  // SmartAttachmentStrategy:
  //   in addition to Iconic, show all body parts
  //   with content-disposition == "inline" inline
  //

  class SmartAttachmentStrategy : public AttachmentStrategy {
    friend class AttachmentStrategy;
  protected:
    SmartAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~SmartAttachmentStrategy() {}
    
  public:
    const char * name() const { return "smart"; }
    const AttachmentStrategy * next() const { return inlined(); }
    const AttachmentStrategy * prev() const { return iconic(); }
  };

  //
  // InlinedAttachmentStrategy:
  //   show everything possible inline
  //

  class InlinedAttachmentStrategy : public AttachmentStrategy {
    friend class AttachmentStrategy;
  protected:
    InlinedAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~InlinedAttachmentStrategy() {}
    
  public:
    const char * name() const { return "inlined"; }
    const AttachmentStrategy * next() const { return hidden(); }
    const AttachmentStrategy * prev() const { return smart(); }
  };

  //
  // HiddenAttachmentStrategy
  //   show nothing except the first text/plain body part _at all_
  //

  class HiddenAttachmentStrategy : public AttachmentStrategy {
    friend class AttachmentStrategy;
  protected:
    HiddenAttachmentStrategy() : AttachmentStrategy() {}
    virtual ~HiddenAttachmentStrategy() {}
    
  public:
    const char * name() const { return "hidden"; }
    const AttachmentStrategy * next() const { return iconic(); }
    const AttachmentStrategy * prev() const { return inlined(); }
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
    kdFatal( 5006 ) << "AttachmentStrategy::create(): Unknown attachment startegy ( type == "
		    << (int)type << " ) requested!" << endl;
    return 0; // make compiler happy
  }

  const AttachmentStrategy * AttachmentStrategy::create( const QString & type ) {
    QString lowerType = type.lower();
    if ( lowerType == "iconic" )  return iconic();
    //if ( lowerType == "smart" )   return smart(); // not needed, see below
    if ( lowerType == "inlined" ) return inlined();
    if ( lowerType == "hidden" )  return hidden();
    // don't kdFatal here, b/c the strings are user-provided
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

}; // namespace KMail
