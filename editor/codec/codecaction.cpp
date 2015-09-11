/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

// Own
#include "codecaction.h"

// KMail
#include "codecmanager.h"

#include "viewer/nodehelper.h"

#include <KCharsets>
// Qt
#include <QTextCodec>

// KDE libs
#include "kmail_debug.h"
#include <KLocalizedString>
#include <QIcon>

class CodecAction::Private
{
public:
    Private(CodecAction::Mode mod, CodecAction *qq)
        : mode(mod)
        , q(qq)
    {
    }

    const CodecAction::Mode mode;
    CodecAction *const q;
};

CodecAction::CodecAction(Mode mode, QObject *parent)
    : KCodecAction(parent, mode == ReaderMode)
    , d(new Private(mode, this))
{
    if (mode == ComposerMode) {
        // Add 'us-ascii' entry.  We want it at the top, so remove then re-add everything.
        // FIXME is there a better way?
        QList<QAction *> oldActions = actions();
        removeAllActions();
        addAction(oldActions.takeFirst());   // 'Default'
        addAction(i18nc("Encodings menu", "us-ascii"));
        foreach (QAction *a, oldActions) {
            addAction(a);
        }
    } else if (mode == ReaderMode) {
        // Nothing to do.
    }

    // Eye candy.
    setIcon(QIcon::fromTheme(QStringLiteral("accessories-character-map")));
    setText(i18nc("Menu item", "Encoding"));
}

CodecAction::~CodecAction()
{
    delete d;
}

QList<QByteArray> CodecAction::mimeCharsets() const
{
    QList<QByteArray> ret;
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
    foreach (QAction *action, root->actions()) {
        KSelectAction *subMenu = dynamic_cast<KSelectAction *>(action);
        if (subMenu) {
            const QString codecNameToSet = selectCharset(subMenu, encoding);
            if (!codecNameToSet.isEmpty()) {
                return codecNameToSet;
            }
        } else {
            const QString fixedActionText = MessageViewer::NodeHelper::fixEncoding(action->text());
            if (KCharsets::charsets()->codecForName(
                        KCharsets::charsets()->encodingForName(fixedActionText))
                    == KCharsets::charsets()->codecForName(encoding)) {
                return action->text();
            }
        }
    }
    return QString();
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
