/*
 * This file is part of KMail.
 * Copyright (c) 2011-2018 Laurent Montel <montel@kde.org>
 *
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>
 * Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>
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

#include "attachmentview.h"

#include "kmkernel.h"
#include "util.h"

#include <MessageComposer/AttachmentModel>

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QDrag>

#include <KConfigGroup>
#include <QIcon>
#include <KLocalizedString>

#include <MessageCore/AttachmentPart>
#include <KFormat>
using MessageCore::AttachmentPart;

using namespace KMail;

AttachmentView::AttachmentView(MessageComposer::AttachmentModel *model, QWidget *parent)
    : QTreeView(parent)
    , grp(KMKernel::self()->config()->group("AttachmentView"))
{
    mWidget = new QWidget();
    QHBoxLayout *lay = new QHBoxLayout(mWidget);
    lay->setMargin(0);
    mToolButton = new QToolButton;
    connect(mToolButton, &QAbstractButton::toggled, this, &AttachmentView::slotShowHideAttchementList);
    mToolButton->setIcon(QIcon::fromTheme(QStringLiteral("mail-attachment")));
    mToolButton->setAutoRaise(true);
    mToolButton->setCheckable(true);
    lay->addWidget(mToolButton);
    mInfoAttachment = new QLabel;
    mInfoAttachment->setMargin(0);
    mInfoAttachment->setTextFormat(Qt::PlainText);
    lay->addWidget(mInfoAttachment);

    mModel = model;
    connect(model, &MessageComposer::AttachmentModel::encryptEnabled, this, &AttachmentView::setEncryptEnabled);
    connect(model, &MessageComposer::AttachmentModel::signEnabled, this, &AttachmentView::setSignEnabled);

    QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(this);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setSourceModel(model);
    setModel(sortModel);
    connect(model, &MessageComposer::AttachmentModel::rowsInserted, this, &AttachmentView::hideIfEmpty);
    connect(model, &MessageComposer::AttachmentModel::rowsRemoved, this, &AttachmentView::hideIfEmpty);
    connect(model, &MessageComposer::AttachmentModel::rowsRemoved, this, &AttachmentView::selectNewAttachment);
    connect(model, &MessageComposer::AttachmentModel::dataChanged, this, &AttachmentView::updateAttachmentLabel);

    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setDropIndicatorShown(false);
    setSortingEnabled(true);

    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(false);
    restoreHeaderState();
    setColumnWidth(0, 200);
}

AttachmentView::~AttachmentView()
{
    saveHeaderState();
}

void AttachmentView::restoreHeaderState()
{
    header()->restoreState(grp.readEntry("State", QByteArray()));
}

void AttachmentView::saveHeaderState()
{
    grp.writeEntry("State", header()->saveState());
    grp.sync();
}

void AttachmentView::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);
    Q_EMIT contextMenuRequested();
}

void AttachmentView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        // Indexes are based on row numbers, and row numbers change when items are deleted.
        // Therefore, first we need to make a list of AttachmentParts to delete.
        AttachmentPart::List toRemove;
        const QModelIndexList selectedIndexes = selectionModel()->selectedRows();
        toRemove.reserve(selectedIndexes.count());
        for (const QModelIndex &index : selectedIndexes) {
            AttachmentPart::Ptr part = model()->data(
                index, MessageComposer::AttachmentModel::AttachmentPartRole).value<AttachmentPart::Ptr>();
            toRemove.append(part);
        }
        for (const AttachmentPart::Ptr &part : qAsConst(toRemove)) {
            mModel->removeAttachment(part);
        }
    } else {
        QTreeView::keyPressEvent(event);
    }
}

void AttachmentView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->source() == this) {
        // Ignore drags from ourselves.
        event->ignore();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void AttachmentView::setEncryptEnabled(bool enabled)
{
    setColumnHidden(MessageComposer::AttachmentModel::EncryptColumn, !enabled);
}

void AttachmentView::setSignEnabled(bool enabled)
{
    setColumnHidden(MessageComposer::AttachmentModel::SignColumn, !enabled);
}

void AttachmentView::hideIfEmpty()
{
    const bool needToShowIt = (model()->rowCount() > 0);
    setVisible(needToShowIt);
    mToolButton->setChecked(needToShowIt);
    widget()->setVisible(needToShowIt);
    if (needToShowIt) {
        updateAttachmentLabel();
    } else {
        mInfoAttachment->clear();
    }
    Q_EMIT modified(true);
}

void AttachmentView::updateAttachmentLabel()
{
    const MessageCore::AttachmentPart::List list = mModel->attachments();
    qint64 size = 0;
    for (const MessageCore::AttachmentPart::Ptr &part : list) {
        size += part->size();
    }
    mInfoAttachment->setText(i18np("1 attachment (%2)", "%1 attachments (%2)", model()->rowCount(), KFormat().formatByteSize(qMax(0LL, size))));
}

void AttachmentView::selectNewAttachment()
{
    if (selectionModel()->selectedRows().isEmpty()) {
        selectionModel()->select(selectionModel()->currentIndex(),
                                 QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void AttachmentView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);

    const QModelIndexList selection = selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        QMimeData *mimeData = model()->mimeData(selection);
        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);
    }
}

QWidget *AttachmentView::widget() const
{
    return mWidget;
}

void AttachmentView::slotShowHideAttchementList(bool show)
{
    setVisible(show);
    if (show) {
        mToolButton->setToolTip(i18n("Hide attachment list"));
    } else {
        mToolButton->setToolTip(i18n("Show attachment list"));
    }
}
