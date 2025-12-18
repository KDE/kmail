/*
 * SPDX-FileCopyrightText: 2011-2025 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "kmsearchmessagemodel.h"
#include <MailCommon/MailUtil>
#include <MessageList/MessageListUtil>

#include <MessageCore/StringUtil>

#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>
#include <Akonadi/Session>

#include <Akonadi/MessageParts>
#include <KMime/Message>

#include <KLocalizedString>
#include <QApplication>
#include <QColor>
#include <QPalette>

KMSearchMessageModel::KMSearchMessageModel(Akonadi::Monitor *monitor, QObject *parent)
    : Akonadi::MessageModel(monitor, parent)
{
    monitor->itemFetchScope().fetchFullPayload();
    monitor->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::All);
}

KMSearchMessageModel::~KMSearchMessageModel() = default;

static QString toolTip(const Akonadi::Item &item)
{
    auto msg = item.payload<std::shared_ptr<KMime::Message>>();

    const QColor bckColor = QApplication::palette().color(QPalette::ToolTipBase);
    const QColor txtColor = QApplication::palette().color(QPalette::ToolTipText);

    const QString bckColorName = bckColor.name();
    const QString txtColorName = txtColor.name();
    const bool textIsLeftToRight = (QApplication::layoutDirection() == Qt::LeftToRight);
    const QString textDirection = textIsLeftToRight ? QStringLiteral("left") : QStringLiteral("right");

    QString tip = QStringLiteral("<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">");
    QString subject;
    if (auto msgSubject = msg->subject(KMime::CreatePolicy::DontCreate)) {
        subject = msgSubject->asUnicodeString();
    }
    tip += QStringLiteral(
               "<tr>"
               "<td bgcolor=\"%1\" align=\"%4\" valign=\"middle\">"
               "<div style=\"color: %2; font-weight: bold;\">"
               "%3"
               "</div>"
               "</td>"
               "</tr>")
               .arg(txtColorName, bckColorName, subject.toHtmlEscaped(), textDirection);

    tip += QStringLiteral(
        "<tr>"
        "<td align=\"center\" valign=\"middle\">"
        "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">");

    const QString htmlCodeForStandardRow = QStringLiteral(
        "<tr>"
        "<td align=\"right\" valign=\"top\" width=\"45\">"
        "<div style=\"font-weight: bold;\"><nobr>"
        "%1:"
        "</nobr></div>"
        "</td>"
        "<td align=\"left\" valign=\"top\">"
        "%2"
        "</td>"
        "</tr>");

    QString content = MessageList::Util::contentSummary(item);

    if (textIsLeftToRight) {
        if (auto from = msg->from(KMime::CreatePolicy::DontCreate)) {
            tip += htmlCodeForStandardRow.arg(i18n("From"), from->displayString());
        }
        if (auto to = msg->to(KMime::CreatePolicy::DontCreate)) {
            tip += htmlCodeForStandardRow.arg(i18nc("Receiver of the email", "To"), to->displayString());
        }
        if (auto date = msg->date(KMime::CreatePolicy::DontCreate)) {
            tip += htmlCodeForStandardRow.arg(i18n("Date"), QLocale().toString(date->dateTime()));
        }
        if (!content.isEmpty()) {
            tip += htmlCodeForStandardRow.arg(i18n("Preview"), content.replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        }
    } else {
        if (auto from = msg->from(KMime::CreatePolicy::DontCreate)) {
            tip += htmlCodeForStandardRow.arg(from->displayString(), i18n("From"));
        }
        if (auto to = msg->to(KMime::CreatePolicy::DontCreate)) {
            tip += htmlCodeForStandardRow.arg(to->displayString(), i18nc("Receiver of the email", "To"));
        }
        if (auto date = msg->date(KMime::CreatePolicy::DontCreate)) {
            tip += htmlCodeForStandardRow.arg(QLocale().toString(date->dateTime()), i18n("Date"));
        }
        if (!content.isEmpty()) {
            tip += htmlCodeForStandardRow.arg(content.replace(QLatin1Char('\n'), QStringLiteral("<br>")), i18n("Preview"));
        }
    }
    tip += QLatin1StringView(
        "</table"
        "</td>"
        "</tr>");
    return tip;
}

int KMSearchMessageModel::entityColumnCount(HeaderGroup headerGroup) const
{
    if (headerGroup == Akonadi::EntityTreeModel::ItemListHeaders) {
        return 6; // keep in sync with the column type enum
    }

    return Akonadi::MessageModel::entityColumnCount(headerGroup);
}

QString KMSearchMessageModel::fullCollectionPath(Akonadi::Collection::Id id) const
{
    QString path = m_collectionFullPathCache.value(id);
    if (path.isEmpty()) {
        path = MailCommon::Util::fullCollectionPath(Akonadi::Collection(id));
        m_collectionFullPathCache.insert(id, path);
    }
    return path;
}

QVariant KMSearchMessageModel::entityData(const Akonadi::Item &item, int column, int role) const
{
    if (!item.isValid()) {
        QVariant();
    }

    if (role == Qt::ToolTipRole) {
        return toolTip(item);
    }

    // The Collection column is first and is added by this model
    if (column == int(KMSearchMessageModel::Column::Collection)) {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const Akonadi::Collection::Id collectionId = item.storageCollectionId();
            if (collectionId >= 0) {
                return fullCollectionPath(collectionId);
            }
            return fullCollectionPath(item.parentCollection().id());
        }
        return {};
    } else {
        // Delegate the remaining columns to the MessageModel
        return Akonadi::MessageModel::entityData(item, column - 1, role);
    }
}

QVariant KMSearchMessageModel::entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == int(KMSearchMessageModel::Column::Collection)) {
        return i18nc("@title:column, folder (e.g. email)", "Folder");
    }
    return Akonadi::MessageModel::entityHeaderData((section - 1), orientation, role, headerGroup);
}

#include "moc_kmsearchmessagemodel.cpp"
