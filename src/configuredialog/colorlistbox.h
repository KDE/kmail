/*
 *   This file is part of libkdepim.
 *
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

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
    Q_REQUIRED_RESULT QColor color(int index) const;

Q_SIGNALS:
    void changed();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    void newColor(const QModelIndex &index);
    QTreeWidgetItem *mCurrentOnDragEnter = nullptr;
};

