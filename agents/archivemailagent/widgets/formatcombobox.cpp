/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "formatcombobox.h"
#include <KLocalizedString>

FormatComboBox::FormatComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // These combobox values have to stay in sync with the ArchiveType enum from BackupJob!
    addItem(i18n("Compressed Zip Archive (.zip)"), static_cast<int>(MailCommon::BackupJob::Zip));
    addItem(i18n("Uncompressed Archive (.tar)"), static_cast<int>(MailCommon::BackupJob::Tar));
    addItem(i18n("BZ2-Compressed Tar Archive (.tar.bz2)"), static_cast<int>(MailCommon::BackupJob::TarBz2));
    addItem(i18n("GZ-Compressed Tar Archive (.tar.gz)"), static_cast<int>(MailCommon::BackupJob::TarGz));
    setCurrentIndex(findData(static_cast<int>(MailCommon::BackupJob::TarBz2)));
}

FormatComboBox::~FormatComboBox() = default;

void FormatComboBox::setFormat(MailCommon::BackupJob::ArchiveType type)
{
    const int index = findData(static_cast<int>(type));
    if (index != -1) {
        setCurrentIndex(index);
    } else {
        setCurrentIndex(0);
    }
}

MailCommon::BackupJob::ArchiveType FormatComboBox::format() const
{
    return static_cast<MailCommon::BackupJob::ArchiveType>(itemData(currentIndex()).toInt());
}
