/**
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef RENAMEJOB_H
#define RENAMEJOB_H

#include "folderjob.h"

class FolderStorage;
class KMFolderDir;
class KMFolder;
class KMCommand;

namespace KIO {
  class Job;
}

namespace KMail {

/**
 * Rename and move (d)imap folders
 * They can be moved everywhere (except search folders) as a new folder is
 * created, all messages are moved there and the original folder is deleted
 */
class RenameJob : public FolderJob
{
  Q_OBJECT
public:
  /**
   * Create a new job
   * @param storage the folder that should be renames
   * @param newName the new name of the folder
   * @param newParent the new parent if the folder should be moved, else 0
   */
  RenameJob( FolderStorage* storage, const QString& newName,
      KMFolderDir* newParent = 0 );

  virtual ~RenameJob();

  virtual void execute();

protected slots:
  /** Rename the folder */
  void slotRenameResult( KIO::Job* job );

  /** Move all messages from the original folder to mNewFolder */
  void slotMoveMessages();

  /** All messages are moved so remove the original folder */
  void slotMoveCompleted( KMCommand *command );

signals:
  /** Emitted when the job is done, check the success bool */
  void renameDone( QString newName, bool success );

protected:
  FolderStorage* mStorage;
  KMFolderDir* mNewParent;
  QString mNewName;
  QString mNewImapPath;
  QString mOldName;
  QString mOldImapPath;
  KMFolder* mNewFolder;
};

} // namespace KMail

#endif /* RENAMEJOB_H */

