/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2004 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2013 Jonathan Marten <jjm@keelhaul.me.uk>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionshortcutpage.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "foldershortcutactionmanager.h"

#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <QHBoxLayout>
#include <KLocalizedString>
#include <KKeySequenceWidget>

using namespace MailCommon;

CollectionShortcutPage::CollectionShortcutPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setObjectName(QStringLiteral("KMail::CollectionShortcutPage"));
    setPageTitle(i18nc("@title:tab Shortcut settings for a folder.", "Shortcut"));
}

CollectionShortcutPage::~CollectionShortcutPage()
= default;

void CollectionShortcutPage::init(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
    mFolder = FolderSettings::forCollection(col, false);

    auto topLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18n("<qt>To choose a key or a combination "
                                    "of keys which select the current folder, "
                                    "click the button below and then press the key(s) "
                                    "you wish to associate with this folder.</qt>"), this);
    label->setWordWrap(true);
    topLayout->addWidget(label);

    auto hbHBoxLayout = new QHBoxLayout;

    mKeySeqWidget = new KKeySequenceWidget(this);
    hbHBoxLayout->addWidget(mKeySeqWidget);
    mKeySeqWidget->setObjectName(QStringLiteral("FolderShortcutSelector"));
    connect(mKeySeqWidget, &KKeySequenceWidget::keySequenceChanged, this, &CollectionShortcutPage::slotShortcutChanged);

    topLayout->addItem(new QSpacerItem(0, 10));
    topLayout->addLayout(hbHBoxLayout);
    topLayout->addStretch(1);

    mKeySeqWidget->setCheckActionCollections(KMKernel::self()->getKMMainWidget()->actionCollections());
}

void CollectionShortcutPage::load(const Akonadi::Collection &col)
{
    init(col);
    if (mFolder) {
        mKeySeqWidget->setKeySequence(mFolder->shortcut(),
                                      KKeySequenceWidget::NoValidate);
        mShortcutChanged = false;
    }
}

void CollectionShortcutPage::save(Akonadi::Collection & /*col*/)
{
    if (mFolder) {
        if (mShortcutChanged) {
            mKeySeqWidget->applyStealShortcut();
            mFolder->setShortcut(mKeySeqWidget->keySequence());
            KMKernel::self()->getKMMainWidget()->folderShortcutActionManager()->shortcutChanged(mCurrentCollection);
        }
    }
}

void CollectionShortcutPage::slotShortcutChanged()
{
    mShortcutChanged = true;
}
