/*
  SPDX-FileCopyrightText: 2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

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

private:
    MailCommon::CollectionExpiryWidget *const mCollectionExpiryWidget;
};
