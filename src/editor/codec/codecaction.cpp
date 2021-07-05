/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Own
#include "codecaction.h"

// KMail
#include "codecmanager.h"

#include <MimeTreeParser/NodeHelper>

#include <KCharsets>
#include <QTextCodec>
// Qt

// KDE libs
#include "kmail_debug.h"
#include <KLocalizedString>
#include <QIcon>
#include <QTextCodec>

CodecAction::CodecAction(Mode mode, QObject *parent)
    : KCodecAction(parent, mode == ReaderMode)
    , mMode(mode)
{
    if (mode == ComposerMode) {
        // Add 'us-ascii' entry.  We want it at the top, so remove then re-add everything.
        // FIXME is there a better way?
        QList<QAction *> oldActions = actions();
        removeAllActions();
        addAction(oldActions.takeFirst()); // 'Default'
        addAction(i18nc("Encodings menu", "us-ascii"));
        for (QAction *a : std::as_const(oldActions)) {
            addAction(a);
        }
    } else if (mode == ReaderMode) {
        // Nothing to do.
    }

    // Eye candy.
    setIcon(QIcon::fromTheme(QStringLiteral("accessories-character-map")));
    setText(i18nc("Menu item", "Encoding"));
}

CodecAction::~CodecAction() = default;

QVector<QByteArray> CodecAction::mimeCharsets() const
{
    QVector<QByteArray> ret;
    qCDebug(KMAIL_LOG) << "current item" << currentItem() << currentText();
    if (currentItem() == 0) {
        // 'Default' selected: return the preferred charsets.
        ret = CodecManager::self()->preferredCharsets();
    } else if (currentItem() == 1) {
        // 'us-ascii' selected.
        ret << "us-ascii";
    } else {
        // Specific codec selected.
        // ret << currentCodecName().toLatin1().toLower(); // FIXME in kdelibs: returns e.g. '&koi8-r'
        ret << currentCodec()->name();
        qCDebug(KMAIL_LOG) << "current codec name" << ret.at(0);
    }
    return ret;
}

void CodecAction::setAutoCharset()
{
    setCurrentItem(0);
}

// We can't simply use KCodecAction::setCurrentCodec(), since that doesn't
// use fixEncoding().
static QString selectCharset(KSelectAction *root, const QString &encoding)
{
    const QList<QAction *> lstActs = root->actions();
    for (QAction *action : lstActs) {
        auto subMenu = qobject_cast<KSelectAction *>(action);
        if (subMenu) {
            const QString codecNameToSet = selectCharset(subMenu, encoding);
            if (!codecNameToSet.isEmpty()) {
                return codecNameToSet;
            }
        } else {
            const QString fixedActionText = MimeTreeParser::NodeHelper::fixEncoding(action->text());
            if (KCharsets::charsets()->codecForName(KCharsets::charsets()->encodingForName(fixedActionText)) == KCharsets::charsets()->codecForName(encoding)) {
                return action->text();
            }
        }
    }
    return {};
}

void CodecAction::setCharset(const QByteArray &charset)
{
    const QString codecNameToSet = selectCharset(this, QString::fromLatin1(charset));
    if (codecNameToSet.isEmpty()) {
        qCDebug(KMAIL_LOG) << "Could not find charset" << charset;
        setAutoCharset();
    } else {
        setCurrentCodec(codecNameToSet);
    }
}
