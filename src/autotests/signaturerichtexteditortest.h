/*
    SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
    SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class SignatureRichTextEditorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testTextEditInsertion();
    void testBug167961();
    void testImages();
    void testLinebreaks();
};
