/*
   SPDX-FileCopyrightText: 2009-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "collectionviewpage.h"
#include <MailCommon/MailKernel>

#include <Akonadi/KMime/MessageFolderAttribute>
#include <AkonadiCore/entitydisplayattribute.h>

#include "kmail_debug.h"
#include <KIconButton>
#include <KLocalizedString>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <MessageList/AggregationComboBox>
#include <MessageList/AggregationConfigButton>
#include <MessageList/ThemeComboBox>
#include <MessageList/ThemeConfigButton>

#include <MailCommon/CollectionViewWidget>

using namespace MailCommon;

CollectionViewPage::CollectionViewPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setObjectName(QStringLiteral("KMail::CollectionViewPage"));
    setPageTitle(i18nc("@title:tab View settings for a folder.", "View"));
}

CollectionViewPage::~CollectionViewPage() = default;

void CollectionViewPage::init(const Akonadi::Collection &col)
{
    mFolderCollection = FolderSettings::forCollection(col);
    mIsLocalSystemFolder = CommonKernel->isSystemFolderCollection(col) || mFolderCollection->isStructural() || Kernel::folderIsInbox(col);

    auto topLayout = new QVBoxLayout(this);
    // Musn't be able to edit details for non-resource, system folder.
    if (!mIsLocalSystemFolder) {
        // icons
        mIconsCheckBox = new QCheckBox(i18n("Use custom &icons"), this);
        mIconsCheckBox->setChecked(false);

        mNormalIconLabel = new QLabel(i18nc("Icon used for folders with no unread messages.", "&Normal:"), this);
        mNormalIconLabel->setEnabled(false);

        mNormalIconButton = new KIconButton(this);
        mNormalIconLabel->setBuddy(mNormalIconButton);
        mNormalIconButton->setIconType(KIconLoader::NoGroup, KIconLoader::Place, false);
        mNormalIconButton->setIconSize(16);
        mNormalIconButton->setStrictIconSize(true);
        mNormalIconButton->setFixedSize(28, 28);
        // Can't use iconset here.
        mNormalIconButton->setIcon(QStringLiteral("folder"));
        mNormalIconButton->setEnabled(false);

        mUnreadIconLabel = new QLabel(i18nc("Icon used for folders which do have unread messages.", "&Unread:"), this);
        mUnreadIconLabel->setEnabled(false);

        mUnreadIconButton = new KIconButton(this);
        mUnreadIconLabel->setBuddy(mUnreadIconButton);
        mUnreadIconButton->setIconType(KIconLoader::NoGroup, KIconLoader::Place, false);
        mUnreadIconButton->setIconSize(16);
        mUnreadIconButton->setStrictIconSize(true);
        mUnreadIconButton->setFixedSize(28, 28);
        // Can't use iconset here.
        mUnreadIconButton->setIcon(QStringLiteral("folder-open"));
        mUnreadIconButton->setEnabled(false);

        auto iconHLayout = new QHBoxLayout();
        iconHLayout->addWidget(mIconsCheckBox);
        iconHLayout->addStretch(2);
        iconHLayout->addWidget(mNormalIconLabel);
        iconHLayout->addWidget(mNormalIconButton);
        iconHLayout->addWidget(mUnreadIconLabel);
        iconHLayout->addWidget(mUnreadIconButton);
        iconHLayout->addStretch(1);
        topLayout->addLayout(iconHLayout);

        connect(mIconsCheckBox, &QCheckBox::toggled, mNormalIconLabel, &QLabel::setEnabled);
        connect(mIconsCheckBox, &QCheckBox::toggled, mNormalIconButton, &KIconButton::setEnabled);
        connect(mIconsCheckBox, &QCheckBox::toggled, mUnreadIconButton, &KIconButton::setEnabled);
        connect(mIconsCheckBox, &QCheckBox::toggled, mUnreadIconLabel, &QLabel::setEnabled);

        connect(mNormalIconButton, &KIconButton::iconChanged, this, &CollectionViewPage::slotChangeIcon);
    }

    mCollectionViewWidget = new MailCommon::CollectionViewWidget(this);
    topLayout->addWidget(mCollectionViewWidget);

    topLayout->addStretch(100);
}

void CollectionViewPage::slotChangeIcon(const QString &icon)
{
    mUnreadIconButton->setIcon(icon);
}

void CollectionViewPage::load(const Akonadi::Collection &col)
{
    init(col);
    if (!mIsLocalSystemFolder) {
        QString iconName;
        QString unreadIconName;
        bool iconWasEmpty = false;
        if (col.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
            iconName = col.attribute<Akonadi::EntityDisplayAttribute>()->iconName();
            unreadIconName = col.attribute<Akonadi::EntityDisplayAttribute>()->activeIconName();
        }

        if (iconName.isEmpty()) {
            iconName = QStringLiteral("folder");
            iconWasEmpty = true;
        }
        mNormalIconButton->setIcon(iconName);

        if (unreadIconName.isEmpty()) {
            mUnreadIconButton->setIcon(iconName);
        } else {
            mUnreadIconButton->setIcon(unreadIconName);
        }

        mIconsCheckBox->setChecked(!iconWasEmpty);
    }
    mCollectionViewWidget->load(col);
}

void CollectionViewPage::save(Akonadi::Collection &col)
{
    if (!mIsLocalSystemFolder) {
        if (mIconsCheckBox->isChecked()) {
            col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing)->setIconName(mNormalIconButton->icon());
            col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing)->setActiveIconName(mUnreadIconButton->icon());
        } else if (col.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
            col.attribute<Akonadi::EntityDisplayAttribute>()->setIconName(QString());
            col.attribute<Akonadi::EntityDisplayAttribute>()->setActiveIconName(QString());
        }
    }
    mCollectionViewWidget->save(col);
}
