/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Based on KMMsgBase code by:
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
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
#include "codecmanager.h"

// KMail
#include "kmkernel.h"

// Qt
#include <QTextCodec>

// KDE libs
#include <kcodecaction.h>
#include <messagecomposer/messagecomposersettings.h>

class CodecManagerPrivate
{
public:
    CodecManagerPrivate();
    ~CodecManagerPrivate();

    CodecManager *instance;
    QList<QByteArray> preferredCharsets;
};

Q_GLOBAL_STATIC(CodecManagerPrivate, sInstance)

CodecManagerPrivate::CodecManagerPrivate()
    : instance(new CodecManager(this))
{
    instance->updatePreferredCharsets();
}

CodecManagerPrivate::~CodecManagerPrivate()
{
    delete instance;
}

CodecManager::CodecManager(CodecManagerPrivate *dd)
    : d(dd)
{
}

// static
CodecManager *CodecManager::self()
{
    return sInstance->instance;
}

QList<QByteArray> CodecManager::preferredCharsets() const
{
    return d->preferredCharsets;
}

void CodecManager::updatePreferredCharsets()
{
    const QStringList prefCharsets = MessageComposer::MessageComposerSettings::self()->preferredCharsets();
    d->preferredCharsets.clear();
    for (const QString &str : prefCharsets) {
        QByteArray charset = str.toLatin1().toLower();

        if (charset == "locale") {
            charset = QTextCodec::codecForLocale()->name();

            // Special case for Japanese:
            // (Introduction to i18n, 6.6 Limit of Locale technology):
            // EUC-JP is the de-facto standard for UNIX systems, ISO 2022-JP
            // is the standard for Internet, and Shift-JIS is the encoding
            // for Windows and Macintosh.
            if (charset == "jisx0208.1983-0"
                || charset == "eucjp"
                || charset == "shift-jis") {
                charset = "iso-2022-jp";
                // TODO wtf is "jis7"?
            }

            // Special case for Korean:
            if (charset == "ksc5601.1987-0") {
                charset = "euc-kr";
            }
        }
        d->preferredCharsets << charset;
    }
}
