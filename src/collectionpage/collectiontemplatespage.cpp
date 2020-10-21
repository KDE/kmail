/*
   SPDX-FileCopyrightText: 2009-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "collectiontemplatespage.h"
#include <MailCommon/CollectionTemplatesWidget>

#include <MailCommon/MailKernel>
#include <MailCommon/FolderSettings>
#include <TemplateParser/TemplatesConfiguration>
#include "templatesconfiguration_kfg.h"
#include <AkonadiCore/collection.h>

#include <KLocalizedString>

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
    topLayout->setContentsMargins({});
    mCollectionTemplateWidget = new MailCommon::CollectionTemplatesWidget(this);
    topLayout->addWidget(mCollectionTemplateWidget);
}

void CollectionTemplatesPage::load(const Collection &col)
{
    mCollectionTemplateWidget->load(col);
}

void CollectionTemplatesPage::save(Collection &col)
{
    mCollectionTemplateWidget->save(col);
}
