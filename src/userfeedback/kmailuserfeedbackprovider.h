/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMAILUSERFEEDBACKPROVIDER_H
#define KMAILUSERFEEDBACKPROVIDER_H

#include <KUserFeedback/Provider>
#include "kmail_export.h"

class KMAIL_EXPORT KMailUserFeedbackProvider : public KUserFeedback::Provider
{
    Q_OBJECT
public:
    explicit KMailUserFeedbackProvider(QObject *parent = nullptr);
    ~KMailUserFeedbackProvider();
};

#endif // KMAILUSERFEEDBACKPROVIDER_H
