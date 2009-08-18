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

#include <QtCore/QObject>

#include <KDE/KUrl>

#include <KPIMIdentities/Identity>

#include <libkdepim/attachmentpart.h>

class KActionCollection;

class KMComposeWin;

namespace KMail {

class AttachmentModel;
class AttachmentView;

class AttachmentController : public QObject
{
  Q_OBJECT

  public:
    AttachmentController( AttachmentModel *model, AttachmentView *view, KMComposeWin *composer );
    ~AttachmentController();

    void createActions();

    // TODO dnd stuff...

  public slots:
    /// model sets these
    void setEncryptEnabled( bool enabled );
    void setSignEnabled( bool enabled );
    /// compression is async...
    void compressAttachment( KPIM::AttachmentPart::Ptr part, bool compress );
    void showContextMenu();
    void openAttachment( KPIM::AttachmentPart::Ptr part );
    void viewAttachment( KPIM::AttachmentPart::Ptr part );
    void editAttachment( KPIM::AttachmentPart::Ptr part, bool openWith = false );
    void editAttachmentWith( KPIM::AttachmentPart::Ptr part );
    void saveAttachmentAs( KPIM::AttachmentPart::Ptr part );
    void attachmentProperties( KPIM::AttachmentPart::Ptr part );
    void showAddAttachmentDialog();
    void addAttachment( const KUrl &url );
    void addAttachments( const KUrl::List &urls );
    void showAttachPublicKeyDialog();
    void attachMyPublicKey();

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void identityChanged() )
    Q_PRIVATE_SLOT( d, void selectionChanged() )
    Q_PRIVATE_SLOT( d, void attachmentRemoved( KPIM::AttachmentPart::Ptr ) )
    Q_PRIVATE_SLOT( d, void compressJobResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void loadJobResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void openSelectedAttachments() )
    Q_PRIVATE_SLOT( d, void viewSelectedAttachments() )
    Q_PRIVATE_SLOT( d, void editSelectedAttachment() )
    Q_PRIVATE_SLOT( d, void editSelectedAttachmentWith() )
    Q_PRIVATE_SLOT( d, void removeSelectedAttachments() )
    Q_PRIVATE_SLOT( d, void saveSelectedAttachmentAs() )
    Q_PRIVATE_SLOT( d, void selectedAttachmentProperties() )
    Q_PRIVATE_SLOT( d, void editDone( KMail::EditorWatcher* ) )
    Q_PRIVATE_SLOT( d, void attachPublicKeyJobResult( KJob* ) )
};

} // namespace KMail

#endif // KMAIL_ATTACHMENTCONTROLLER_H
