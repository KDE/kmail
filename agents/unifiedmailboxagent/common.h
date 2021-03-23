/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>

namespace Common
{
static const auto MailMimeType = QStringLiteral("message/rfc822");

static const auto InboxBoxId = QStringLiteral("inbox");
static const auto SentBoxId = QStringLiteral("sent-mail");
static const auto DraftsBoxId = QStringLiteral("drafts");

static constexpr auto SpecialCollectionInbox = "inbox";
static constexpr auto SpecialCollectionSentMail = "send-mail";
static constexpr auto SpecialCollectionDrafts = "drafts";

static const auto AgentIdentifier = QStringLiteral("akonadi_unifiedmailbox_agent");
}

