/*
   SPDX-FileCopyrightText: 2009-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLLECTIONVIEWPAGE_H
#define COLLECTIONVIEWPAGE_H

#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>
#include <MailCommon/FolderSettings>
class QCheckBox;
class QLabel;
class QComboBox;
class KIconButton;
class QRadioButton;
class CollectionViewWidget;
template<typename T> class QSharedPointer;

namespace MessageList {
namespace Utils {
class AggregationComboBox;
class ThemeComboBox;
}
}
namespace MailCommon {
class CollectionViewWidget;
}
class CollectionViewPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionViewPage(QWidget *parent = nullptr);
    ~CollectionViewPage() override;

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

private:
    void slotChangeIcon(const QString &icon);

    void init(const Akonadi::Collection &);
    MailCommon::CollectionViewWidget *mCollectionViewWidget = nullptr;
    QSharedPointer<MailCommon::FolderSettings> mFolderCollection;
    QCheckBox *mIconsCheckBox = nullptr;
    QLabel *mNormalIconLabel = nullptr;
    KIconButton *mNormalIconButton = nullptr;
    QLabel *mUnreadIconLabel = nullptr;
    KIconButton *mUnreadIconButton = nullptr;
    bool mIsLocalSystemFolder = false;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionViewPageFactory, CollectionViewPage)

#endif /* COLLECTIONVIEWPAGE_H */
