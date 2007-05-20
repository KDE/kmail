/**
 *  Copyright (c) 2005 Till Adam <adam@kde.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
#ifndef COPYFOLDERJOB_H
#define COPYFOLDERJOB_H

#include "folderjob.h"

#include <QPointer>

class FolderStorage;
class KMFolderDir;
class KMFolder;
class KMCommand;

class KMFolderNode;

namespace KMail {

/**
 * Copy a hierarchy of folders somewhere else in the folder tree.  Currently
 * online imap folders are not supported as target folders, and the same is
 * true for search folders where it does not make much sense for them to be
 * target folders.
 */
class CopyFolderJob : public FolderJob
{
  Q_OBJECT
public:
  /**
   * Create a new job
   * @param storage of the folder that should be copied
   * @param newParent the target parent folder
   */
  CopyFolderJob( FolderStorage* const storage, KMFolderDir* const newParent = 0 );

  virtual ~CopyFolderJob();

  virtual void execute();

  /**
    Returns the newly created target folder.
  */
  KMFolder* targetFolder() const { return mNewFolder; }

protected slots:

  /** Create the target directory under the new parent. Returns success or failure.*/
  bool createTargetDir();

  /** Copy all messages from the original folder to mNewFolder */
  void copyMessagesToTargetDir();

  /** Called when the CopyCommand has either successfully completed copying
   * the contents of our folder to the new location or failed. */
  void slotCopyCompleted( KMCommand *command );

  /** Called when the previous sibling's copy operation completed.
   * @param success indicates whether the last copy was successful. */
  void slotCopyNextChild( bool success = true );

  /** Called when one of the operations of the foldre itself or one of it's
   * child folders failed and the already created target folder needs to be
   * removed again. */
  void rollback();

  /**
    Called when the online IMAP folder creation finished.
  */
  void folderCreationDone( const QString &name, bool success );

signals:
  /** Emitted when the job is done, check the success bool */
  void folderCopyComplete( bool success );

protected:
  QPointer<FolderStorage> const mStorage;
  KMFolderDir* const mNewParent;
  QPointer<KMFolder> mNewFolder;
  QList<KMFolderNode*>::Iterator mChildFolderNodeIterator;
  KMFolder* mNextChildFolder;
  bool mHasChildFolders;
};

} // namespace KMail

#endif /* COPYFOLDERJOB_H */
