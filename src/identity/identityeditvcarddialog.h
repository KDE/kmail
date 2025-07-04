/*
  SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>

namespace Akonadi
{
class AkonadiContactEditor;
}

class IdentityEditVcardDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IdentityEditVcardDialog(const QString &fileName, QWidget *parent = nullptr);
    ~IdentityEditVcardDialog() override;
    /**
     * @brief loadVcard load vcard in a contact editor
     * @param vcardFileName
     */
    void loadVcard(const QString &vcardFileName);
    /**
     * @brief saveVcard
     * @return The file path for current vcard.
     */
    [[nodiscard]] QString saveVcard() const;

    void reject() override;
Q_SIGNALS:
    void vcardRemoved();

private:
    void slotDeleteCurrentVCard();
    void deleteCurrentVcard(bool deleteOnDisk);
    QString mVcardFileName;
    Akonadi::AkonadiContactEditor *const mContactEditor;
};
