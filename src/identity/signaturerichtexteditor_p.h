/*
    SPDX-FileCopyrightText: 2002-2004 Marc Mutz <mutz@kde.org>
    SPDX-FileCopyrightText: 2007 Tom Albers <tomalbers@kde.nl>
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>
    Author: Stefan Taferner <taferner@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include <KIdentityManagementCore/Signature>

namespace KPIMTextEdit
{
class RichTextComposer;
}

namespace KMail
{

/** Helper methods for rich text signature editing .*/
namespace SignatureRichTextEditor
{
/** Inserts this signature into the given text edit.
 * If the signature is inserted at the beginning, a couple of new
 * lines will be inserted before it, and the cursor is moved to
 * the beginning. Otherwise, the cursor position is preserved.
 * For undo/redo, this is treated as one operation.
 *
 * Rich text mode of the text edit will be enabled if the signature is in
 * inlined HTML format.
 *
 * If this signature uses images, they will be added automatically.
 *
 * @param placement defines where in the text edit the signature should be
 *                  inserted.
 * @param addedText defines which other texts should be added to the signature
 * @param textEdit the signature will be inserted into this text edit.
 *
 * @since 4.9
 */
void insertIntoTextEdit(const KIdentityManagementCore::Signature &sig,
                        KIdentityManagementCore::Signature::Placement placement,
                        KIdentityManagementCore::Signature::AddedText addedText,
                        KPIMTextEdit::RichTextComposer *textEdit,
                        bool forceDisplay = false);
}

}
