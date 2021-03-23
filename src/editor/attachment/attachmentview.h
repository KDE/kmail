/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * SPDX-FileCopyrightText: 2003 Ingo Kloecker <kloecker@kde.org>
 * SPDX-FileCopyrightText: 2007 Thomas McGuire <Thomas.McGuire@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KConfigGroup>
#include <QTreeView>

class QContextMenuEvent;
class QToolButton;
class QLabel;
namespace MessageComposer
{
class AttachmentModel;
}

namespace KMail
{
class AttachmentView : public QTreeView
{
    Q_OBJECT

public:
    /// can't change model afterwards.
    explicit AttachmentView(MessageComposer::AttachmentModel *model, QWidget *parent = nullptr);
    ~AttachmentView() override;

    QWidget *widget() const;

public Q_SLOTS:
    /// model sets these
    void setEncryptEnabled(bool enabled);
    void setSignEnabled(bool enabled);
    void hideIfEmpty();
    void selectNewAttachment();

    void updateAttachmentLabel();

protected:
    /** reimpl to avoid default drag cursor */
    void startDrag(Qt::DropActions supportedActions) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    /** reimpl to avoid drags from ourselves */
    void dragEnterEvent(QDragEnterEvent *event) override;

private:
    void slotShowHideAttchementList(bool);
    void saveHeaderState();
    void restoreHeaderState();

Q_SIGNALS:
    void contextMenuRequested();
    void modified(bool);

private:
    MessageComposer::AttachmentModel *const mModel;
    QToolButton *const mToolButton;
    QLabel *const mInfoAttachment;
    QWidget *const mWidget;
    KConfigGroup grp;
};
} // namespace KMail

