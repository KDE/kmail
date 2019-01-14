/*
 * Copyright (c) 2011-2019 Montel Laurent <montel@kde.org>
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
#include "MailCommon/MailUtil"
#include "messagelist/messagelistutil.h"

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
#include <KFormat>

KMSearchMessageModel::KMSearchMessageModel(QObject *parent)
    : Akonadi::MessageModel(parent)
{
    fetchScope().fetchFullPayload();
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::All);
}

KMSearchMessageModel::~KMSearchMessageModel()
{
}

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
        tip += htmlCodeForStandardRow.arg(msg->from()->displayString()).arg(i18n("From"));
        tip += htmlCodeForStandardRow.arg(msg->to()->displayString()).arg(i18nc("Receiver of the email", "To"));
        tip += htmlCodeForStandardRow.arg(QLocale().toString(msg->date()->dateTime())).arg(i18n("Date"));
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

int KMSearchMessageModel::columnCount(const QModelIndex &parent) const
{
    if (collection().isValid()
        && !collection().contentMimeTypes().contains(QLatin1String("message/rfc822"))
        && collection().contentMimeTypes() != QStringList(QStringLiteral("inode/directory"))) {
        return 1;
    }

    if (!parent.isValid()) {
        return 6;    // keep in sync with the column type enum
    }

    return 0;
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

QVariant KMSearchMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.row() >= rowCount()) {
        return QVariant();
    }

    if (!collection().contentMimeTypes().contains(QLatin1String("message/rfc822"))) {
        if (role == Qt::DisplayRole) {
            return i18nc("@label", "This model can only handle email folders. The current collection holds mimetypes: %1",
                         collection().contentMimeTypes().join(QLatin1Char(',')));
        } else {
            return QVariant();
        }
    }

    Akonadi::Item item = itemForIndex(index);

    // Handle the most common case first, before calling payload().
    if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == Collection) {
        if (item.storageCollectionId() >= 0) {
            return fullCollectionPath(item.storageCollectionId());
        }
        return fullCollectionPath(item.parentCollection().id());
    }

    if (!item.hasPayload<KMime::Message::Ptr>()) {
        return QVariant();
    }
    KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Subject:
            return msg->subject()->asUnicodeString();
        case Sender:
            return msg->from()->asUnicodeString();
        case Receiver:
            return msg->to()->asUnicodeString();
        case Date:
            return QLocale().toString(msg->date()->dateTime());
        case Size:
            if (item.size() == 0) {
                return i18nc("@label No size available", "-");
            } else {
                return KFormat().formatByteSize(item.size());
            }
        default:
            return QVariant();
        }
    } else if (role == Qt::EditRole) { // used for sorting
        switch (index.column()) {
        case Subject:
            return msg->subject()->asUnicodeString();
        case Sender:
            return msg->from()->asUnicodeString();
        case Receiver:
            return msg->to()->asUnicodeString();
        case Date:
            return msg->date()->dateTime();
        case Size:
            return item.size();
        default:
            return QVariant();
        }
    } else if (role == Qt::ToolTipRole) {
        return toolTip(item);
    }
    return ItemModel::data(index, role);
}

QVariant KMSearchMessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (collection().isValid()
        && !collection().contentMimeTypes().contains(QLatin1String("message/rfc822"))
        && collection().contentMimeTypes() != QStringList(QStringLiteral("inode/directory"))) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Collection:
            return i18nc("@title:column, folder (e.g. email)", "Folder");
        default:
            return Akonadi::MessageModel::headerData((section - 1), orientation, role);
        }
    }
    return Akonadi::MessageModel::headerData((section - 1), orientation, role);
}
