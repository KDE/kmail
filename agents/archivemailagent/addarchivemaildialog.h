/*
  Copyright (c) 2012-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ADDARCHIVEMAILDIALOG_H
#define ADDARCHIVEMAILDIALOG_H

#include "MailCommon/BackupJob"
#include "archivemailinfo.h"
#include <QDialog>
#include <Collection>
class QUrl;
class KComboBox;
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
    explicit AddArchiveMailDialog(ArchiveMailInfo *info, QWidget *parent = Q_NULLPTR);
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

private Q_SLOTS:
    void slotFolderChanged(const Akonadi::Collection &);
    void slotUpdateOkButton();

private:
    void load(ArchiveMailInfo *info);
    MailCommon::FolderRequester *mFolderRequester;
    FormatComboBox *mFormatComboBox;
    UnitComboBox *mUnits;
    QCheckBox *mRecursiveCheckBox;
    KUrlRequester *mPath;
    QSpinBox *mDays;
    QSpinBox *mMaximumArchive;

    ArchiveMailInfo *mInfo;
    QPushButton *mOkButton;
};

#endif // ADDARCHIVEMAILDIALOG_H
