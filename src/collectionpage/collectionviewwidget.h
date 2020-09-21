/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLLECTIONVIEWWIDGET_H
#define COLLECTIONVIEWWIDGET_H

#include <QWidget>

class CollectionViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionViewWidget(QWidget *parent = nullptr);
    ~CollectionViewWidget();
};

#endif // COLLECTIONVIEWWIDGET_H
