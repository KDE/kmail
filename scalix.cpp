/*
 *   This file is part of KMail.
 *
 *   Copyright (C) 2007 Trolltech ASA. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "scalix.h"
#include "kmfolder.h"
#include "kmfolderdir.h"

#include <QtCore/QMap>

using namespace Scalix;

FolderAttributeParser::FolderAttributeParser( const QString &attribute )
{
  QStringList parts = attribute.split( ',', QString::SkipEmptyParts );

  for ( int i = 0; i < parts.count(); ++i ) {
    if ( parts[i].startsWith( QLatin1String( "\\X-SpecialFolder=" ) ) )
      mFolderName = parts[i].mid( 17 );
    else if ( parts[i].startsWith( QLatin1String( "\\X-FolderClass=" ) ) )
      mFolderClass = parts[i].mid( 15 );
  }
}

QString FolderAttributeParser::folderClass() const
{
  return mFolderClass;
}

QString FolderAttributeParser::folderName() const
{
  return mFolderName;
}

KMFolder* Utils::findStandardResourceFolder( KMFolderDir* folderParentDir,
                                             KMail::FolderContentsType contentsType,
                                             const QStringList &attributes )
{
  QMap<int, QString> typeMap;
  typeMap.insert( KMail::ContentsTypeCalendar, "IPF.Appointment" );
  typeMap.insert( KMail::ContentsTypeContact, "IPF.Contact" );
  typeMap.insert( KMail::ContentsTypeNote, "IPF.StickyNote" );
  typeMap.insert( KMail::ContentsTypeTask, "IPF.Task" );

  if ( !typeMap.contains( contentsType ) )
    return 0;

  for ( int i = 0; i < attributes.count(); ++i ) {
    FolderAttributeParser parser( attributes[ i ] );
    if ( parser.folderClass() == typeMap[ contentsType ] ) {
      KMFolderNode* node = folderParentDir->hasNamedFolder( parser.folderName() );
      if ( node && !node->isDir() )
        return static_cast<KMFolder*>( node );
    }
  }

  return 0;
}

KMail::FolderContentsType Utils::scalixIdToContentsType( const QString &name )
{
  if ( name == "IPF.Appointment" )
    return KMail::ContentsTypeCalendar;
  else if ( name == "IPF.Contact" )
    return KMail::ContentsTypeContact;
  else if ( name == "IPF.StickyNote" )
    return KMail::ContentsTypeNote;
  else if ( name == "IPF.Task" )
    return KMail::ContentsTypeTask;
  else
    return KMail::ContentsTypeMail;
}

QString Utils::contentsTypeToScalixId( KMail::FolderContentsType type )
{
  if ( type == KMail::ContentsTypeCalendar )
    return "IPF.Appointment";
  else if ( type == KMail::ContentsTypeContact )
    return "IPF.Contact";
  else if ( type == KMail::ContentsTypeNote )
    return "IPF.StickyNote";
  else if ( type == KMail::ContentsTypeTask )
    return "IPF.Task";
  else
    return "IPF.Note";
}
