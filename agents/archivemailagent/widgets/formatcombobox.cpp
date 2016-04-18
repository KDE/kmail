/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

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

#include "formatcombobox.h"
#include <KLocalizedString>

FormatComboBox::FormatComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // These combobox values have to stay in sync with the ArchiveType enum from BackupJob!
    addItem(i18n("Compressed Zip Archive (.zip)"), (int)MailCommon::BackupJob::Zip);
    addItem(i18n("Uncompressed Archive (.tar)"), (int)MailCommon::BackupJob::Tar);
    addItem(i18n("BZ2-Compressed Tar Archive (.tar.bz2)"), (int)MailCommon::BackupJob::TarBz2);
    addItem(i18n("GZ-Compressed Tar Archive (.tar.gz)"), (int)MailCommon::BackupJob::TarGz);
    setCurrentIndex(findData((int)MailCommon::BackupJob::TarBz2));
}

FormatComboBox::~FormatComboBox()
{

}

void FormatComboBox::setFormat(MailCommon::BackupJob::ArchiveType type)
{
    const int index = findData((int)type);
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

