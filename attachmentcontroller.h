/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * Various authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KMAIL_ATTACHMENTCONTROLLER_H
#define KMAIL_ATTACHMENTCONTROLLER_H

#include "attachmentcontrollerbase.h"
#include "attachmentview.h"


class KMComposeWin;

namespace Message {
class AttachmentModel;
}

namespace KMail {

class AttachmentController : public Message::AttachmentControllerBase
{
  Q_OBJECT

  public:
    AttachmentController( Message::AttachmentModel *model, AttachmentView *view, KMComposeWin *composer );
    ~AttachmentController();

  public slots:

     /// @reimp
    virtual void attachMyPublicKey();

  private slots:

    void identityChanged();
    void actionsCreated();
    void addAttachmentItems( const Akonadi::Item::List &items );
    void slotFetchJob( KJob * job );
    void selectionChanged();
    void onShowAttachment( KMime::Content *content, const QByteArray &charset );

  private:
    KMComposeWin *mComposer;
    AttachmentView *mView;
};

} // namespace KMail

#endif // KMAIL_ATTACHMENTCONTROLLER_H
