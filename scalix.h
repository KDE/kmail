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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**
 * This file contains helper classes for Scalix groupware support.
 * As the storage system of Scalix is quite similar to Kolab we reuse some of
 * the exsiting code and replace other code by our own.
 *
 * Differences between Kolab and Scalix:
 *
 * Groupware folder:
 * Kolab marks the groupware folders (e.g. Contacts, Calendar etc.) by setting special
 * annotations. Scalix however passes the information, which folder to treat as a
 * special groupware folder, via custom IMAP attributes in the LIST result of the
 * toplevel folder.
 *
 * Creating groupware folders:
 * Kolab creates a normal folder and marks it as groupware folder by setting the right
 * annotation. Scalix however has a special IMAP command 'X-CREATE-SPECIAL <type> <name>'
 * to create a new groupware folder.
 */

#ifndef SCALIX_H
#define SCALIX_H

#include <QtCore/QString>

#include "kmfoldertype.h"

class KMFolder;
class KMFolderDir;

namespace Scalix {

/**
 * This class takes a folder attribute string as argument and provides access to the single
 * parts.
 */
class FolderAttributeParser
{
  public:
    FolderAttributeParser( const QString &attribute );

    QString folderClass() const;
    QString folderName() const;

  private:
    QString mFolderClass;
    QString mFolderName;
};

class Utils
{
  public:
    static KMFolder* findStandardResourceFolder( KMFolderDir* folderParentDir,
                                                 KMail::FolderContentsType contentsType,
                                                 const QStringList &attributes );

    static KMail::FolderContentsType scalixIdToContentsType( const QString &name );

    static QString contentsTypeToScalixId( KMail::FolderContentsType type );
};

}

#endif
