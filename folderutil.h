/* Copyright 2009 Klarälvdalens Datakonsult AB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FOLDERUTIL_H
#define FOLDERUTIL_H

#include "kmfoldertype.h"

class KMFolder;
class KMFolderDir;
class QString;

namespace KMail
{

namespace FolderUtil
{

/**
 * Low-level function to create a subfolder for a folder of any kind.
 *
 * @param parentFolder parent folder of the folder that should be created. Can be 0 in case of
 *                     local folders
 * @param parentDir parent folder directory, which should be the folder directory of parentFolder
 * @param folderName the name the newly created folder should have
 * @param namespaceName for (d)IMAP folders, the namespace the new folder should be in. Can be empty.
 * @param localFolderType for local folders, this determines if the folder should be MBOX or maildir
 *
 * @return the newly created folder or 0 in case an error occured
 */
KMFolder *createSubFolder( KMFolder *parentFolder, KMFolderDir *parentDir,
                           const QString &folderName, const QString &namespaceName,
                           KMFolderType localFolderType );

}

}

#endif
