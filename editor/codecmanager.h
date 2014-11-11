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

#ifndef CODECMANAGER_H
#define CODECMANAGER_H

#include <QByteArray>

class CodecManagerPrivate;

class CodecManager
{
public:
    /**
      Returns the CodecManager instance.
    */
    static CodecManager *self();

    /**
      A list of preferred charsets to use when composing messages.
    */
    QList<QByteArray> preferredCharsets() const;

    /**
      Re-read the preferred charsets from settings.
      TODO KConfig XT would probably make this unnecessary
    */
    void updatePreferredCharsets();

private:
    friend class CodecManagerPrivate;
    CodecManagerPrivate *const d;

    // Singleton.  The only instance lives in sInstance->instance
    explicit CodecManager(CodecManagerPrivate *dd);
    //~CodecManager();
};

#endif /* CODECMANAGER_H */
