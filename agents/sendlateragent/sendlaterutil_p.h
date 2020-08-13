/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERUTIL_P_H
#define SENDLATERUTIL_P_H

#include <QDateTime>

namespace SendLater {
class SendLaterInfo;

namespace SendLaterUtil {
Q_REQUIRED_RESULT QDateTime updateRecurence(SendLater::SendLaterInfo *info, QDateTime dateTime);
}
}
#endif // SENDLATERUTIL_P_H
