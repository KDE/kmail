/*
   SPDX-FileCopyrightText: 2009-2022 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <Akonadi/CollectionPropertiesPage>
#include <MailCommon/FolderSettings>
class QCheckBox;
class QLabel;
class KIconButton;
class CollectionViewWidget;
template<typename T> class QSharedPointer;

namespace MailCommon
{
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

