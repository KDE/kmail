/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "addarchivemailwidget.h"

#include "archivemailrangewidget.h"
#include "widgets/formatcombobox.h"
#include "widgets/unitcombobox.h"
#include <KLineEdit>
#include <KLocalizedString>
#include <KSeparator>
#include <KUrlRequester>
#include <MailCommon/FolderRequester>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

using namespace Qt::Literals::StringLiterals;
AddArchiveMailWidget::AddArchiveMailWidget(ArchiveMailInfo *info, QWidget *parent)
    : QWidget{parent}
    , mFolderRequester(new MailCommon::FolderRequester(this))
    , mFormatComboBox(new FormatComboBox(this))
    , mUnits(new UnitComboBox(this))
    , mRecursiveCheckBox(new QCheckBox(i18nc("@option:check", "Archive all subfolders"), this))
    , mPath(new KUrlRequester(this))
    , mDays(new QSpinBox(this))
    , mMaximumArchive(new QSpinBox(this))
    , mArchiveMailRangeWidget(new ArchiveMailRangeWidget(this))
    , mInfo(info)
{
    auto mainLayout = new QFormLayout(this);
    mainLayout->setContentsMargins({});

    auto folderLabel = new QLabel(i18nc("@label:textbox", "Folder:"), this);
    mFolderRequester->setObjectName("folder_requester"_L1);
    mFolderRequester->setMustBeReadWrite(false);
    mFolderRequester->setNotAllowToCreateNewFolder(true);
    mainLayout->addRow(folderLabel, mFolderRequester);
    connect(mFolderRequester, &MailCommon::FolderRequester::folderChanged, this, &AddArchiveMailWidget::slotFolderChanged);
    if (info) { // Don't authorize to modify folder when we just modify item.
        mFolderRequester->setEnabled(false);
    }

    auto formatLabel = new QLabel(i18nc("@label:textbox", "Format:"), this);
    formatLabel->setObjectName("label_format"_L1);
    mainLayout->addRow(formatLabel, mFormatComboBox);

    mRecursiveCheckBox->setObjectName("recursive_checkbox"_L1);
    mRecursiveCheckBox->setChecked(true);
    mainLayout->addWidget(mRecursiveCheckBox);

    auto pathLabel = new QLabel(i18nc("@label:textbox", "Path:"), this);
    pathLabel->setObjectName("path_label"_L1);
    mPath->lineEdit()->setTrapReturnKey(true);
    connect(mPath, &KUrlRequester::textChanged, this, &AddArchiveMailWidget::slotUpdateOkButton);
    mPath->setMode(KFile::Directory);
    mainLayout->addRow(pathLabel, mPath);

    auto dateLabel = new QLabel(i18nc("@label:textbox", "Backup each:"), this);
    dateLabel->setObjectName("date_label"_L1);

    auto hlayout = new QHBoxLayout;
    mDays->setMinimum(1);
    mDays->setMaximum(3600);
    hlayout->addWidget(mDays);

    hlayout->addWidget(mUnits);

    mainLayout->addRow(dateLabel, hlayout);

    auto maxCountlabel = new QLabel(i18nc("@label:textbox", "Maximum number of archive:"), this);
    mMaximumArchive->setMinimum(0);
    mMaximumArchive->setMaximum(9999);
    mMaximumArchive->setSpecialValueText(i18n("unlimited"));
    maxCountlabel->setBuddy(mMaximumArchive);
    mainLayout->addRow(maxCountlabel, mMaximumArchive);

    mArchiveMailRangeWidget->setObjectName("mArchiveMailRangeWidget"_L1);

    mainLayout->addRow(mArchiveMailRangeWidget);
    if (mInfo) {
        load(mInfo);
    }
}

AddArchiveMailWidget::~AddArchiveMailWidget() = default;

void AddArchiveMailWidget::load(ArchiveMailInfo *info)
{
    mPath->setUrl(info->url());
    mRecursiveCheckBox->setChecked(info->saveSubCollection());
    mFolderRequester->setCollection(Akonadi::Collection(info->saveCollectionId()));
    mFormatComboBox->setFormat(info->archiveType());
    mDays->setValue(info->archiveAge());
    mUnits->setUnit(info->archiveUnit());
    mMaximumArchive->setValue(info->maximumArchiveCount());
    const bool useRange{info->useRange()};
    mArchiveMailRangeWidget->setRangeEnabled(useRange);
    if (useRange) {
        mArchiveMailRangeWidget->setRange(info->range());
    }
}

ArchiveMailInfo *AddArchiveMailWidget::info()
{
    if (!mInfo) {
        mInfo = new ArchiveMailInfo();
    }
    mInfo->setSaveSubCollection(mRecursiveCheckBox->isChecked());
    mInfo->setArchiveType(mFormatComboBox->format());
    mInfo->setSaveCollectionId(mFolderRequester->collection().id());
    mInfo->setUrl(mPath->url());
    mInfo->setArchiveAge(mDays->value());
    mInfo->setArchiveUnit(mUnits->unit());
    mInfo->setMaximumArchiveCount(mMaximumArchive->value());
    const bool isRangeEnabled = mArchiveMailRangeWidget->isRangeEnabled();
    mInfo->setUseRange(isRangeEnabled);
    if (isRangeEnabled) {
        mInfo->setRange(mArchiveMailRangeWidget->range());
    }
    return mInfo;
}

void AddArchiveMailWidget::slotUpdateOkButton()
{
    const bool valid = (!mPath->lineEdit()->text().trimmed().isEmpty() && !mPath->url().isEmpty() && mFolderRequester->collection().isValid());
    Q_EMIT enableOkButton(valid);
}

void AddArchiveMailWidget::slotFolderChanged(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection)
    slotUpdateOkButton();
}

#include "moc_addarchivemailwidget.cpp"
