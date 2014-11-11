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

// Qt
#include <QTextCodec>

// KDE libs
#include <QDebug>
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
    setIcon(QIcon::fromTheme(QLatin1String("accessories-character-map")));
    setText(i18nc("Menu item", "Encoding"));
}

CodecAction::~CodecAction()
{
    delete d;
}

QList<QByteArray> CodecAction::mimeCharsets() const
{
    QList<QByteArray> ret;
    qDebug() << "current item" << currentItem() << currentText();
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
        qDebug() << "current codec name" << ret.first();
    }
    return ret;
}

