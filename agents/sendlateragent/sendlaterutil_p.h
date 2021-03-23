/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>

namespace SendLater
{
class SendLaterInfo;

namespace SendLaterUtil
{
Q_REQUIRED_RESULT QDateTime updateRecurence(SendLater::SendLaterInfo *info, QDateTime dateTime);
}
}
