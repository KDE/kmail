/*
   SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

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
[[nodiscard]] bool compareSendLaterInfo(MessageComposer::SendLaterInfo *left, MessageComposer::SendLaterInfo *right);

[[nodiscard]] KSharedConfig::Ptr defaultConfig();

void writeSendLaterInfo(KSharedConfig::Ptr config, MessageComposer::SendLaterInfo *info);
[[nodiscard]] MessageComposer::SendLaterInfo *readSendLaterInfo(KConfigGroup &config);

[[nodiscard]] bool sentLaterAgentEnabled();

void changeRecurrentDate(MessageComposer::SendLaterInfo *info);
void forceReparseConfiguration();

[[nodiscard]] QString sendLaterPattern();
}
