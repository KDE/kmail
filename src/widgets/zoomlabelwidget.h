/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QLabel>

class ZoomLabelWidget : public QLabel
{
    Q_OBJECT
public:
    explicit ZoomLabelWidget(QWidget *parent = nullptr);
    ~ZoomLabelWidget() override;

    void setZoom(qreal zoomFactor);

protected:
    void wheelEvent(QWheelEvent *event) override;

Q_SIGNALS:
    void changeZoom(qreal zoomFactor);

private:
    qreal mZoomFactor = 100.0;
};
