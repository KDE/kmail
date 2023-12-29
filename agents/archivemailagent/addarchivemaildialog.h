/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include <Akonadi/Collection>
#include <QDialog>

class AddArchiveMailWidget;
class AddArchiveMailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddArchiveMailDialog(ArchiveMailInfo *info, QWidget *parent = nullptr);
    ~AddArchiveMailDialog() override;

    [[nodiscard]] ArchiveMailInfo *info();

private:
    QPushButton *mOkButton = nullptr;
    AddArchiveMailWidget *const mAddArchiveMailWidget;
};
