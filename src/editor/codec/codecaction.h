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

#ifndef CODECACTION_H
#define CODECACTION_H

// Qt
#include <QList>

// KDE
#include <kcodecaction.h>

// TODO since the reader is now in a separate lib, we can probably have this
// class for the composer only.  The reader can use KCodecAction directly anyway.
class CodecAction : public KCodecAction
{
    Q_OBJECT

public:
    enum Mode {
        ComposerMode,         ///< Used in the composer.  Show a 'Default' menu entry,
        ///  which uses one of the preferred codecs.  Also show 'us-ascii'.
        ReaderMode            ///< Used in the reader.  Show an 'Auto' entry for each language,
        ///  and detect any charset.
    };

    explicit CodecAction(Mode mode, QObject *parent = nullptr);
    ~CodecAction();

    /**
      The name of the charset, if a specific encoding was selected, or a list
      containing the names of the preferred charsets, if 'Default' was selected in Composer
      mode.  In Reader mode it probably makes more sense to use KCodecAction::currentCodec()
      and KCodecAction::currentAutoDetectScript().
    */
    QList<QByteArray> mimeCharsets() const;

    void setAutoCharset();
    void setCharset(const QByteArray &charset);
private:
    class Private;
    friend class Private;
    Private *const d;
};

#endif
