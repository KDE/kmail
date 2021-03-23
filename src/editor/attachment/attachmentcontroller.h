/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * Various authors.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <AkonadiCore/Item>
#include <MessageComposer/AttachmentControllerBase>
class KMComposerWin;
class QModelIndex;
namespace MessageComposer
{
class AttachmentModel;
}

namespace KMail
{
class AttachmentView;

class AttachmentController : public MessageComposer::AttachmentControllerBase
{
    Q_OBJECT

public:
    explicit AttachmentController(MessageComposer::AttachmentModel *model, AttachmentView *view, KMComposerWin *composer);
    ~AttachmentController() override;

public Q_SLOTS:
    void attachMyPublicKey() override;

private:
    void identityChanged();
    void slotActionsCreated();
    void addAttachmentItems(const Akonadi::Item::List &items);
    void selectionChanged();
    void onShowAttachment(KMime::Content *content, const QByteArray &charset);
    void doubleClicked(const QModelIndex &itemClicked);
    void slotSelectAllAttachment();

    KMComposerWin *const mComposer;
    AttachmentView *const mView;
};
} // namespace KMail

