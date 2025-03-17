/*
  SPDX-FileCopyrightText: 2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include <Akonadi/Collection>
#include <QDialog>
namespace MailCommon
{
class CollectionExpiryWidget;
}
class IdentityExpireSpamFolderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IdentityExpireSpamFolderDialog(QWidget *parent = nullptr);
    ~IdentityExpireSpamFolderDialog() override;

    void load(const Akonadi::Collection &collection);

private:
    void slotSaveAndExpire();
    void slotChanged();
    void slotConfigChanged(bool changed);
    void saveAndExpire(Akonadi::Collection &collection, bool saveSettings, bool expireNow);
    MailCommon::CollectionExpiryWidget *const mCollectionExpiryWidget;
    Akonadi::Collection mCollection;
    bool mChanged = false;
};
