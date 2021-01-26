/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Based on KMMsgBase code by:
 * SPDX-FileCopyrightText: 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Own
#include "codecmanager.h"

// KMail
#include "kmkernel.h"

// Qt
#include <QTextCodec>

// KDE libs
#include <MessageComposer/MessageComposerSettings>

CodecManager::CodecManager()
{
    updatePreferredCharsets();
}

// static
CodecManager *CodecManager::self()
{
    static CodecManager instance;
    return &instance;
}

QVector<QByteArray> CodecManager::preferredCharsets() const
{
    return mPreferredCharsets;
}

void CodecManager::updatePreferredCharsets()
{
    const QStringList prefCharsets = MessageComposer::MessageComposerSettings::self()->preferredCharsets();
    mPreferredCharsets.clear();
    for (const QString &str : prefCharsets) {
        QByteArray charset = str.toLatin1().toLower();

        if (charset == "locale") {
            charset = QTextCodec::codecForLocale()->name();

            // Special case for Japanese:
            // (Introduction to i18n, 6.6 Limit of Locale technology):
            // EUC-JP is the de-facto standard for UNIX systems, ISO 2022-JP
            // is the standard for Internet, and Shift-JIS is the encoding
            // for Windows and Macintosh.
            if (charset == "jisx0208.1983-0" || charset == "eucjp" || charset == "shift-jis") {
                charset = "iso-2022-jp";
                // TODO wtf is "jis7"?
            }

            // Special case for Korean:
            if (charset == "ksc5601.1987-0") {
                charset = "euc-kr";
            }
        }
        mPreferredCharsets << charset;
    }
}
