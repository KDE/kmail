/* -*- mode: C++ -*-
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
#ifndef EXPIREJOB_H
#define EXPIREJOB_H

#include "jobscheduler.h"
#include "kmcommands.h"

namespace KMail {

class ExpireJob : public ScheduledJob
{
  Q_OBJECT
public:
  ExpireJob( KMFolder* folder, bool immediate );
  virtual ~ExpireJob();

  virtual void execute();
  virtual void kill();

private slots:
  void slotDoWork();
  void slotMessagesMoved( KMCommand *command );

private:
  void done();

private:
  QTimer mTimer;
  QPtrList<KMMsgBase> mRemovedMsgs;
  int mCurrentIndex;
  int mMaxUnreadTime;
  int mMaxReadTime;
  bool mFolderOpen;
  KMFolder *mMoveToFolder;
};

/// A scheduled "expire mails in this folder" task.
class ScheduledExpireTask : public ScheduledTask
{
public:
  /// If immediate is set, the job will execute synchronously. This is used when
  /// the user requests explicitely that the operation should happen immediately.
  ScheduledExpireTask( KMFolder* folder, bool immediate )
    : ScheduledTask( folder, immediate ) {}
  virtual ~ScheduledExpireTask() {}
  virtual ScheduledJob* run() {
    return folder() ? new ExpireJob( folder(), isImmediate() ) : 0;
  }
  virtual int taskTypeId() const { return 1; }
};

} // namespace

#endif /* EXPIREJOB_H */

