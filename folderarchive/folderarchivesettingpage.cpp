/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "folderarchivesettingpage.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchiveutil.h"
#include "mailcommon/folder/folderrequester.h"

#include <KLocale>
#include <KGlobal>
#include <KSharedConfig>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>

FolderArchiveComboBox::FolderArchiveComboBox(QWidget *parent)
    : QComboBox(parent)
{
    initialize();
}

FolderArchiveComboBox::~FolderArchiveComboBox()
{
}

void FolderArchiveComboBox::initialize()
{
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Unique"),
            FolderArchiveAccountInfo::UniqueFolder);
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Month and year"),
            FolderArchiveAccountInfo::FolderByMonths);
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Year"),
            FolderArchiveAccountInfo::FolderByYears);
}

void FolderArchiveComboBox::setType(FolderArchiveAccountInfo::FolderArchiveType type)
{
    const int index = findData(static_cast<int>(type));
    if (index != -1) {
        setCurrentIndex(index);
    } else {
        setCurrentIndex(0);
    }
}

FolderArchiveAccountInfo::FolderArchiveType FolderArchiveComboBox::type() const
{
    return static_cast<FolderArchiveAccountInfo::FolderArchiveType>(itemData(currentIndex()).toInt());
}

FolderArchiveSettingPage::FolderArchiveSettingPage(const QString &instanceName, QWidget *parent)
    : QWidget(parent),
      mInstanceName(instanceName),
      mInfo(0)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mEnabled = new QCheckBox(i18n("Enable"));
    connect(mEnabled, SIGNAL(toggled(bool)), this, SLOT(slotEnableChanged(bool)));
    lay->addWidget(mEnabled);

    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *lab = new QLabel(i18nc(
        "@label:chooser for the folder that messages will be archived under",
        "Archive into:"));
    hbox->addWidget(lab);
    mArchiveFolder = new MailCommon::FolderRequester;
    hbox->addWidget(mArchiveFolder);
    lay->addLayout(hbox);

    hbox = new QHBoxLayout;
    lab = new QLabel(i18nc("@label:listbox", "Archive folder name:"));
    hbox->addWidget(lab);
    mArchiveNamed = new FolderArchiveComboBox;
    hbox->addWidget(mArchiveNamed);

    lay->addLayout(hbox);

    lay->addStretch();

    setLayout(lay);
}

FolderArchiveSettingPage::~FolderArchiveSettingPage()
{
    delete mInfo;
}

void FolderArchiveSettingPage::slotEnableChanged(bool enabled)
{
    mArchiveFolder->setEnabled(enabled);
    mArchiveNamed->setEnabled(enabled);
}

void FolderArchiveSettingPage::loadSettings()
{
    KSharedConfig::Ptr config = KGlobal::config();
    const QString groupName = FolderArchive::FolderArchiveUtil::groupConfigPattern() + mInstanceName;
    if (config->hasGroup(groupName)) {
        KConfigGroup grp = config->group(groupName);
        mInfo = new FolderArchiveAccountInfo(grp);
        mEnabled->setChecked(mInfo->enabled());
        mArchiveFolder->setCollection(Akonadi::Collection(mInfo->archiveTopLevel()));
        mArchiveNamed->setType(mInfo->folderArchiveType());
    } else {
        mInfo = new FolderArchiveAccountInfo();
        mEnabled->setChecked(false);
    }
    slotEnableChanged(mEnabled->isChecked());
}

void FolderArchiveSettingPage::writeSettings()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup grp = config->group(FolderArchive::FolderArchiveUtil::groupConfigPattern() + mInstanceName);
    mInfo->setInstanceName(mInstanceName);
    if (mArchiveFolder->collection().isValid()) {
        mInfo->setEnabled(mEnabled->isChecked());
        mInfo->setArchiveTopLevel(mArchiveFolder->collection().id());
    } else {
        mInfo->setEnabled(false);
        mInfo->setArchiveTopLevel(-1);
    }

    mInfo->setFolderArchiveType(mArchiveNamed->type());
    mInfo->writeConfig(grp);
}

