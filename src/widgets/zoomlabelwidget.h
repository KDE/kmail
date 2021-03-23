/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

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
};

