/*
 *   This file is part of kmail
 *
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "colorlistbox.h"

#include <KColorMimeData>
#include <QColorDialog>

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHeaderView>

ColorListBox::ColorListBox(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(1);
    setRootIsDecorated(false);
    header()->hide();

    connect(this, &ColorListBox::doubleClicked, this, &ColorListBox::newColor);
    setAcceptDrops(true);
}

void ColorListBox::addColor(const QString &text, const QColor &color)
{
    auto item = new QTreeWidgetItem(QStringList() << text);
    item->setData(0, Qt::DecorationRole, color);
    addTopLevelItem(item);
}

void ColorListBox::setColorSilently(int index, const QColor &color)
{
    if (index < model()->rowCount()) {
        topLevelItem(index)->setData(0, Qt::DecorationRole, color);
    }
}

void ColorListBox::setColor(int index, const QColor &color)
{
    if (index < model()->rowCount()) {
        topLevelItem(index)->setData(0, Qt::DecorationRole, color);
        Q_EMIT changed();
    }
}

QColor ColorListBox::color(int index) const
{
    if (index < model()->rowCount()) {
        return topLevelItem(index)->data(0, Qt::DecorationRole).value<QColor>();
    } else {
        return Qt::black;
    }
}

void ColorListBox::newColor(const QModelIndex &index)
{
    if (!isEnabled()) {
        return;
    }

    if (index.isValid()) {
        QColor c = color(index.row());
        c = QColorDialog::getColor(c, this);
        if (c.isValid()) {
            setColor(index.row(), c);
        }
    }
}

void ColorListBox::dragEnterEvent(QDragEnterEvent *e)
{
    if (KColorMimeData::canDecode(e->mimeData()) && isEnabled()) {
        mCurrentOnDragEnter = currentItem();
        e->setAccepted(true);
    } else {
        mCurrentOnDragEnter = nullptr;
        e->setAccepted(false);
    }
}

void ColorListBox::dragLeaveEvent(QDragLeaveEvent *)
{
    if (mCurrentOnDragEnter) {
        setCurrentItem(mCurrentOnDragEnter);
        mCurrentOnDragEnter = nullptr;
    }
}

void ColorListBox::dragMoveEvent(QDragMoveEvent *e)
{
    if (KColorMimeData::canDecode(e->mimeData()) && isEnabled()) {
        QTreeWidgetItem *item = itemAt(e->pos());
        if (item) {
            setCurrentItem(item);
        }
    }
}

void ColorListBox::dropEvent(QDropEvent *e)
{
    const QColor color = KColorMimeData::fromMimeData(e->mimeData());
    if (color.isValid()) {
        QTreeWidgetItem *item = currentItem();
        if (item) {
            item->setData(0, Qt::DecorationRole, color);
            Q_EMIT changed();
        }
        mCurrentOnDragEnter = nullptr;
    }
}
