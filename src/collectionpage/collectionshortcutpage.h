/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2004 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2013 Jonathan Marten <jjm@keelhaul.me.uk>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <MailCommon/FolderSettings>

#include <AkonadiWidgets/collectionpropertiespage.h>

template<typename T> class QSharedPointer;

class KKeySequenceWidget;

class CollectionShortcutPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionShortcutPage(QWidget *parent = nullptr);
    ~CollectionShortcutPage() override;

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

private:
    void slotShortcutChanged();
    void init(const Akonadi::Collection &);
    QSharedPointer<MailCommon::FolderSettings> mFolder;
    Akonadi::Collection mCurrentCollection;
    KKeySequenceWidget *const mKeySeqWidget;
    bool mShortcutChanged = false;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionShortcutPageFactory, CollectionShortcutPage)

