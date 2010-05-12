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

#include "attachmentcontroller.h"

#include "messagecomposer/attachmentmodel.h"
#include "attachmentview.h"
#include "attachmentfrompublickeyjob.h"
#include "messageviewer/editorwatcher.h"
#include "globalsettings.h"
#include "kmkernel.h"
#include "kmcomposewin.h"
#include "kmcommands.h"
#include "foldercollection.h"

#include <akonadi/itemfetchjob.h>
#include <kio/jobuidelegate.h>

#include <QMenu>
#include <QPointer>

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KEncodingFileDialog>
#include <KFileDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMimeTypeTrader>
#include <KPushButton>
#include <KRun>
#include <KTemporaryFile>

#include "kmreadermainwin.h"
#include <kpimutils/kfileio.h>

#include <libkleo/kleo/cryptobackendfactory.h>
#include <libkleo/ui/keyselectiondialog.h>

#include <messagecore/attachmentcompressjob.h>
#include <messagecore/attachmentfrommimecontentjob.h>
#include <messagecore/attachmentfromurljob.h>
#include <messagecore/attachmentpropertiesdialog.h>

using namespace KMail;
using namespace KPIM;

AttachmentController::AttachmentController( Message::AttachmentModel *model, AttachmentView *view, KMComposeWin *composer )
  : AttachmentControllerBase( model, view, composer )
{
  mComposer = composer;
}

AttachmentController::~AttachmentController()
{
//   delete d;
}

void AttachmentController::attachMyPublicKey()
{
  const KPIMIdentities::Identity &identity = mComposer->identity();
  kDebug() << identity.identityName();
  exportPublicKey( mComposer->identity().pgpEncryptionKey() );
}

#include "attachmentcontroller.moc"
