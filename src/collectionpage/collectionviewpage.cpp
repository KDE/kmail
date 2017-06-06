/*
   Copyright (C) 2009-2017 Montel Laurent <montel@kde.org>

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

#include "collectionviewpage.h"
#include "mailcommon/mailkernel.h"

#include <AkonadiCore/collection.h>
#include <AkonadiCore/entitydisplayattribute.h>
#include <Akonadi/KMime/MessageFolderAttribute>

#include <QGroupBox>
#include <QHBoxLayout>
#include <kicondialog.h>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <KLocalizedString>
#include <KIconButton>
#include <KConfigGroup>
#include <QRadioButton>
#include "kmail_debug.h"

#include "messagelist/aggregationcombobox.h"
#include "messagelist/aggregationconfigbutton.h"
#include "messagelist/themecombobox.h"
#include "messagelist/themeconfigbutton.h"

#include <MailCommon/FolderSettings>

using namespace MailCommon;

CollectionViewPage::CollectionViewPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
    , mIsLocalSystemFolder(false)
{
    setObjectName(QStringLiteral("KMail::CollectionViewPage"));
    setPageTitle(i18nc("@title:tab View settings for a folder.", "View"));
}

CollectionViewPage::~CollectionViewPage()
{
}

void CollectionViewPage::init(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
    mFolderCollection = FolderSettings::forCollection(col);
    mIsLocalSystemFolder = CommonKernel->isSystemFolderCollection(col) || mFolderCollection->isStructural() || Kernel::folderIsInbox(col);

    QVBoxLayout *topLayout = new QVBoxLayout(this);
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

        QHBoxLayout *iconHLayout = new QHBoxLayout();
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

    // sender or receiver column
    const QString senderReceiverColumnTip = i18n("Show Sender/Receiver Column in List of Messages");

    QLabel *senderReceiverColumnLabel = new QLabel(i18n("Sho&w column:"), this);
    mShowSenderReceiverComboBox = new KComboBox(this);
    mShowSenderReceiverComboBox->setToolTip(senderReceiverColumnTip);
    senderReceiverColumnLabel->setBuddy(mShowSenderReceiverComboBox);
    mShowSenderReceiverComboBox->insertItem(0, i18nc("@item:inlistbox Show default value.", "Default"));
    mShowSenderReceiverComboBox->insertItem(1, i18nc("@item:inlistbox Show sender.", "Sender"));
    mShowSenderReceiverComboBox->insertItem(2, i18nc("@item:inlistbox Show receiver.", "Receiver"));

    QHBoxLayout *senderReceiverColumnHLayout = new QHBoxLayout();
    senderReceiverColumnHLayout->addWidget(senderReceiverColumnLabel);
    senderReceiverColumnHLayout->addWidget(mShowSenderReceiverComboBox);
    topLayout->addLayout(senderReceiverColumnHLayout);

    // message list
    QGroupBox *messageListGroup = new QGroupBox(i18n("Message List"), this);
    QVBoxLayout *messageListGroupLayout = new QVBoxLayout(messageListGroup);
    topLayout->addWidget(messageListGroup);

    // message list aggregation
    mUseDefaultAggregationCheckBox = new QCheckBox(i18n("Use default aggregation"), messageListGroup);
    messageListGroupLayout->addWidget(mUseDefaultAggregationCheckBox);
    connect(mUseDefaultAggregationCheckBox, &QCheckBox::stateChanged, this, &CollectionViewPage::slotAggregationCheckboxChanged);

    mAggregationComboBox = new MessageList::Utils::AggregationComboBox(messageListGroup);

    QLabel *aggregationLabel = new QLabel(i18n("Aggregation"), messageListGroup);
    aggregationLabel->setBuddy(mAggregationComboBox);

    using MessageList::Utils::AggregationConfigButton;
    AggregationConfigButton *aggregationConfigButton = new AggregationConfigButton(messageListGroup, mAggregationComboBox);
    // Make sure any changes made in the aggregations configure dialog are reflected in the combo.
    connect(aggregationConfigButton, &AggregationConfigButton::configureDialogCompleted, this, &CollectionViewPage::slotSelectFolderAggregation);

    QHBoxLayout *aggregationLayout = new QHBoxLayout();
    aggregationLayout->addWidget(aggregationLabel, 1);
    aggregationLayout->addWidget(mAggregationComboBox, 1);
    aggregationLayout->addWidget(aggregationConfigButton, 0);
    messageListGroupLayout->addLayout(aggregationLayout);

    // message list theme
    mUseDefaultThemeCheckBox = new QCheckBox(i18n("Use default theme"), messageListGroup);
    messageListGroupLayout->addWidget(mUseDefaultThemeCheckBox);
    connect(mUseDefaultThemeCheckBox, &QCheckBox::stateChanged, this, &CollectionViewPage::slotThemeCheckboxChanged);

    mThemeComboBox = new MessageList::Utils::ThemeComboBox(messageListGroup);

    QLabel *themeLabel = new QLabel(i18n("Theme"), messageListGroup);
    themeLabel->setBuddy(mThemeComboBox);

    using MessageList::Utils::ThemeConfigButton;
    ThemeConfigButton *themeConfigButton = new ThemeConfigButton(messageListGroup, mThemeComboBox);
    // Make sure any changes made in the themes configure dialog are reflected in the combo.
    connect(themeConfigButton, &ThemeConfigButton::configureDialogCompleted, this, &CollectionViewPage::slotSelectFolderTheme);

    QHBoxLayout *themeLayout = new QHBoxLayout();
    themeLayout->addWidget(themeLabel, 1);
    themeLayout->addWidget(mThemeComboBox, 1);
    themeLayout->addWidget(themeConfigButton, 0);
    messageListGroupLayout->addLayout(themeLayout);

    // Message Default Format
    QGroupBox *messageFormatGroup = new QGroupBox(i18n("Message Default Format"), this);
    QVBoxLayout *messageFormatGroupLayout = new QVBoxLayout(messageFormatGroup);
    mPreferHtmlToText = new QRadioButton(i18n("Prefer HTML to text"), this);
    messageFormatGroupLayout->addWidget(mPreferHtmlToText);
    mPreferTextToHtml = new QRadioButton(i18n("Prefer text to HTML"), this);
    messageFormatGroupLayout->addWidget(mPreferTextToHtml);
    mUseGlobalSettings = new QRadioButton(i18n("Use Global Settings"), this);
    messageFormatGroupLayout->addWidget(mUseGlobalSettings);

    topLayout->addWidget(messageFormatGroup);

    topLayout->addStretch(100);
}

void CollectionViewPage::slotChangeIcon(const QString &icon)
{
    mUnreadIconButton->setIcon(icon);
}

void CollectionViewPage::slotAggregationCheckboxChanged()
{
    mAggregationComboBox->setEnabled(!mUseDefaultAggregationCheckBox->isChecked());
}

void CollectionViewPage::slotThemeCheckboxChanged()
{
    mThemeComboBox->setEnabled(!mUseDefaultThemeCheckBox->isChecked());
}

void CollectionViewPage::slotSelectFolderAggregation()
{
    bool usesPrivateAggregation = false;
    mAggregationComboBox->readStorageModelConfig(mCurrentCollection, usesPrivateAggregation);
    mUseDefaultAggregationCheckBox->setChecked(!usesPrivateAggregation);
}

void CollectionViewPage::slotSelectFolderTheme()
{
    bool usesPrivateTheme = false;
    mThemeComboBox->readStorageModelConfig(mCurrentCollection, usesPrivateTheme);
    mUseDefaultThemeCheckBox->setChecked(!usesPrivateTheme);
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

    if (col.hasAttribute<Akonadi::MessageFolderAttribute>()) {
        const bool outboundFolder = col.attribute<Akonadi::MessageFolderAttribute>()->isOutboundFolder();
        if (outboundFolder) {
            mShowSenderReceiverComboBox->setCurrentIndex(2);
        } else {
            mShowSenderReceiverComboBox->setCurrentIndex(1);
        }
    } else {
        mShowSenderReceiverComboBox->setCurrentIndex(0);
    }
    mShowSenderReceiverValue = mShowSenderReceiverComboBox->currentIndex();

    // message list aggregation
    slotSelectFolderAggregation();

    // message list theme
    slotSelectFolderTheme();

    const MessageViewer::Viewer::DisplayFormatMessage formatMessage = mFolderCollection->formatMessage();
    switch (formatMessage) {
    case MessageViewer::Viewer::Html:
        mPreferHtmlToText->setChecked(true);
        break;
    case MessageViewer::Viewer::Text:
        mPreferTextToHtml->setChecked(true);
        break;
    case MessageViewer::Viewer::UseGlobalSetting:
        mUseGlobalSettings->setChecked(true);
        break;
    default:
        qCDebug(KMAIL_LOG) << "No settings defined";
        break;
    }
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

    const int currentIndex = mShowSenderReceiverComboBox->currentIndex();
    if (mShowSenderReceiverValue != currentIndex) {
        if (currentIndex == 1) {
            Akonadi::MessageFolderAttribute *messageFolder = col.attribute<Akonadi::MessageFolderAttribute>(Akonadi::Collection::AddIfMissing);
            messageFolder->setOutboundFolder(false);
        } else if (currentIndex == 2) {
            Akonadi::MessageFolderAttribute *messageFolder = col.attribute<Akonadi::MessageFolderAttribute>(Akonadi::Collection::AddIfMissing);
            messageFolder->setOutboundFolder(true);
        } else {
            col.removeAttribute<Akonadi::MessageFolderAttribute>();
        }
    }
    // message list theme
    const bool usePrivateTheme = !mUseDefaultThemeCheckBox->isChecked();
    mThemeComboBox->writeStorageModelConfig(QString::number(mCurrentCollection.id()), usePrivateTheme);
    // message list aggregation
    const bool usePrivateAggregation = !mUseDefaultAggregationCheckBox->isChecked();
    mAggregationComboBox->writeStorageModelConfig(QString::number(mCurrentCollection.id()), usePrivateAggregation);

    MessageViewer::Viewer::DisplayFormatMessage formatMessage = MessageViewer::Viewer::Unknown;
    if (mPreferHtmlToText->isChecked()) {
        formatMessage = MessageViewer::Viewer::Html;
    } else if (mPreferTextToHtml->isChecked()) {
        formatMessage = MessageViewer::Viewer::Text;
    } else if (mUseGlobalSettings->isChecked()) {
        formatMessage = MessageViewer::Viewer::UseGlobalSetting;
    } else {
        qCDebug(KMAIL_LOG) << "No settings defined";
    }
    mFolderCollection->setFormatMessage(formatMessage);
}
