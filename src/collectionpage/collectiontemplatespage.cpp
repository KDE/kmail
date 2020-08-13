/*
   SPDX-FileCopyrightText: 2009-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "collectiontemplatespage.h"

#include <MailCommon/MailKernel>
#include <MailCommon/FolderSettings>
#include <TemplateParser/TemplatesConfiguration>
#include "templatesconfiguration_kfg.h"
#include <AkonadiCore/collection.h>

#include <KLocalizedString>
#include <QPushButton>
#include <QCheckBox>

using namespace Akonadi;
using namespace MailCommon;

CollectionTemplatesPage::CollectionTemplatesPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setObjectName(QStringLiteral("KMail::CollectionTemplatesPage"));
    setPageTitle(i18n("Templates"));
    init();
}

CollectionTemplatesPage::~CollectionTemplatesPage()
{
}

bool CollectionTemplatesPage::canHandle(const Collection &collection) const
{
    return !CommonKernel->isSystemFolderCollection(collection)
           || CommonKernel->isMainFolderCollection(collection);
}

void CollectionTemplatesPage::init()
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    QHBoxLayout *topItems = new QHBoxLayout;
    topItems->setContentsMargins(0, 0, 0, 0);
    topLayout->addLayout(topItems);

    mCustom = new QCheckBox(i18n("&Use custom message templates in this folder"), this);
    connect(mCustom, &QCheckBox::clicked, this, &CollectionTemplatesPage::slotChanged);
    topItems->addWidget(mCustom, Qt::AlignLeft);

    mWidget = new TemplateParser::TemplatesConfiguration(this, QStringLiteral("folder-templates"));
    connect(mWidget, &TemplateParser::TemplatesConfiguration::changed, this, &CollectionTemplatesPage::slotChanged);
    mWidget->setEnabled(false);

    // Move the help label outside of the templates configuration widget,
    // so that the help can be read even if the widget is not enabled.
    topItems->addStretch(9);
    topItems->addWidget(mWidget->helpLabel(), Qt::AlignRight);

    topLayout->addWidget(mWidget);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *copyGlobal = new QPushButton(i18n("&Copy Global Templates"), this);
    copyGlobal->setEnabled(false);
    btns->addWidget(copyGlobal);
    topLayout->addLayout(btns);

    connect(mCustom, &QCheckBox::toggled, mWidget, &TemplateParser::TemplatesConfiguration::setEnabled);
    connect(mCustom, &QCheckBox::toggled, copyGlobal, &QPushButton::setEnabled);

    connect(copyGlobal, &QPushButton::clicked, this, &CollectionTemplatesPage::slotCopyGlobal);
}

void CollectionTemplatesPage::load(const Collection &col)
{
    const QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(col, false);
    if (!fd) {
        return;
    }

    mCollectionId = QString::number(col.id());

    TemplateParser::Templates t(mCollectionId);

    mCustom->setChecked(t.useCustomTemplates());

    mIdentity = fd->identity();

    mWidget->loadFromFolder(mCollectionId, mIdentity);
    mChanged = false;
}

void CollectionTemplatesPage::save(Collection &)
{
    if (mChanged && !mCollectionId.isEmpty()) {
        TemplateParser::Templates t(mCollectionId);
        //qCDebug(KMAIL_LOG) << "use custom templates for folder" << fid <<":" << mCustom->isChecked();
        t.setUseCustomTemplates(mCustom->isChecked());
        t.save();

        mWidget->saveToFolder(mCollectionId);
    }
}

void CollectionTemplatesPage::slotCopyGlobal()
{
    if (mIdentity) {
        mWidget->loadFromIdentity(mIdentity);
    } else {
        mWidget->loadFromGlobal();
    }
}

void CollectionTemplatesPage::slotChanged()
{
    mChanged = true;
}
