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

#ifndef __LIBKDEPIM_PLUGINLOADERBASE_H__
#define __LIBKDEPIM_PLUGINLOADERBASE_H__

#include <qstring.h>
#include <qmap.h>

class KLibrary;
class QStringList;

namespace KPIM {

  class PluginMetaData {
  public:
    PluginMetaData() {}
    PluginMetaData( const QString & lib, const QString & name,
		    const QString & comment )
      : library( lib ), nameLabel( name ),
	descriptionLabel( comment ), loaded( false ) {}
    QString library;
    QString nameLabel;
    QString descriptionLabel;
    mutable bool loaded;
  };

  class PluginLoaderBase {
  protected:
    PluginLoaderBase();
    virtual ~PluginLoaderBase();

  public:
    /** Returns a list of all available plugin objects (of kind @p T) */
    QStringList types() const;

    /** Returns the @ref PluginMetaData structure for a given type */
    const PluginMetaData * infoForName( const QString & type ) const;

    /** Overload this method in subclasses to call @ref doScan with
        the right @p path argument */
    virtual void scan() = 0;

  protected:
    /** Rescans the plugin directory to find any newly installed
	plugins. Extend this method in subclasses to add any
	builtins. Subclasses must call this explicitely. It's not
	called for them in the constructor.
    **/
    void doScan( const char * path );

    /** Returns a pointer to symbol @p main_func in the library that
        implements the plugin of type @p type */
    void * mainFunc( const QString & type, const char * main_func ) const;

  private:
    const KLibrary * openLibrary( const QString & libName ) const;
    QMap< QString, PluginMetaData > mPluginMap;

    class Private;
    Private * d;
  };

}; // namespace KMime

#endif // __LIBKDEPIM_PLUGINLOADERBASE_H__
