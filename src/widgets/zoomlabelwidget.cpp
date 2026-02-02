/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "zoomlabelwidget.h"
#include <KLocalizedString>
#include <QWheelEvent>

ZoomLabelWidget::ZoomLabelWidget(QWidget *parent)
    : QLabel(parent)
{
}

ZoomLabelWidget::~ZoomLabelWidget() = default;

void ZoomLabelWidget::setZoom(qreal zoomFactor)
{
    mZoomFactor = zoomFactor;
    if (zoomFactor != 100.0) {
        setText(i18n("Zoom: %1%", zoomFactor));
        show();
    } else {
        hide();
    }
}

void ZoomLabelWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        const int y = event->angleDelta().y();
        if (y < 0) {
            Q_EMIT changeZoom(mZoomFactor - 10);
        } else if (y > 0) {
            Q_EMIT changeZoom(mZoomFactor + 10);
        }
    } else {
        QLabel::wheelEvent(event);
    }
}
#include "moc_zoomlabelwidget.cpp"
