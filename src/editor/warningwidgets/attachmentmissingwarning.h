/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KMessageWidget>

class AttachmentMissingWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit AttachmentMissingWarning(QWidget *parent = nullptr);
    ~AttachmentMissingWarning() override;
    void slotFileAttached();

Q_SIGNALS:
    void attachMissingFile();
    void closeAttachMissingFile();
    void explicitClosedMissingAttachment();

private:
    void explicitlyClosed();
    void slotAttachFile();
};

