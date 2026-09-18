/*
 *
 * SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
 * SPDX-FileCopyrightText: 2009-2026 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "collectionquotapage.h"

#include "collectionquotawidget.h"
#include <Akonadi/Collection>
#include <Akonadi/CollectionQuotaAttribute>

#include <KLocalizedString>
#include <QVBoxLayout>
using namespace Qt::Literals::StringLiterals;
CollectionQuotaPage::CollectionQuotaPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
    , mQuotaWidget(new CollectionQuotaWidget(this))
{
    setObjectName("KMail::CollectionQuotaPage"_L1);
    setPageTitle(i18nc("@title:tab Quota page.", "Quota"));
    init();
}

bool CollectionQuotaPage::canHandle(const Akonadi::Collection &collection) const
{
    if (const auto att = collection.attribute<Akonadi::CollectionQuotaAttribute>(); att) {
        if (att->maximumValue() <= 0) {
            return false;
        }
        return true;
    }
    return false;
}

void CollectionQuotaPage::init()
{
    auto topLayout = new QVBoxLayout(this);
    topLayout->addWidget(mQuotaWidget);
}

void CollectionQuotaPage::load(const Akonadi::Collection &col)
{
    if (const auto att = col.attribute<Akonadi::CollectionQuotaAttribute>(); att) {
        const qint64 currentValue = att->currentValue();

        const qint64 maximumValue = att->maximumValue();
        // Test over quota.
        mQuotaWidget->setQuotaInfo(qMin(currentValue, maximumValue), maximumValue);
    }
}

void CollectionQuotaPage::save(Akonadi::Collection &)
{
    // nothing to do, we are read-only
}

#include "moc_collectionquotapage.cpp"
