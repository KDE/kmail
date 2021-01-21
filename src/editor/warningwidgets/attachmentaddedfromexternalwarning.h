/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ATTACHMENTADDEDFROMEXTERNALWARNING_H
#define ATTACHMENTADDEDFROMEXTERNALWARNING_H

#include <QWidget>
#include <KMessageWidget>

class AttachmentAddedFromExternalWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit AttachmentAddedFromExternalWarning(QWidget *parent = nullptr);
    ~AttachmentAddedFromExternalWarning() override;

    void setAttachmentNames(const QStringList &lst);
};

#endif // ATTACHMENTADDEDFROMEXTERNALWARNING_H
