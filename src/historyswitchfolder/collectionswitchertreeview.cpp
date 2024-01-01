/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2021-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionswitchertreeview.h"

#include <QKeyEvent>

CollectionSwitcherTreeView::CollectionSwitcherTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setTextElideMode(Qt::ElideMiddle);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setHeaderHidden(true);
    setRootIsDecorated(false);
}

CollectionSwitcherTreeView::~CollectionSwitcherTreeView() = default;

int CollectionSwitcherTreeView::sizeHintWidth() const
{
    return sizeHintForColumn(0);
}

void CollectionSwitcherTreeView::resizeColumnsToContents()
{
    resizeColumnToContents(0);
}

void CollectionSwitcherTreeView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        Q_EMIT collectionSelected(selectionModel()->currentIndex());
        event->accept();
        hide();
    } else {
        QTreeView::keyReleaseEvent(event);
    }
}

void CollectionSwitcherTreeView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        hide();
    } else {
        QTreeView::keyPressEvent(event);
    }
}

void CollectionSwitcherTreeView::showEvent(QShowEvent *event)
{
    resizeColumnsToContents();
    reset();
    QTreeView::showEvent(event);
}

#include "moc_collectionswitchertreeview.cpp"
