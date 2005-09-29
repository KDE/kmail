/*
 * Copyright (c) 2004 David Faure <faure@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
#ifndef COMPACTIONJOB_H
#define COMPACTIONJOB_H

#include "jobscheduler.h"
#include <qstringlist.h>

namespace KMail {

/**
 * A job that runs in the background and compacts mbox folders.
 */
class MboxCompactionJob : public ScheduledJob
{
  Q_OBJECT
public:
  /// @p folder should be a folder with a KMFolderMbox storage.
  MboxCompactionJob( KMFolder* folder, bool immediate );
  virtual ~MboxCompactionJob();

  int executeNow( bool silent );
  virtual void execute() { executeNow( false ); }
  virtual void kill();

private slots:
  void slotDoWork();

private:
  // Real folder location, != location in case of symlinks
  QString realLocation() const;
  void done( int rc );

private:
  QTimer mTimer;
  QString mTempName;
  FILE *mTmpFile;
  off_t mOffset;
  int mCurrentIndex;
  bool mFolderOpen;
  bool mSilent;
};

/**
 * A job that runs in the background and compacts maildir folders.
 */
class MaildirCompactionJob : public ScheduledJob
{
  Q_OBJECT
public:
  /// @p folder should be a folder with a KMFolderMaildir storage.
  MaildirCompactionJob( KMFolder* folder, bool immediate );
  virtual ~MaildirCompactionJob();

  int executeNow( bool silent );
  virtual void execute() { executeNow( false ); }
  virtual void kill();

private slots:
  void slotDoWork();

private:
  void done( int rc );

private:
  QTimer mTimer;
  QStringList mEntryList;
  int mCurrentIndex;
  bool mFolderOpen;
  bool mSilent;
};

/// A scheduled "compact mails in this folder" task.
class ScheduledCompactionTask : public ScheduledTask
{
public:
  /// If immediate is set, the job will execute synchronously. This is used when
  /// the user requests explicitely that the operation should happen immediately.
  ScheduledCompactionTask( KMFolder* folder, bool immediate )
    : ScheduledTask( folder, immediate ) {}
  virtual ~ScheduledCompactionTask() {}
  virtual ScheduledJob* run();
  virtual int taskTypeId() const { return 2; }
};

} // namespace

#endif /* COMPACTIONJOB_H */

