/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <MailCommon/FolderRequester>

namespace Akonadi
{
class Collection;
}

namespace KMail
{
class IdentityFolderRequester : public MailCommon::FolderRequester
{
    Q_OBJECT
public:
    explicit IdentityFolderRequester(QWidget *parent = nullptr);
    ~IdentityFolderRequester() override;

    void setIsInvalidFolder(const Akonadi::Collection &col);

private:
    void slotFolderChanged(const Akonadi::Collection &col);
};
}
