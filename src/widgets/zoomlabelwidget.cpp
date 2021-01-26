/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "zoomlabelwidget.h"
#include <KLocalizedString>

ZoomLabelWidget::ZoomLabelWidget(QWidget *parent)
    : QLabel(parent)
{
}

ZoomLabelWidget::~ZoomLabelWidget() = default;

void ZoomLabelWidget::setZoom(qreal zoomFactor)
{
    if (zoomFactor != 100.0) {
        setText(i18n("Zoom: %1%", zoomFactor));
        show();
    } else {
        hide();
    }
}
