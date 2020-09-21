/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "collectiontemplateswidget.h"

#include <KLocalizedString>
#include "templatesconfiguration_kfg.h"

#include <MailCommon/FolderSettings>
#include <QCheckBox>
#include <QSharedPointer>
#include <QVBoxLayout>
#include <TemplateParser/TemplatesConfiguration>

CollectionTemplatesWidget::CollectionTemplatesWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    QHBoxLayout *topItems = new QHBoxLayout;
    topItems->setContentsMargins(0, 0, 0, 0);
    topLayout->addLayout(topItems);

    mCustom = new QCheckBox(i18n("&Use custom message templates in this folder"), this);
    connect(mCustom, &QCheckBox::clicked, this, &CollectionTemplatesWidget::slotChanged);
    topItems->addWidget(mCustom, Qt::AlignLeft);

    mWidget = new TemplateParser::TemplatesConfiguration(this, QStringLiteral("folder-templates"));
    connect(mWidget, &TemplateParser::TemplatesConfiguration::changed, this, &CollectionTemplatesWidget::slotChanged);
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

    connect(copyGlobal, &QPushButton::clicked, this, &CollectionTemplatesWidget::slotCopyGlobal);
}

CollectionTemplatesWidget::~CollectionTemplatesWidget()
{

}

void CollectionTemplatesWidget::save(const Akonadi::Collection &)
{
    if (mChanged && !mCollectionId.isEmpty()) {
        TemplateParser::Templates t(mCollectionId);
        //qCDebug(KMAIL_LOG) << "use custom templates for folder" << fid <<":" << mCustom->isChecked();
        t.setUseCustomTemplates(mCustom->isChecked());
        t.save();

        mWidget->saveToFolder(mCollectionId);
    }
}

void CollectionTemplatesWidget::slotCopyGlobal()
{
    if (mIdentity) {
        mWidget->loadFromIdentity(mIdentity);
    } else {
        mWidget->loadFromGlobal();
    }
}

void CollectionTemplatesWidget::slotChanged()
{
    mChanged = true;
}

void CollectionTemplatesWidget::load(const Akonadi::Collection &col)
{
    const QSharedPointer<MailCommon::FolderSettings> fd = MailCommon::FolderSettings::forCollection(col, false);
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
