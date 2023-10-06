/*
   SPDX-FileCopyrightText: 2015-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <MailCommon/BackupJob>

#include <QComboBox>

class FormatComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit FormatComboBox(QWidget *parent = nullptr);
    ~FormatComboBox() override;

    [[nodiscard]] MailCommon::BackupJob::ArchiveType format() const;
    void setFormat(MailCommon::BackupJob::ArchiveType type);
};
