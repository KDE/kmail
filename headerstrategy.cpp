/*  -*- c++ -*-
    headerstrategy.cpp

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "headerstrategy.h"

#include "kmkernel.h"

#include <kdebug.h>
#include <kconfig.h>

namespace KMail {

  //
  // Header tables:
  //

  static const char * briefHeaders[] = {
    "subject", "from", "cc", "bcc", "date"
  };
  static const int numBriefHeaders = sizeof briefHeaders / sizeof *briefHeaders;


  static const char * standardHeaders[] = {
    "subject", "from", "cc", "bcc", "to"
  };
  static const int numStandardHeaders = sizeof standardHeaders / sizeof *standardHeaders;


  static const char * richHeaders[] = {
    "subject", "date", "from", "cc", "bcc", "to",
    "organization", "organisation", "reply-to", /* "status" ,*/ "statuspic"
  };
  static const int numRichHeaders = sizeof richHeaders / sizeof *richHeaders;

  //
  // Convenience function
  //

  static QStringList stringList( const char * headers[], int numHeaders ) {
    QStringList sl;
    for ( int i = 0 ; i < numHeaders ; ++i )
      sl.push_back( headers[i] );
    return sl;
  }

  //
  // AllHeaderStrategy:
  //   show everything
  //

  class AllHeaderStrategy : public HeaderStrategy {
    friend class HeaderStrategy;
  protected:
    AllHeaderStrategy() : HeaderStrategy() {}
    virtual ~AllHeaderStrategy() {}

  public:
    const char * name() const { return "all"; }
    const HeaderStrategy * next() const { return rich(); }
    const HeaderStrategy * prev() const { return custom(); }

    DefaultPolicy defaultPolicy() const { return Display; }

    bool showHeader( const QString & ) const {
      return true; // more efficient than default impl
    }
  };

  //
  // RichHeaderStrategy:
  //   Date, Subject, From, To, CC, ### what exactly?
  //

  class RichHeaderStrategy : public HeaderStrategy {
    friend class HeaderStrategy;
  protected:
    RichHeaderStrategy()
      : HeaderStrategy(),
	mHeadersToDisplay( stringList( richHeaders, numRichHeaders ) ) {}
    virtual ~RichHeaderStrategy() {}

  public:
    const char * name() const { return "rich"; }
    const HeaderStrategy * next() const { return standard(); }
    const HeaderStrategy * prev() const { return all(); }

    QStringList headersToDisplay() const { return mHeadersToDisplay; }
    DefaultPolicy defaultPolicy() const { return Hide; }

  private:
    const QStringList mHeadersToDisplay;
  };

  //
  // StandardHeaderStrategy:
  //   BCC, CC, Date, From, Subject, To
  //

  class StandardHeaderStrategy : public HeaderStrategy {
    friend class HeaderStrategy;
  protected:
    StandardHeaderStrategy()
      : HeaderStrategy(),
	mHeadersToDisplay( stringList( standardHeaders, numStandardHeaders) ) {}
    virtual ~StandardHeaderStrategy() {}

  public:
    const char * name() const { return "standard"; }
    const HeaderStrategy * next() const { return brief(); }
    const HeaderStrategy * prev() const { return rich(); }

    QStringList headersToDisplay() const { return mHeadersToDisplay; }
    DefaultPolicy defaultPolicy() const { return Hide; }

  private:
    const QStringList mHeadersToDisplay;
  };

  //
  // BriefHeaderStrategy
  //   From, Subject, Date
  //

  class BriefHeaderStrategy : public HeaderStrategy {
    friend class HeaderStrategy;
  protected:
    BriefHeaderStrategy()
      : HeaderStrategy(),
	mHeadersToDisplay( stringList( briefHeaders, numBriefHeaders ) ) {}
    virtual ~BriefHeaderStrategy() {}

  public:
    const char * name() const { return "brief"; }
    const HeaderStrategy * next() const { return custom(); }
    const HeaderStrategy * prev() const { return standard(); }

    QStringList headersToDisplay() const { return mHeadersToDisplay; }
    DefaultPolicy defaultPolicy() const { return Hide; }

  private:
    const QStringList mHeadersToDisplay;
  };


  //
  // CustomHeaderStrategy
  //   Determined by user
  //

  class CustomHeaderStrategy : public HeaderStrategy {
    friend class HeaderStrategy;
  protected:
    CustomHeaderStrategy();
    virtual ~CustomHeaderStrategy() {}

  public:
    const char * name() const { return "custom"; }
    const HeaderStrategy * next() const { return all(); }
    const HeaderStrategy * prev() const { return brief(); }

    QStringList headersToDisplay() const { return mHeadersToDisplay; }
    QStringList headersToHide() const { return mHeadersToHide; }
    DefaultPolicy defaultPolicy() const { return mDefaultPolicy; }

  private:
    QStringList mHeadersToDisplay;
    QStringList mHeadersToHide;
    DefaultPolicy mDefaultPolicy;
  };


  CustomHeaderStrategy::CustomHeaderStrategy()
    : HeaderStrategy()
  {
    KConfigGroup customHeader( KMKernel::config(), "Custom Headers" );
    if ( customHeader.hasKey( "headers to display" ) ) {
      mHeadersToDisplay = customHeader.readListEntry( "headers to display" );
      for ( QStringList::iterator it = mHeadersToDisplay.begin() ; it != mHeadersToDisplay.end() ; ++ it )
	*it = (*it).lower();
    } else
      mHeadersToDisplay = stringList( standardHeaders, numStandardHeaders );

    if ( customHeader.hasKey( "headers to hide" ) ) {
      mHeadersToHide = customHeader.readListEntry( "headers to hide" );
      for ( QStringList::iterator it = mHeadersToHide.begin() ; it != mHeadersToHide.end() ; ++ it )
	*it = (*it).lower();
    }

    mDefaultPolicy = customHeader.readEntry( "default policy", "hide" ) == "display" ? Display : Hide ;
  }

  //
  // HeaderStrategy abstract base:
  //

  HeaderStrategy::HeaderStrategy() {

  }

  HeaderStrategy::~HeaderStrategy() {

  }

  QStringList HeaderStrategy::headersToDisplay() const {
    return QStringList();
  }

  QStringList HeaderStrategy::headersToHide() const {
    return QStringList();
  }

  bool HeaderStrategy::showHeader( const QString & header ) const {
    if ( headersToDisplay().contains( header.lower() ) ) return true;
    if ( headersToHide().contains( header.lower() ) ) return false;
    return defaultPolicy() == Display;
  }

  const HeaderStrategy * HeaderStrategy::create( Type type ) {
    switch ( type ) {
    case All:  return all();
    case Rich:   return rich();
    case Standard: return standard();
    case Brief:  return brief();
    case Custom:  return custom();
    }
    kdFatal( 5006 ) << "HeaderStrategy::create(): Unknown header strategy ( type == "
		    << (int)type << " ) requested!" << endl;
    return 0; // make compiler happy
  }

  const HeaderStrategy * HeaderStrategy::create( const QString & type ) {
    QString lowerType = type.lower();
    if ( lowerType == "all" )  return all();
    if ( lowerType == "rich" )   return HeaderStrategy::rich();
    //if ( lowerType == "standard" ) return standard(); // not needed, see below
    if ( lowerType == "brief" ) return brief();
    if ( lowerType == "custom" )  return custom();
    // don't kdFatal here, b/c the strings are user-provided
    // (KConfig), so fail gracefully to the default:
    return standard();
  }

  static const HeaderStrategy * allStrategy = 0;
  static const HeaderStrategy * richStrategy = 0;
  static const HeaderStrategy * standardStrategy = 0;
  static const HeaderStrategy * briefStrategy = 0;
  static const HeaderStrategy * customStrategy = 0;

  const HeaderStrategy * HeaderStrategy::all() {
    if ( !allStrategy )
      allStrategy = new AllHeaderStrategy();
    return allStrategy;
  }

  const HeaderStrategy * HeaderStrategy::rich() {
    if ( !richStrategy )
      richStrategy = new RichHeaderStrategy();
    return richStrategy;
  }

  const HeaderStrategy * HeaderStrategy::standard() {
    if ( !standardStrategy )
      standardStrategy = new StandardHeaderStrategy();
    return standardStrategy;
  }

  const HeaderStrategy * HeaderStrategy::brief() {
    if ( !briefStrategy )
      briefStrategy = new BriefHeaderStrategy();
    return briefStrategy;
  }

  const HeaderStrategy * HeaderStrategy::custom() {
    if ( !customStrategy )
      customStrategy = new CustomHeaderStrategy();
    return customStrategy;
  }

} // namespace KMail
