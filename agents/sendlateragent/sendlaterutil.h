/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KSharedConfig>

namespace MessageComposer
{
class SendLaterInfo;
}

/** Send later utilities. */
namespace SendLaterUtil
{
Q_REQUIRED_RESULT bool compareSendLaterInfo(MessageComposer::SendLaterInfo *left, MessageComposer::SendLaterInfo *right);

Q_REQUIRED_RESULT KSharedConfig::Ptr defaultConfig();

void writeSendLaterInfo(KSharedConfig::Ptr config, MessageComposer::SendLaterInfo *info);
Q_REQUIRED_RESULT MessageComposer::SendLaterInfo *readSendLaterInfo(KConfigGroup &config);

Q_REQUIRED_RESULT bool sentLaterAgentEnabled();

void changeRecurrentDate(MessageComposer::SendLaterInfo *info);
void forceReparseConfiguration();

Q_REQUIRED_RESULT QString sendLaterPattern();
}

