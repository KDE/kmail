/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DROP_WIDGET_H
#define DROP_WIDGET_H

#include <QWidget>

class QDragEnterEvent;
class QDropEvent;

class DropWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DropWidget(QWidget *parent);

Q_SIGNALS:
    void summaryWidgetDropped(QWidget *target, QObject *source, int alignment);

protected:
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;
};

#endif
