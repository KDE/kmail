/*
  Copyright (c) 2015-2020 Laurent Montel <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#ifndef POTENTIALPHISHINGDETAILWIDGET_H
#define POTENTIALPHISHINGDETAILWIDGET_H

#include <QWidget>
#include "kmail_private_export.h"
class QListWidget;
class KMAILTESTS_TESTS_EXPORT PotentialPhishingDetailWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailWidget(QWidget *parent = nullptr);
    ~PotentialPhishingDetailWidget();

    void save();

    void fillList(const QStringList &lst);
private:
    QListWidget *mListWidget = nullptr;
};

#endif // POTENTIALPHISHINGDETAILWIDGET_H
