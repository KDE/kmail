/*
   SPDX-FileCopyrightText: 2009 Klarälvdalens Datakonsult AB
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "archivefolderdialog.h"

#include "kmkernel.h"
#include "kmmainwidget.h"
#include <MailCommon/BackupJob>
#include <MailCommon/FolderRequester>
#include <MessageViewer/MessageViewerUtil>

#include <AkonadiCore/Collection>

#include <KLocalizedString>
#include <KMessageBox>
#include <KSeparator>
#include <KUrlRequester>
#include <QComboBox>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMimeDatabase>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

using namespace KMail;
using namespace MailCommon;

QString ArchiveFolderDialog::standardArchivePath(const QString &folderName)
{
    QString currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QDir dir(currentPath);
    if (!dir.exists()) {
        currentPath = QDir::homePath();
    }
    return currentPath + QLatin1Char('/') + i18nc("Start of the filename for a mail archive file", "Archive") + QLatin1Char('_') + folderName + QLatin1Char('_')
        + QDate::currentDate().toString(Qt::ISODate) + QLatin1String(".tar.bz2");
}

ArchiveFolderDialog::ArchiveFolderDialog(QWidget *parent)
    : QDialog(parent)
    , mParentWidget(parent)
{
    setObjectName(QStringLiteral("archive_folder_dialog"));
    setWindowTitle(i18nc("@title:window for archiving a folder", "Archive Folder"));
    auto topLayout = new QVBoxLayout(this);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ArchiveFolderDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ArchiveFolderDialog::reject);
    mOkButton->setDefault(true);
    mOkButton->setText(i18nc("@action", "Archive"));
    setModal(true);
    auto mainWidget = new QWidget(this);
    topLayout->addWidget(mainWidget);
    topLayout->addWidget(buttonBox);
    auto mainLayout = new QGridLayout(mainWidget);
    mainLayout->setContentsMargins({});

    int row = 0;

    // TODO: Explanation label

    auto folderLabel = new QLabel(i18n("&Folder:"), mainWidget);
    mainLayout->addWidget(folderLabel, row, 0);
    mFolderRequester = new FolderRequester(mainWidget);
    mFolderRequester->setMustBeReadWrite(false);
    mFolderRequester->setNotAllowToCreateNewFolder(true);
    connect(mFolderRequester, &FolderRequester::folderChanged, this, &ArchiveFolderDialog::slotFolderChanged);
    folderLabel->setBuddy(mFolderRequester);
    mainLayout->addWidget(mFolderRequester, row, 1);
    row++;

    auto formatLabel = new QLabel(i18n("F&ormat:"), mainWidget);
    mainLayout->addWidget(formatLabel, row, 0);
    mFormatComboBox = new QComboBox(mainWidget);
    formatLabel->setBuddy(mFormatComboBox);

    // These combobox values have to stay in sync with the ArchiveType enum from BackupJob!
    mFormatComboBox->addItem(i18n("Compressed Zip Archive (.zip)"));
    mFormatComboBox->addItem(i18n("Uncompressed Archive (.tar)"));
    mFormatComboBox->addItem(i18n("BZ2-Compressed Tar Archive (.tar.bz2)"));
    mFormatComboBox->addItem(i18n("GZ-Compressed Tar Archive (.tar.gz)"));
    mFormatComboBox->setCurrentIndex(2);
    connect(mFormatComboBox, qOverload<int>(&QComboBox::activated), this, &ArchiveFolderDialog::slotFixFileExtension);
    mainLayout->addWidget(mFormatComboBox, row, 1);
    row++;

    auto fileNameLabel = new QLabel(i18n("&Archive File:"), mainWidget);
    mainLayout->addWidget(fileNameLabel, row, 0);
    mUrlRequester = new KUrlRequester(mainWidget);
    mUrlRequester->setMode(KFile::LocalOnly | KFile::File);
    mUrlRequester->setFilter(QStringLiteral("*.tar *.zip *.tar.gz *.tar.bz2"));
    fileNameLabel->setBuddy(mUrlRequester);
    connect(mUrlRequester, &KUrlRequester::urlSelected, this, &ArchiveFolderDialog::slotFixFileExtension);
    connect(mUrlRequester, &KUrlRequester::textChanged, this, &ArchiveFolderDialog::slotUrlChanged);
    mainLayout->addWidget(mUrlRequester, row, 1);
    row++;

    // TODO: Make this appear more dangerous!
    mDeleteCheckBox = new QCheckBox(i18n("&Delete folder and subfolders after completion"), mainWidget);
    mainLayout->addWidget(mDeleteCheckBox, row, 0, 1, 2, Qt::AlignLeft);
    row++;

    mRecursiveCheckBox = new QCheckBox(i18n("Archive all subfolders"), mainWidget);
    connect(mRecursiveCheckBox, &QCheckBox::clicked, this, &ArchiveFolderDialog::slotRecursiveCheckboxClicked);
    mainLayout->addWidget(mRecursiveCheckBox, row, 0, 1, 2, Qt::AlignLeft);
    mRecursiveCheckBox->setChecked(true);
    row++;

    // TODO: what's this, tooltips

    // TODO: Warn that user should do mail check for online IMAP and possibly cached IMAP as well

    mainLayout->addWidget(new KSeparator(), row, 0, 1, 2);
    row++;
    mainLayout->setColumnStretch(1, 1);
    mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), row, 0);

    // Make it a bit bigger, else the folder requester cuts off the text too early
    resize(500, minimumSize().height());
}

bool canRemoveFolder(const Akonadi::Collection &col)
{
    const QSharedPointer<FolderSettings> folder = FolderSettings::forCollection(col, false);
    return folder && col.isValid() && !col.isVirtual() && (col.rights() & Akonadi::Collection::CanDeleteCollection) && !folder->isStructural()
        && !folder->isSystemFolder();
}

void ArchiveFolderDialog::slotRecursiveCheckboxClicked()
{
    slotFolderChanged(mFolderRequester->collection());
}

void ArchiveFolderDialog::slotFolderChanged(const Akonadi::Collection &folder)
{
    mDeleteCheckBox->setEnabled(allowToDeleteFolders(folder));
}

bool ArchiveFolderDialog::allowToDeleteFolders(const Akonadi::Collection &folder) const
{
    return canRemoveFolder(folder) && mRecursiveCheckBox->isChecked();
}

void ArchiveFolderDialog::setFolder(const Akonadi::Collection &defaultCollection)
{
    mFolderRequester->setCollection(defaultCollection);
    // TODO: what if the file already exists?
    mUrlRequester->setUrl(QUrl::fromLocalFile(standardArchivePath(defaultCollection.name())));
    const QSharedPointer<FolderSettings> folder = FolderSettings::forCollection(defaultCollection, false);
    mDeleteCheckBox->setEnabled(allowToDeleteFolders(defaultCollection));
    mOkButton->setEnabled(defaultCollection.isValid() && folder && !folder->isStructural());
}

void ArchiveFolderDialog::slotAccepted()
{
    if (!MessageViewer::Util::checkOverwrite(mUrlRequester->url(), this)) {
        return;
    }

    if (!mFolderRequester->hasCollection()) {
        KMessageBox::information(this, i18n("Please select the folder that should be archived."), i18n("No folder selected"));
        return;
    }

    auto backupJob = new MailCommon::BackupJob(mParentWidget);
    backupJob->setRootFolder(mFolderRequester->collection());
    backupJob->setSaveLocation(mUrlRequester->url());
    backupJob->setArchiveType(static_cast<BackupJob::ArchiveType>(mFormatComboBox->currentIndex()));
    backupJob->setDeleteFoldersAfterCompletion(mDeleteCheckBox->isEnabled() && mDeleteCheckBox->isChecked());
    backupJob->setRecursive(mRecursiveCheckBox->isChecked());
    backupJob->start();
    accept();
}

void ArchiveFolderDialog::slotFixFileExtension()
{
    const int numExtensions = 4;
    // The extensions here are also sorted, like the enum order of BackupJob::ArchiveType
    const char *extensions[numExtensions] = {".zip", ".tar", ".tar.bz2", ".tar.gz"};

    QString fileName = mUrlRequester->url().path();
    if (fileName.isEmpty()) {
        fileName = standardArchivePath(mFolderRequester->hasCollection() ? mFolderRequester->collection().name() : QString());
    }

    QMimeDatabase db;
    const QString extension = db.suffixForFileName(fileName);
    if (!extension.isEmpty()) {
        fileName.truncate(fileName.length() - extension.length() - 1);
    }

    // Now, we've got a filename without an extension, simply append the correct one
    fileName += QLatin1String(extensions[mFormatComboBox->currentIndex()]);
    mUrlRequester->setUrl(QUrl::fromLocalFile(fileName));
}

void ArchiveFolderDialog::slotUrlChanged(const QString &url)
{
    mOkButton->setEnabled(!url.isEmpty());
}
