/*
  This file is part of KDE Kontact.

  Copyright (C) 2004 Tobias Koenig <tokoe@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "dropwidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
DropWidget::DropWidget(QWidget *parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
}

void DropWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QStringLiteral("application/x-kontact-summary"))) {
        event->acceptProposedAction();
    }
}

void DropWidget::dropEvent(QDropEvent *event)
{
    int alignment = (event->pos().x() < (width() / 2) ? Qt::AlignLeft : Qt::AlignRight);
    alignment |= (event->pos().y() < (height() / 2) ? Qt::AlignTop : Qt::AlignBottom);
    Q_EMIT summaryWidgetDropped(this, event->source(), alignment);
}
