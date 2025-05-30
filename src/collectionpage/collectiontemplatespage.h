/*
   SPDX-FileCopyrightText: 2009-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <Akonadi/CollectionPropertiesPage>
namespace MailCommon
{
class CollectionTemplatesWidget;
}
class CollectionTemplatesPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionTemplatesPage(QWidget *parent = nullptr);
    ~CollectionTemplatesPage() override;

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;
    [[nodiscard]] bool canHandle(const Akonadi::Collection &collection) const override;

private:
    void init();
    MailCommon::CollectionTemplatesWidget *const mCollectionTemplateWidget;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionTemplatesPageFactory, CollectionTemplatesPage)
