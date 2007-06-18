/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#ifndef FOLDERJOB_H
#define FOLDERJOB_H

#include "kmmessage.h"

#include <QList>
#include <QString>

class KMFolder;

namespace KMail {

class FolderJob : public QObject
{
  Q_OBJECT

public:
  enum JobType { tListMessages, tGetFolder, tCreateFolder, tExpungeFolder,
		 tDeleteMessage, tGetMessage, tPutMessage, tAddSubfolders,
		 tDeleteFolders, tCheckUidValidity, tRenameFolder,
                 tCopyMessage, tMoveMessage, tOther /* used by subclasses */ };
  /**
   * Constructs a new job, operating on the message msg, of type
   * @p jt and with a parent folder @p folder.
   */
  explicit FolderJob( KMMessage *msg, JobType jt = tGetMessage,
        KMFolder *folder = 0, const QString &partSpecifier = QString() );

  /**
   * Constructs a new job, operating on a message list @p msgList,
   * set @sets, JobType @p jt and with the parent folder @p folder.
   *
   */
  FolderJob( const QList<KMMessage*>& msgList, const QString& sets,
	     JobType jt = tGetMessage, KMFolder *folder = 0 );
  /**
   * This one should ONLY be used in derived folders, when a job
   * does something internal and needs to construct an empty parent
   * FolderJob
   */
  FolderJob( JobType jt );
  virtual ~FolderJob();

  QList<KMMessage*> msgList() const;
  /**
   * Start the job
   */
  void start();

  /**
   * Interrupt the job. Note that the finished() and result() signal
   * will be emitted, unless you called setPassiveDestructor(true) before.
   * This kills the job, don't use it afterwards.
   */
  virtual void kill();

  /**
   * @return the error code of the job. This must only be called from
   * the slot connected to the finished() signal.
   */
  int error() const { return mErrorCode; }

  /**
   * @return true if this job can be canceled, e.g. to exit the application
   */
  bool isCancellable() const { return mCancellable; }

  /**
   * Call this to change the "cancellable" property of this job.
   * By default, tListMessages, tGetMessage, tGetFolder and tCheckUidValidity
   * are cancellable, the others are not. But when copying, a non-cancellable
   * tGetMessage is needed.
   */
  void setCancellable( bool b ) { mCancellable = b; }

  void setPassiveDestructor( bool passive ) { mPassiveDestructor = passive; }
  bool passiveDestructor() { return mPassiveDestructor; }

signals:
  /**
   * Emitted whenever a KMMessage has been completely
   * retrieved from the server/folder.
   */
  void messageRetrieved( KMMessage * );

  /**
   * Emitted whenever a KMMessage was updated
   */
  void messageUpdated( KMMessage *, const QString& );

  /**
   * Emitted whenever a message has been stored in
   * the folder.
   */
  void messageStored( KMMessage * );

  /**
   * Emitted when a list of messages has been
   * copied to the specified location. QPtrList contains
   * the list of the copied messages.
   */
  void messageCopied( QList<KMMessage*> );

  /**
   * Overloaded signal to the one above. A lot of copying
   * specifies only one message as the argument and this
   * signal is easier to use when this happens.
   */
  void messageCopied( KMMessage * );

  /**
   * Emitted when the job finishes all processing.
   */
  void finished();

  /**
   * Emitted when the job finishes all processing.
   * More convenient signal than finished(), since it provides a pointer to the job.
   * This signal is emitted by the FolderJob destructor => do NOT downcast
   * the job to a subclass!
   */
  void result( KMail::FolderJob* job );

  /**
   * This progress signal contains the "done" and the "total" numbers so
   * that the caller can either make a % out of it, or combine it into
   * a higher-level progress info.
   */
  void progress( unsigned long bytesDownloaded, unsigned long bytesTotal );

private:
  void init();

protected:
  /**
   * Has to be reimplemented. It's called by the start() method. Should
   * start the processing of the specified job function.
   */
  virtual void execute()=0;

  QList<KMMessage*>   mMsgList;
  JobType             mType;
  QString             mSets;
  KMFolder*           mSrcFolder;
  KMFolder*           mDestFolder;
  QString             mPartSpecifier;
  int                 mErrorCode;

  //finished() won't be emitted when this is set
  bool                mPassiveDestructor;
  bool                mStarted;
  bool                mCancellable;
};

}

#endif
