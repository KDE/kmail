/*  -*- c++ -*-
    identitylistview.cpp

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2002 Marc Mutz <mutz@kde.org>
    SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "identitylistview.h"

#include <KIdentityManagement/kidentitymanagement/identity.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>

#include "kmkernel.h"

#include "kmail_debug.h"
#include <KLocalizedString>

#include <QDrag>
#include <QHeaderView>
#include <QLineEdit>
#include <QMimeData>

using namespace KMail;
//
//
// IdentityListViewItem
//
//

IdentityListViewItem::IdentityListViewItem(IdentityListView *parent, const KIdentityManagement::Identity &ident)
    : QTreeWidgetItem(parent)
    , mUOID(ident.uoid())
{
    init(ident);
}

IdentityListViewItem::IdentityListViewItem(IdentityListView *parent, QTreeWidgetItem *after, const KIdentityManagement::Identity &ident)
    : QTreeWidgetItem(parent, after)
    , mUOID(ident.uoid())
{
    init(ident);
}

uint IdentityListViewItem::uoid() const
{
    return mUOID;
}

KIdentityManagement::Identity &IdentityListViewItem::identity() const
{
    KIdentityManagement::IdentityManager *im = qobject_cast<IdentityListView *>(treeWidget())->identityManager();
    Q_ASSERT(im);
    return im->modifyIdentityForUoid(mUOID);
}

void IdentityListViewItem::setIdentity(const KIdentityManagement::Identity &ident)
{
    mUOID = ident.uoid();
    init(ident);
}

void IdentityListViewItem::redisplay()
{
    init(identity());
}

void IdentityListViewItem::init(const KIdentityManagement::Identity &ident)
{
    if (ident.isDefault()) {
        // Add "(Default)" to the end of the default identity's name:
        setText(0,
                i18nc("%1: identity name. Used in the config "
                      "dialog, section Identity, to indicate the "
                      "default identity",
                      "%1 (Default)",
                      ident.identityName()));
        QFont fontItem(font(0));
        fontItem.setBold(true);
        setFont(0, fontItem);
    } else {
        QFont fontItem(font(0));
        fontItem.setBold(false);
        setFont(0, fontItem);

        setText(0, ident.identityName());
    }
    setText(1, ident.fullEmailAddr());
}

//
//
// IdentityListView
//
//

IdentityListView::IdentityListView(QWidget *parent)
    : QTreeWidget(parent)
{
#ifndef QT_NO_DRAGANDDROP
    setDragEnabled(true);
    setAcceptDrops(true);
#endif
    setHeaderLabels(QStringList() << i18n("Identity Name") << i18n("Email Address"));
    setRootIsDecorated(false);
    header()->setSectionsMovable(false);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    setSelectionMode(ExtendedSelection);
    setColumnWidth(0, 175);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &IdentityListView::customContextMenuRequested, this, &IdentityListView::slotCustomContextMenuRequested);
}

void IdentityListView::editItem(QTreeWidgetItem *item, int column)
{
    if (column == 0 && item) {
        auto *lvItem = dynamic_cast<IdentityListViewItem *>(item);
        if (lvItem) {
            KIdentityManagement::Identity &ident = lvItem->identity();
            if (ident.isDefault()) {
                lvItem->setText(0, ident.identityName());
            }
        }

        Qt::ItemFlags oldFlags = item->flags();
        item->setFlags(oldFlags | Qt::ItemIsEditable);
        QTreeWidget::editItem(item, 0);
        item->setFlags(oldFlags);
    }
}

void IdentityListView::commitData(QWidget *editor)
{
    qCDebug(KMAIL_LOG) << "after editing";

    if (!selectedItems().isEmpty()) {
        auto edit = qobject_cast<QLineEdit *>(editor);
        if (edit) {
            IdentityListViewItem *item = dynamic_cast<IdentityListViewItem *>(selectedItems().at(0));
            const QString text = edit->text();
            Q_EMIT rename(item, text);
        }
    }
}

void IdentityListView::slotCustomContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem *item = itemAt(pos);
    if (item) {
        auto *lvItem = dynamic_cast<IdentityListViewItem *>(item);
        if (lvItem) {
            Q_EMIT contextMenu(lvItem, viewport()->mapToGlobal(pos));
        }
    } else {
        Q_EMIT contextMenu(nullptr, viewport()->mapToGlobal(pos));
    }
}

#ifndef QT_NO_DRAGANDDROP
void IdentityListView::startDrag(Qt::DropActions /*supportedActions*/)
{
    auto *item = dynamic_cast<IdentityListViewItem *>(currentItem());
    if (!item) {
        return;
    }

    auto drag = new QDrag(viewport());
    auto md = new QMimeData;
    drag->setMimeData(md);
    item->identity().populateMimeData(md);
    drag->setPixmap(QIcon::fromTheme(QStringLiteral("user-identity")).pixmap(16, 16));
    drag->exec(Qt::CopyAction);
}

#endif

KIdentityManagement::IdentityManager *IdentityListView::identityManager() const
{
    Q_ASSERT(mIdentityManager);
    return mIdentityManager;
}

void IdentityListView::setIdentityManager(KIdentityManagement::IdentityManager *im)
{
    mIdentityManager = im;
}
