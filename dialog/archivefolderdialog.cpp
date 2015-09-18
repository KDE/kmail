/* Copyright 2009 Klarälvdalens Datakonsult AB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "archivefolderdialog.h"

#include "MailCommon/BackupJob"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "MailCommon/FolderRequester"
#include "MessageViewer/MessageViewerUtil"

#include <AkonadiCore/Collection>

#include <KLocalizedString>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kmessagebox.h>
#include <KSeparator>

#include <qlabel.h>
#include <qcheckbox.h>
#include <QGridLayout>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace KMail;
using namespace MailCommon;

QString ArchiveFolderDialog::standardArchivePath(const QString &folderName)
{
    QString currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(currentPath);
    if (!dir.exists()) {
        currentPath = QDir::homePath();
    }
    return currentPath + QLatin1Char('/') +
           i18nc("Start of the filename for a mail archive file" , "Archive") + QLatin1Char('_') + folderName + QLatin1Char('_') + QDate::currentDate().toString(Qt::ISODate) + QLatin1String(".tar.bz2");
}

ArchiveFolderDialog::ArchiveFolderDialog(QWidget *parent)
    : QDialog(parent), mParentWidget(parent)
{
    setObjectName(QStringLiteral("archive_folder_dialog"));
    setWindowTitle(i18nc("@title:window for archiving a folder", "Archive Folder"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout *topLayout = new QVBoxLayout;
    setLayout(topLayout);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ArchiveFolderDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ArchiveFolderDialog::reject);
    mOkButton->setDefault(true);
    mOkButton->setText(i18nc("@action", "Archive"));
    setModal(true);
    QWidget *mainWidget = new QWidget(this);
    topLayout->addWidget(mainWidget);
    topLayout->addWidget(buttonBox);
    QGridLayout *mainLayout = new QGridLayout(mainWidget);

    int row = 0;

    // TODO: Explaination label

    QLabel *folderLabel = new QLabel(i18n("&Folder:"), mainWidget);
    mainLayout->addWidget(folderLabel, row, 0);
    mFolderRequester = new FolderRequester(mainWidget);
    mFolderRequester->setMustBeReadWrite(false);
    mFolderRequester->setNotAllowToCreateNewFolder(true);
    connect(mFolderRequester, &FolderRequester::folderChanged, this, &ArchiveFolderDialog::slotFolderChanged);
    folderLabel->setBuddy(mFolderRequester);
    mainLayout->addWidget(mFolderRequester, row, 1);
    row++;

    QLabel *formatLabel = new QLabel(i18n("F&ormat:"), mainWidget);
    mainLayout->addWidget(formatLabel, row, 0);
    mFormatComboBox = new KComboBox(mainWidget);
    formatLabel->setBuddy(mFormatComboBox);

    // These combobox values have to stay in sync with the ArchiveType enum from BackupJob!
    mFormatComboBox->addItem(i18n("Compressed Zip Archive (.zip)"));
    mFormatComboBox->addItem(i18n("Uncompressed Archive (.tar)"));
    mFormatComboBox->addItem(i18n("BZ2-Compressed Tar Archive (.tar.bz2)"));
    mFormatComboBox->addItem(i18n("GZ-Compressed Tar Archive (.tar.gz)"));
    mFormatComboBox->setCurrentIndex(2);
    connect(mFormatComboBox, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &ArchiveFolderDialog::slotFixFileExtension);
    mainLayout->addWidget(mFormatComboBox, row, 1);
    row++;

    QLabel *fileNameLabel = new QLabel(i18n("&Archive File:"), mainWidget);
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
    const QSharedPointer<FolderCollection> folder = FolderCollection::forCollection(col, false);
    return folder
           && col.isValid()
           && !col.isVirtual()
           && (col.rights() & Akonadi::Collection::CanDeleteCollection)
           && !folder->isStructural()
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
    mUrlRequester->setUrl(standardArchivePath(defaultCollection.name()));
    const QSharedPointer<FolderCollection> folder = FolderCollection::forCollection(defaultCollection, false);
    mDeleteCheckBox->setEnabled(allowToDeleteFolders(defaultCollection));
    mOkButton->setEnabled(defaultCollection.isValid() && folder && !folder->isStructural());
}

void ArchiveFolderDialog::slotAccepted()
{
    if (!MessageViewer::Util::checkOverwrite(mUrlRequester->url(), this)) {
        return;
    }

    if (!mFolderRequester->hasCollection()) {
        KMessageBox::information(this, i18n("Please select the folder that should be archived."),
                                 i18n("No folder selected"));
        return;
    }

    MailCommon::BackupJob *backupJob = new MailCommon::BackupJob(mParentWidget);
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
    const char *extensions[numExtensions] = { ".zip", ".tar", ".tar.bz2", ".tar.gz" };

    QString fileName = mUrlRequester->url().path();
    if (fileName.isEmpty())
        fileName = standardArchivePath(mFolderRequester->hasCollection() ?
                                       mFolderRequester->collection().name() : QString());

    QMimeDatabase db;
    const QString extension = db.suffixForFileName(fileName);
    if (!extension.isEmpty()) {
        fileName = fileName.left(fileName.length() - extension.length() - 1);
    }

    // Now, we've got a filename without an extension, simply append the correct one
    fileName += QLatin1String(extensions[mFormatComboBox->currentIndex()]);
    mUrlRequester->setUrl(fileName);
}

void ArchiveFolderDialog::slotUrlChanged(const QString &url)
{
    mOkButton->setEnabled(!url.isEmpty());
}

