/*  -*- c++ -*-
    This file is part of libkdepim.

    Copyright (c) 2002,2004 Marc Mutz <mutz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <pluginloaderbase.h>

#include <ksimpleconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <klibloader.h>
#include <kglobal.h>
#include <kdebug.h>

#include <qfile.h>
#include <qstringlist.h>

static kdbgstream warning() {
  return kdWarning( 5300 ) << "PluginLoaderBase: ";
}
#ifndef NDEBUG
static kdbgstream debug( bool cond )
#else
static kndbgstream debug( bool cond )
#endif
{
  return kdDebug( cond, 5300 ) << "PluginLoaderBase: ";
}

namespace KPIM {

  PluginLoaderBase::PluginLoaderBase() : d(0) {}
  PluginLoaderBase::~PluginLoaderBase() {}


  QStringList PluginLoaderBase::types() const {
    QStringList result;
    for ( QMap< QString, PluginMetaData >::const_iterator it = mPluginMap.begin();
	  it != mPluginMap.end() ; ++it )
      result.push_back( it.key() );
    return result;
  }

  const PluginMetaData * PluginLoaderBase::infoForName( const QString & type ) const {
    return mPluginMap.contains( type ) ? &(mPluginMap[type]) : 0 ;
  }


  void PluginLoaderBase::doScan( const char * path ) {
    mPluginMap.clear();

    const QStringList list =
      KGlobal::dirs()->findAllResources( "data", path, true, true );
    for ( QStringList::const_iterator it = list.begin() ;
	  it != list.end() ; ++it ) {
      KSimpleConfig config( *it, true );
      if ( config.hasGroup( "Misc" ) && config.hasGroup( "Plugin" ) ) {
	config.setGroup( "Plugin" );

	const QString type = config.readEntry( "Type" ).lower();
	if ( type.isEmpty() ) {
	  warning() << "missing or empty [Plugin]Type value in \""
		    << *it << "\" - skipping" << endl;
	  continue;
	}

	const QString library = config.readEntry( "X-KDE-Library" );
	if ( library.isEmpty() ) {
	  warning() << "missing or empty [Plugin]X-KDE-Library value in \""
		    << *it << "\" - skipping" << endl;
	  continue;
	}

	config.setGroup( "Misc" );

	QString name = config.readEntry( "Name" );
	if ( name.isEmpty() ) {
	  warning() << "missing or empty [Misc]Name value in \""
		    << *it << "\" - inserting default name" << endl;
	  name = i18n("Unnamed plugin");
	}

	QString comment = config.readEntry( "Comment" );
	if ( comment.isEmpty() ) {
	  warning() << "missing or empty [Misc]Comment value in \""
		    << *it << "\" - inserting default name" << endl;
	  comment = i18n("No description available");
	}

	mPluginMap.insert( type, PluginMetaData( library, name, comment ) );
      } else {
	warning() << "Desktop file \"" << *it
		  << "\" doesn't seem to describe a plugin "
		  << "(misses Misc and/or Plugin group)" << endl;
      }
    }
  }

  void * PluginLoaderBase::mainFunc( const QString & type,
				     const char * mf_name ) const {
    if ( type.isEmpty() || !mPluginMap.contains( type ) )
      return 0;

    const QString libName = mPluginMap[ type ].library;
    if ( libName.isEmpty() )
      return 0;

    const KLibrary * lib = openLibrary( libName );
    if ( !lib )
      return 0;

    mPluginMap[ type ].loaded = true;

    const QString factory_name = libName + '_' + mf_name;
    if ( !lib->hasSymbol( factory_name.latin1() ) ) {
      warning() << "No symbol named \"" << factory_name.latin1() << "\" ("
		<< factory_name << ") was found in library \"" << libName
		<< "\"" << endl;
      return 0;
    }

    return lib->symbol( factory_name.latin1() );
  }

  const KLibrary * PluginLoaderBase::openLibrary( const QString & libName ) const {

    const QString path = KLibLoader::findLibrary( QFile::encodeName( libName ) );

    if ( path.isEmpty() ) {
      warning() << "No plugin library named \"" << libName
		<< "\" was found!" << endl;
      return 0;
    }

    const KLibrary * library = KLibLoader::self()->library( QFile::encodeName( path ) );

    debug( !library ) << "Could not load library '" << libName << "'" << endl;

    return library;
  }


}; // namespace KMime
