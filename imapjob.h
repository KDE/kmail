/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2002-2003 Zack Rusin <zack@kde.org>
 *                2000-2002 Michael Haeckel <haeckel@kde.org>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#ifndef IMAPJOB_H
#define IMAPJOB_H

#include <kio/job.h>
#include <kio/global.h>

#include "folderjob.h"

#include <qcstring.h>

class KMAcctImap;
class KMMessage;
class KMFolderTreeItem;
class KMFolderImap;
namespace KPIM {
  class ProgressItem;
}
using KPIM::ProgressItem;

namespace KMail {

class AttachmentStrategy;

class ImapJob : public FolderJob
{
  Q_OBJECT
  friend class ::KMAcctImap;

public:
  ImapJob( KMMessage *msg, JobType jt = tGetMessage, KMFolderImap *folder = 0,
           QString partSpecifier = QString::null, const AttachmentStrategy *as = 0 );
  ImapJob( QPtrList<KMMessage>& msgList, QString sets,
           JobType jt = tGetMessage, KMFolderImap *folder = 0 );
  virtual ~ImapJob();

  void setParentFolder( const KMFolderImap* parent );
  ProgressItem* parentProgressItem() const { return mParentProgressItem; }
  void setParentProgressItem( ProgressItem *p ) { mParentProgressItem = p; }

private slots:
  void slotGetMessageResult( KIO::Job * job );
  void slotGetBodyStructureResult( KIO::Job * job );
  void slotGetNextMessage();
  /** Feeds the message in pieces to the server */
  void slotPutMessageDataReq( KIO::Job *job, QByteArray &data );
  void slotPutMessageResult( KIO::Job *job );
  void slotPutMessageInfoData(KIO::Job *, const QString &data);
  /** result of a copy-operation */
  void slotCopyMessageResult( KIO::Job *job );
  void slotCopyMessageInfoData( KIO::Job *, const QString &data );
  void slotProcessedSize( KIO::Job *, KIO::filesize_t processed );

private:
  void execute();
  void init( JobType jt, QString sets, KMFolderImap *folder,
             QPtrList<KMMessage>& msgList );
  KIO::Job *mJob;
  QByteArray mData;
  const AttachmentStrategy *mAttachmentStrategy;
  KMFolderImap *mParentFolder;
  ProgressItem *mParentProgressItem;
};

}


#endif
