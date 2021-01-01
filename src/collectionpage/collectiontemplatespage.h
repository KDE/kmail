/*
   SPDX-FileCopyrightText: 2009-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLLECTIONTEMPLATESPAGE_H
#define COLLECTIONTEMPLATESPAGE_H
#include <AkonadiWidgets/collectionpropertiespage.h>
namespace MailCommon {
class CollectionTemplatesWidget;
}
template<typename T> class QSharedPointer;

class CollectionTemplatesPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionTemplatesPage(QWidget *parent = nullptr);
    ~CollectionTemplatesPage() override;

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;
    Q_REQUIRED_RESULT bool canHandle(const Akonadi::Collection &collection) const override;

private:
    void init();
    MailCommon::CollectionTemplatesWidget *mCollectionTemplateWidget = nullptr;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionTemplatesPageFactory, CollectionTemplatesPage)

#endif /* COLLECTIONTEMPLATESPAGE_H */
