/*
   SPDX-FileCopyrightText: 2009 Klarälvdalens Datakonsult AB

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QDialog>

class QCheckBox;
class KUrlRequester;
class QComboBox;
class QPushButton;
namespace Akonadi
{
class Collection;
}

namespace MailCommon
{
class FolderRequester;
}

namespace KMail
{
class ArchiveFolderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ArchiveFolderDialog(QWidget *parent = nullptr);
    void setFolder(const Akonadi::Collection &defaultCollection);

private:
    void slotFixFileExtension();
    void slotFolderChanged(const Akonadi::Collection &);
    void slotRecursiveCheckboxClicked();
    void slotAccepted();
    void slotUrlChanged(const QString &);

    Q_REQUIRED_RESULT bool allowToDeleteFolders(const Akonadi::Collection &folder) const;
    Q_REQUIRED_RESULT QString standardArchivePath(const QString &folderName);

    QWidget *const mParentWidget;
    QCheckBox *mDeleteCheckBox = nullptr;
    QCheckBox *mRecursiveCheckBox = nullptr;
    MailCommon::FolderRequester *mFolderRequester = nullptr;
    QComboBox *mFormatComboBox = nullptr;
    KUrlRequester *mUrlRequester = nullptr;
    QPushButton *mOkButton = nullptr;
};
}

