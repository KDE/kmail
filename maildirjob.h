/*
 * maildirjob.h
 *
 * Copyright (C)  2002  Zack Rusin <zack@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef MAILDIRJOB_H
#define MAILDIRJOB_H

#include "folderjob.h"

class KMFolderMaildir;

namespace KMail {

class MaildirJob : public FolderJob
{
  Q_OBJECT
public:
  MaildirJob( KMMessage *msg, JobType jt = tGetMessage, KMFolder *folder = 0 );
  MaildirJob( QPtrList<KMMessage>& msgList, const QString& sets,
              JobType jt = tGetMessage, KMFolder *folder = 0 );
  virtual ~MaildirJob();

  void setParentFolder( KMFolderMaildir* parent );
protected:
  void execute();
  void expireMessages();
protected slots:
  void startJob();
private:
  KMFolderMaildir* mParentFolder;
};

}

#endif
