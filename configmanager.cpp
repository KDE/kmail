/*
    configmanager.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "configmanager.h"

ConfigManager::ConfigManager( QObject * parent, const char * name )
  : QObject( parent, name )
{

}

ConfigManager::~ConfigManager()
{

}

#include "configmanager.moc"
