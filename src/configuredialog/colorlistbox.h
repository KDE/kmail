/*
 *   This file is part of libkdepim.
 *
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2007 Mathias Soeken, msoeken@tzi.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef COLORLISTBOX_H
#define COLORLISTBOX_H

#include <QTreeWidget>

class QDragLeaveEvent;
class QDropEvent;
class QDragMoveEvent;
class QDragLeaveEvent;

class ColorListBox : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ColorListBox(QWidget *parent = nullptr);
    void addColor(const QString &text, const QColor &color = Qt::black);
    void setColor(int index, const QColor &color);
    // like setColor, but does not Q_EMIT changed()
    void setColorSilently(int index, const QColor &color);
    QColor color(int index) const;

Q_SIGNALS:
    void changed();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    void newColor(const QModelIndex &index);
    QTreeWidgetItem *mCurrentOnDragEnter;
};

#endif
