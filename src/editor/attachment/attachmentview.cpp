/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>
 *
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * SPDX-FileCopyrightText: 2003 Ingo Kloecker <kloecker@kde.org>
 * SPDX-FileCopyrightText: 2007 Thomas McGuire <Thomas.McGuire@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "attachmentview.h"

#include "kmkernel.h"
#include "util.h"

#include <MessageComposer/AttachmentModel>

#include <QContextMenuEvent>
#include <QDrag>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QToolButton>

#include <KLocalizedString>
#include <QIcon>

#include <KFormat>
#include <MessageCore/AttachmentPart>
using MessageCore::AttachmentPart;

using namespace KMail;

AttachmentView::AttachmentView(MessageComposer::AttachmentModel *model, QWidget *parent)
    : QTreeView(parent)
    , mModel(model)
    , mToolButton(new QToolButton(this))
    , mInfoAttachment(new QLabel(this))
    , mWidget(new QWidget())
    , grp(KMKernel::self()->config()->group("AttachmentView"))
{
    auto lay = new QHBoxLayout(mWidget);
    lay->setContentsMargins({});
    connect(mToolButton, &QAbstractButton::toggled, this, &AttachmentView::slotShowHideAttchementList);
    mToolButton->setIcon(QIcon::fromTheme(QStringLiteral("mail-attachment")));
    mToolButton->setAutoRaise(true);
    mToolButton->setCheckable(true);
    lay->addWidget(mToolButton);
    mInfoAttachment->setContentsMargins({});
    mInfoAttachment->setTextFormat(Qt::PlainText);
    lay->addWidget(mInfoAttachment);

    connect(model, &MessageComposer::AttachmentModel::encryptEnabled, this, &AttachmentView::setEncryptEnabled);
    connect(model, &MessageComposer::AttachmentModel::signEnabled, this, &AttachmentView::setSignEnabled);

    auto sortModel = new QSortFilterProxyModel(this);
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
    Q_UNUSED(event)
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
            auto part = model()->data(index, MessageComposer::AttachmentModel::AttachmentPartRole).value<AttachmentPart::Ptr>();
            toRemove.append(part);
        }
        for (const AttachmentPart::Ptr &part : std::as_const(toRemove)) {
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
        selectionModel()->select(selectionModel()->currentIndex(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void AttachmentView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions)

    const QModelIndexList selection = selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        QMimeData *mimeData = model()->mimeData(selection);
        auto drag = new QDrag(this);
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
