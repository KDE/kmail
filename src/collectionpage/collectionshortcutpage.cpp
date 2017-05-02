/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2004 Till Adam <adam@kde.org>
  Copyright (c) 2013 Jonathan Marten <jjm@keelhaul.me.uk>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "collectionshortcutpage.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "foldershortcutactionmanager.h"

#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <QDialog>
#include <QHBoxLayout>
#include <KLocalizedString>
#include <KKeySequenceWidget>
#include <KConfigGroup>

using namespace MailCommon;

CollectionShortcutPage::CollectionShortcutPage(QWidget *parent) :
    CollectionPropertiesPage(parent),
    mShortcutChanged(false)
{
    setObjectName(QStringLiteral("KMail::CollectionShortcutPage"));
    setPageTitle(i18nc("@title:tab Shortcut settings for a folder.", "Shortcut"));
}

CollectionShortcutPage::~CollectionShortcutPage()
{
}

void CollectionShortcutPage::init(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
    mFolder = FolderCollection::forCollection(col, false);

    QVBoxLayout *topLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18n("<qt>To choose a key or a combination "
                                    "of keys which select the current folder, "
                                    "click the button below and then press the key(s) "
                                    "you wish to associate with this folder.</qt>"), this);
    label->setWordWrap(true);
    topLayout->addWidget(label);

    QHBoxLayout *hbHBoxLayout = new QHBoxLayout;

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

