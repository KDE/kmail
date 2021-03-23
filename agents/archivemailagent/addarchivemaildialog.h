/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include <Collection>
#include <MailCommon/BackupJob>
#include <QDialog>
class QUrl;
class QCheckBox;
class KUrlRequester;
class QSpinBox;
class QPushButton;

class FormatComboBox;
class UnitComboBox;
namespace MailCommon
{
class FolderRequester;
}

class AddArchiveMailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddArchiveMailDialog(ArchiveMailInfo *info, QWidget *parent = nullptr);
    ~AddArchiveMailDialog();

    void setArchiveType(MailCommon::BackupJob::ArchiveType type);
    MailCommon::BackupJob::ArchiveType archiveType() const;

    void setRecursive(bool b);
    Q_REQUIRED_RESULT bool recursive() const;

    void setSelectedFolder(const Akonadi::Collection &collection);
    Akonadi::Collection selectedFolder() const;

    Q_REQUIRED_RESULT QUrl path() const;
    void setPath(const QUrl &);

    ArchiveMailInfo *info();

    void setMaximumArchiveCount(int);

    Q_REQUIRED_RESULT int maximumArchiveCount() const;

private:
    void slotFolderChanged(const Akonadi::Collection &);
    void slotUpdateOkButton();
    void load(ArchiveMailInfo *info);
    MailCommon::FolderRequester *mFolderRequester = nullptr;
    FormatComboBox *mFormatComboBox = nullptr;
    UnitComboBox *mUnits = nullptr;
    QCheckBox *mRecursiveCheckBox = nullptr;
    KUrlRequester *mPath = nullptr;
    QSpinBox *mDays = nullptr;
    QSpinBox *mMaximumArchive = nullptr;

    ArchiveMailInfo *mInfo = nullptr;
    QPushButton *mOkButton = nullptr;
};

