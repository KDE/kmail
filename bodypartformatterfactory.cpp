/*  -*- mode: C++; c-file-style: "gnu" -*-
    bodypartformatterfactory.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>,
                       Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

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

#include "bodypartformatterfactory.h"
#include "bodypartformatterfactory_p.h"
using namespace KMail::BodyPartFormatterFactoryPrivate;

#include "interfaces/bodypartformatter.h"
#include "urlhandlermanager.h"

// libkdepim
#include <libkdepim/pluginloader.h>

// KDE
#include <kdebug.h>

// Qt
#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>

#include <assert.h>

namespace {

  KPIM_DEFINE_PLUGIN_LOADER( BodyPartFormatterPluginLoader,
			     KMail::Interface::BodyPartFormatterPlugin,
			     "create_bodypart_formatter_plugin",
			     "kmail/plugins/bodypartformatter/*.desktop" )

}

KMail::BodyPartFormatterFactory * KMail::BodyPartFormatterFactory::mSelf = 0;

const KMail::BodyPartFormatterFactory * KMail::BodyPartFormatterFactory::instance() {
  if ( !mSelf )
    mSelf = new BodyPartFormatterFactory();
  return mSelf;
}

KMail::BodyPartFormatterFactory::BodyPartFormatterFactory() {
  mSelf = this;
}

KMail::BodyPartFormatterFactory::~BodyPartFormatterFactory() {
  mSelf = 0;
}

static TypeRegistry * all = 0;

static void insertBodyPartFormatter( const char * type, const char * subtype,
				     const KMail::Interface::BodyPartFormatter * formatter ) {
  if ( !type || !*type || !subtype || !*subtype || !formatter || !all )
    return;

  TypeRegistry::iterator type_it = all->find( type );
  if ( type_it == all->end() ) {
    kdDebug( 5006 ) << "BodyPartFormatterFactory: instantiating new Subtype Registry for \""
		    << type << "\"" << endl;
    type_it = all->insert( std::make_pair( type, SubtypeRegistry() ) ).first;
    assert( type_it != all->end() );
  }

  SubtypeRegistry & subtype_reg = type_it->second;
  SubtypeRegistry::iterator subtype_it = subtype_reg.find( subtype );
  if ( subtype_it != subtype_reg.end() ) {
    kdDebug( 5006 ) << "BodyPartFormatterFactory: overwriting previously registered formatter for \""
		    << type << "/" << subtype << "\"" << endl;
    subtype_reg.erase( subtype_it ); subtype_it = subtype_reg.end();
  }

  subtype_reg.insert( std::make_pair( subtype, formatter ) );
}

static void loadPlugins() {
  const BodyPartFormatterPluginLoader * pl = BodyPartFormatterPluginLoader::instance();
  if ( !pl ) {
    kdWarning( 5006 ) << "BodyPartFormatterFactory: cannot instantiate plugin loader!" << endl;
    return;
  }
  const QStringList types = pl->types();
  kdDebug( 5006 ) << "BodyPartFormatterFactory: found " << types.size() << " plugins." << endl;
  for ( QStringList::const_iterator it = types.begin() ; it != types.end() ; ++it ) {
    const KMail::Interface::BodyPartFormatterPlugin * plugin = pl->createForName( *it );
    if ( !plugin ) {
      kdWarning( 5006 ) << "BodyPartFormatterFactory: plugin \"" << *it << "\" is not valid!" << endl;
      continue;
    }
    for ( int i = 0 ; const KMail::Interface::BodyPartFormatter * bfp = plugin->bodyPartFormatter( i ) ; ++i ) {
      const char * type = plugin->type( i );
      if ( !type || !*type ) {
	kdWarning( 5006 ) << "BodyPartFormatterFactory: plugin \"" << *it
			  << "\" returned empty type specification for index "
			  << i << endl;
	break;
      }
      const char * subtype = plugin->subtype( i );
      if ( !subtype || !*subtype ) {
	kdWarning( 5006 ) << "BodyPartFormatterFactory: plugin \"" << *it
			  << "\" returned empty subtype specification for index "
			  << i << endl;
	break;
      }
      insertBodyPartFormatter( type, subtype, bfp );
    }
    for ( int i = 0 ; const KMail::Interface::BodyPartURLHandler * handler = plugin->urlHandler( i ) ; ++i )
      KMail::URLHandlerManager::instance()->registerHandler( handler );
  }
}

static void setup() {
  if ( !all ) {
    all = new TypeRegistry();
    kmail_create_builtin_bodypart_formatters( all );
    loadPlugins();
  }
}


const KMail::Interface::BodyPartFormatter * KMail::BodyPartFormatterFactory::createFor( const char * type, const char * subtype ) const {
  if ( !type || !*type )
    type = "*";
  if ( !subtype || !*subtype )
    subtype = "*";

  setup();
  assert( all );

  if ( all->empty() )
    return 0;

  TypeRegistry::const_iterator type_it = all->find( type );
  if ( type_it == all->end() )
    type_it = all->find( "*" );
  if ( type_it == all->end() )
    return 0;

  const SubtypeRegistry & subtype_reg = type_it->second;
  if ( subtype_reg.empty() )
    return 0;

  SubtypeRegistry::const_iterator subtype_it = subtype_reg.find( subtype );
  if ( subtype_it == subtype_reg.end() )
    subtype_it = subtype_reg.find( "*" );
  if ( subtype_it == subtype_reg.end() )
    return 0;

  kdWarning( !(*subtype_it).second, 5006 )
    << "BodyPartFormatterFactory: a null bodypart formatter sneaked in for \""
    << type << "/" << subtype << "\"!" << endl;

  return (*subtype_it).second;
}

const KMail::Interface::BodyPartFormatter * KMail::BodyPartFormatterFactory::createFor( const QString & type, const QString & subtype ) const {
  return createFor( type.latin1(), subtype.latin1() );
}

const KMail::Interface::BodyPartFormatter * KMail::BodyPartFormatterFactory::createFor( const QCString & type, const QCString & subtype ) const {
  return createFor( type.data(), subtype.data() );
}
