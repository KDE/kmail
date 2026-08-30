/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>
using namespace Qt::Literals::StringLiterals;

namespace Common
{
static const auto MailMimeType = u"message/rfc822"_s;

static const auto InboxBoxId = u"inbox"_s;
static const auto SentBoxId = u"sent-mail"_s;
static const auto DraftsBoxId = u"drafts"_s;

static constexpr auto SpecialCollectionInbox = "inbox";
static constexpr auto SpecialCollectionSentMail = "send-mail";
static constexpr auto SpecialCollectionDrafts = "drafts";

static const auto AgentIdentifier = u"akonadi_unifiedmailbox_agent"_s;
}
