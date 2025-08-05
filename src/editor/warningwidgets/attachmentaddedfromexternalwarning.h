/*
   SPDX-FileCopyrightText: 2020-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KMessageWidget>
class QWidget;

class AttachmentAddedFromExternalWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit AttachmentAddedFromExternalWarning(QWidget *parent = nullptr);
    ~AttachmentAddedFromExternalWarning() override;

    void setAttachmentNames(const QStringList &lst);
};
