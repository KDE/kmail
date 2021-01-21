/*
  SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef IDENTITYINVALIDFOLDER_H
#define IDENTITYINVALIDFOLDER_H

#include <KMessageWidget>
namespace KMail {
class IdentityInvalidFolder : public KMessageWidget
{
    Q_OBJECT
public:
    explicit IdentityInvalidFolder(QWidget *parent = nullptr);
    ~IdentityInvalidFolder() override;

    void setErrorMessage(const QString &msg);
};
}
#endif // IDENTITYINVALIDFOLDER_H
