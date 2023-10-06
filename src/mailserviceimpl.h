/*
 *
 *  This file is part of KMail, the KDE mail client.
 *  SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

class QByteArray;
class QString;
#include <QObject>

namespace KMail
{
// This class implements the D-Bus interface
// libkdepim/interfaces/org.kde.mailtransport.service.xml
class MailServiceImpl : public QObject
{
    Q_OBJECT
public:
    explicit MailServiceImpl(QObject *parent = nullptr);
    [[nodiscard]] bool sendMessage(const QString &from,
                                   const QString &to,
                                   const QString &cc,
                                   const QString &bcc,
                                   const QString &subject,
                                   const QString &body,
                                   const QStringList &attachments);

    [[nodiscard]] bool sendMessage(const QString &from,
                                   const QString &to,
                                   const QString &cc,
                                   const QString &bcc,
                                   const QString &subject,
                                   const QString &body,
                                   const QByteArray &attachment);
};
}
