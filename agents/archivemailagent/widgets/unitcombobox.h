/*
   SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UNITCOMBOBOX_H
#define UNITCOMBOBOX_H

#include <QComboBox>
#include "archivemailinfo.h"

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
