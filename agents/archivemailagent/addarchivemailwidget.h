/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include <QWidget>
class QUrl;
class QCheckBox;
class KUrlRequester;
class QSpinBox;
class QPushButton;

class FormatComboBox;
class UnitComboBox;
class ArchiveMailRangeWidget;
namespace MailCommon
{
class FolderRequester;
}
class AddArchiveMailWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AddArchiveMailWidget(ArchiveMailInfo *info, QWidget *parent = nullptr);
    ~AddArchiveMailWidget() override;

    void load(ArchiveMailInfo *info);
    [[nodiscard]] ArchiveMailInfo *info();
Q_SIGNALS:
    void enableOkButton(bool state);

private:
    void slotUpdateOkButton();
    void slotFolderChanged(const Akonadi::Collection &collection);

    MailCommon::FolderRequester *const mFolderRequester;
    FormatComboBox *const mFormatComboBox;
    UnitComboBox *const mUnits;
    QCheckBox *const mRecursiveCheckBox;
    KUrlRequester *const mPath;
    QSpinBox *const mDays;
    QSpinBox *const mMaximumArchive;
    ArchiveMailRangeWidget *const mArchiveMailRangeWidget;

    ArchiveMailInfo *mInfo = nullptr;
};
