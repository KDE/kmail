/*
 * Copyright (c) 2011-2020 Laurent Montel <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#include "kmsearchmessagemodel.h"
#include <MailCommon/MailUtil>
#include <MessageList/MessageListUtil>

#include <MessageCore/StringUtil>

#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/monitor.h>
#include <AkonadiCore/session.h>

#include <Akonadi/KMime/MessageParts>
#include <kmime/kmime_message.h>

#include <QColor>
#include <QApplication>
#include <QPalette>
#include "kmail_debug.h"
#include <KLocalizedString>

KMSearchMessageModel::KMSearchMessageModel(Akonadi::Monitor *monitor, QObject *parent)
    : Akonadi::MessageModel(monitor, parent)
{
    monitor->itemFetchScope().fetchFullPayload();
    monitor->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::All);
}

KMSearchMessageModel::~KMSearchMessageModel() = default;

static QString toolTip(const Akonadi::Item &item)
{
    KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

    QColor bckColor = QApplication::palette().color(QPalette::ToolTipBase);
    QColor txtColor = QApplication::palette().color(QPalette::ToolTipText);

    const QString bckColorName = bckColor.name();
    const QString txtColorName = txtColor.name();
    const bool textIsLeftToRight = (QApplication::layoutDirection() == Qt::LeftToRight);
    const QString textDirection = textIsLeftToRight ? QStringLiteral("left") : QStringLiteral("right");

    QString tip = QStringLiteral(
        "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
        );
    tip += QStringLiteral(
        "<tr>"                                                         \
        "<td bgcolor=\"%1\" align=\"%4\" valign=\"middle\">"           \
        "<div style=\"color: %2; font-weight: bold;\">"                \
        "%3"                                                           \
        "</div>"                                                       \
        "</td>"                                                        \
        "</tr>"
        ).arg(txtColorName, bckColorName, msg->subject()->asUnicodeString().toHtmlEscaped(), textDirection);

    tip += QStringLiteral(
        "<tr>"                                                              \
        "<td align=\"center\" valign=\"middle\">"                           \
        "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
        );

    const QString htmlCodeForStandardRow = QStringLiteral(
        "<tr>"                                                       \
        "<td align=\"right\" valign=\"top\" width=\"45\">"           \
        "<div style=\"font-weight: bold;\"><nobr>"                   \
        "%1:"                                                        \
        "</nobr></div>"                                              \
        "</td>"                                                      \
        "<td align=\"left\" valign=\"top\">"                         \
        "%2"                                                         \
        "</td>"                                                      \
        "</tr>");

    QString content = MessageList::Util::contentSummary(item);

    if (textIsLeftToRight) {
        tip += htmlCodeForStandardRow.arg(i18n("From"), msg->from()->displayString());
        tip += htmlCodeForStandardRow.arg(i18nc("Receiver of the email", "To"), msg->to()->displayString());
        tip += htmlCodeForStandardRow.arg(i18n("Date"), QLocale().toString(msg->date()->dateTime()));
        if (!content.isEmpty()) {
            tip += htmlCodeForStandardRow.arg(i18n("Preview"), content.replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        }
    } else {
        tip += htmlCodeForStandardRow.arg(msg->from()->displayString(), i18n("From"));
        tip += htmlCodeForStandardRow.arg(msg->to()->displayString(), i18nc("Receiver of the email", "To"));
        tip += htmlCodeForStandardRow.arg(QLocale().toString(msg->date()->dateTime()), i18n("Date"));
        if (!content.isEmpty()) {
            tip += htmlCodeForStandardRow.arg(content.replace(QLatin1Char('\n'), QStringLiteral("<br>")), i18n("Preview"));
        }
    }
    tip += QLatin1String(
        "</table"         \
        "</td>"           \
        "</tr>"
        );
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
    if (role == Qt::ToolTipRole) {
        return toolTip(item);
    }

    // The Collection column is first and is added by this model
    if (column == Collection) {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (item.storageCollectionId() >= 0) {
                return fullCollectionPath(item.storageCollectionId());
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
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == Collection) {
        return i18nc("@title:column, folder (e.g. email)", "Folder");
    }
    return Akonadi::MessageModel::entityHeaderData((section - 1), orientation, role, headerGroup);
}
