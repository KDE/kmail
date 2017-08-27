/*
   Copyright (C) 2012-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef ADDARCHIVEMAILDIALOG_H
#define ADDARCHIVEMAILDIALOG_H

#include "MailCommon/BackupJob"
#include "archivemailinfo.h"
#include <QDialog>
#include <Collection>
class QUrl;
class QCheckBox;
class KUrlRequester;
class QSpinBox;
class QPushButton;

class FormatComboBox;
class UnitComboBox;
namespace MailCommon {
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
    bool recursive() const;

    void setSelectedFolder(const Akonadi::Collection &collection);
    Akonadi::Collection selectedFolder() const;

    QUrl path() const;
    void setPath(const QUrl &);

    ArchiveMailInfo *info();

    void setMaximumArchiveCount(int);

    int maximumArchiveCount() const;

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

#endif // ADDARCHIVEMAILDIALOG_H
