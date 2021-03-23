/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

// Qt
#include <QVector>

// KDE
#include <KCodecAction>

// TODO since the reader is now in a separate lib, we can probably have this
// class for the composer only.  The reader can use KCodecAction directly anyway.
class CodecAction : public KCodecAction
{
    Q_OBJECT

public:
    enum Mode {
        ComposerMode, ///< Used in the composer.  Show a 'Default' menu entry,
        ///  which uses one of the preferred codecs.  Also show 'us-ascii'.
        ReaderMode ///< Used in the reader.  Show an 'Auto' entry for each language,
        ///  and detect any charset.
    };

    explicit CodecAction(Mode mode, QObject *parent = nullptr);
    ~CodecAction() override;

    /**
      The name of the charset, if a specific encoding was selected, or a list
      containing the names of the preferred charsets, if 'Default' was selected in Composer
      mode.  In Reader mode it probably makes more sense to use KCodecAction::currentCodec()
      and KCodecAction::currentAutoDetectScript().
    */
    Q_REQUIRED_RESULT QVector<QByteArray> mimeCharsets() const;

    void setAutoCharset();
    void setCharset(const QByteArray &charset);

private:
    const CodecAction::Mode mMode;
};

