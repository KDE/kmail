/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
