/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef COMMON_H
#define COMMON_H

#include <QString>

namespace Common {

static const auto MailMimeType = QStringLiteral("message/rfc822");

static const auto InboxBoxId = QStringLiteral("inbox");
static const auto SentBoxId = QStringLiteral("sent-mail");
static const auto DraftsBoxId = QStringLiteral("drafts");

static constexpr auto SpecialCollectionInbox = "inbox";
static constexpr auto SpecialCollectionSentMail = "send-mail";
static constexpr auto SpecialCollectionDrafts = "drafts";

static const auto AgentIdentifier = QStringLiteral("akonadi_unifiedmailbox_agent");

}

#endif
