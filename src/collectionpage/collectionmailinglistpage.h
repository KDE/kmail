/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2005 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2011-2024 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2012 Jonathan Marten <jjm@keelhaul.me.uk>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <MailCommon/FolderSettings>
#include <MessageCore/MailingList>

#include <Akonadi/Collection>
#include <Akonadi/CollectionPropertiesPage>

class QCheckBox;
class QPushButton;

template<typename T>
class QSharedPointer;

class QComboBox;
class QPushButton;
class KJob;
class KEditListWidget;
class KSqueezedTextLabel;

class CollectionMailingListPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionMailingListPage(QWidget *parent = nullptr);
    ~CollectionMailingListPage() override;

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

    [[nodiscard]] bool canHandle(const Akonadi::Collection &col) const override;

private:
    void slotFetchDone(KJob *job);
    void init(const Akonadi::Collection &);
    /*
     * Detects mailing-list related stuff
     */
    void slotDetectMailingList();
    void slotInvokeHandler();
    void slotMLHandling(int element);
    void slotHoldsML(bool holdsML);
    void slotAddressChanged(int addr);
    void slotConfigChanged();
    void fillMLFromWidgets();
    void fillEditBox();

    Akonadi::Collection mCurrentCollection;
    QSharedPointer<MailCommon::FolderSettings> mFolder;

    MailingList mMailingList;
    QCheckBox *mHoldsMailingList = nullptr;
    QComboBox *mMLHandlerCombo = nullptr;
    QPushButton *mDetectButton = nullptr;
    QComboBox *mAddressCombo = nullptr;
    KEditListWidget *mEditList = nullptr;
    KSqueezedTextLabel *mMLId = nullptr;
    QPushButton *mHandleButton = nullptr;
    int mLastItem = 0;
    bool changed = false;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionMailingListPageFactory, CollectionMailingListPage)
