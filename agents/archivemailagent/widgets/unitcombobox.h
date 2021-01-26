/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UNITCOMBOBOX_H
#define UNITCOMBOBOX_H

#include "archivemailinfo.h"
#include <QComboBox>

class UnitComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit UnitComboBox(QWidget *parent = nullptr);
    ~UnitComboBox();
    ArchiveMailInfo::ArchiveUnit unit() const;
    void setUnit(ArchiveMailInfo::ArchiveUnit unit);
};

#endif // UNITCOMBOBOX_H
