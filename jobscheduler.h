/**
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

#ifndef KMAIL_JOBSCHEDULER_H
#define KMAIL_JOBSCHEDULER_H

#include <qobject.h>
#include <qvaluelist.h>
#include <qguardedptr.h>
#include <qtimer.h>

#include "folderjob.h"

// If this define is set, JobScheduler will show debug output, and related kmkernel timers will be shortened
// This is for debugging purposes only, don't commit with it.
//#define DEBUG_SCHEDULER

class KMFolder;
namespace KMail {

class FolderJob;
class ScheduledJob;

/**
 * A scheduled task is some information about a folder job that should be run later.
 * As long as it's not running, it's called a "task", i.e. something that needs to be done.
 * Tasks are held in the @see JobScheduler.
 */
class ScheduledTask {
public:
  /// Create a scheduled task for a given folder
  /// If @p immediate is true, the scheduler will run this task as soon
  /// as possible (but won't interrupt a currently running job for it)
  ScheduledTask( KMFolder* folder, bool immediate )
    : mCurrentFolder( folder ), mImmediate( immediate ) {}
  virtual ~ScheduledTask() {}

  /// Run this task, i.e. create a job for it.
  /// Important: the job's execute() method must either call open() on the
  /// folder or storage immediately, or abort (deleting itself).
  /// Usually, that job should also be cancellable.
  /// Otherwise (if the open() is delayed) an unrelated open() could happen first
  /// and mess things up.
  /// If for some reason (e.g. folder deleted) nothing should be done, return 0.
  virtual ScheduledJob* run() = 0;

  /// An identifier for the type of task (a bit like QListViewItem::rtti)
  /// This allows to automatically prevent two identical tasks from being scheduled
  /// for the same folder. To circumvent this feature and make every task
  /// unique, return 0 here.
  virtual int taskTypeId() const = 0;

  /// The folder which this task is about, 0 if it was deleted meanwhile.
  KMFolder* folder() const { return mCurrentFolder; }

  bool isImmediate() const { return mImmediate; }

private:
  QGuardedPtr<KMFolder> mCurrentFolder;
  bool mImmediate;
};

/**
 * The unique JobScheduler instance (owned by kmkernel) implements "background processing"
 * of folder operations (like expiration and compaction). Tasks (things to be done)
 * are registered with the JobScheduler, and it will execute them one at a time,
 * separated with a 1-minute timer. The jobs themselves should use timers to avoid
 * using too much CPU for too long. Tasks for opened folders are not executed until
 * the folder is closed.
 */
class JobScheduler : public QObject
{
  Q_OBJECT
public:
  JobScheduler( QObject* parent, const char* name = 0 );
  ~JobScheduler();

  /// Register a task to be done for a given folder
  /// The ownership of the task is transferred to the JobScheduler
  void registerTask( ScheduledTask* task );

  /// Called by [implementations of] FolderStorage::open()
  /// Interrupt any running job for this folder and re-schedule it for later
  void notifyOpeningFolder( KMFolder* folder );

private slots:
  /// Called by a timer to run the next job
  void slotRunNextJob();

  /// Called when the current job terminates
  void slotJobFinished();

private:
  void restartTimer();
  void interruptCurrentTask();
  void runTaskNow( ScheduledTask* task );
  typedef QValueList<ScheduledTask *> TaskList;
  void removeTask( TaskList::Iterator& it );
private:
  TaskList mTaskList; // FIFO of tasks to be run

  QTimer mTimer;
  int mPendingImmediateTasks;

  /// Information about the currently running job, if any
  ScheduledTask* mCurrentTask;
  ScheduledJob* mCurrentJob;
};

/**
 * Base class for scheduled jobs
 */
class ScheduledJob : public FolderJob
{
public:
  ScheduledJob( KMFolder* folder, bool immediate );

  bool isOpeningFolder() const { return mOpeningFolder; }

protected:
  bool mImmediate;
  bool mOpeningFolder;
};

} // namespace

#endif /* KMAIL_JOBSCHEDULER_H */
