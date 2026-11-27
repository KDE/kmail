/*
    SPDX-FileCopyrightText: 2002-2004 Marc Mutz <mutz@kde.org>
    SPDX-FileCopyrightText: 2007 Tom Albers <tomalbers@kde.nl>
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "signaturerichtexteditor_p.h"

#include <KPIMTextEdit/RichTextComposer>
#include <KPIMTextEdit/RichTextComposerControler>
#include <KPIMTextEdit/RichTextComposerImages>

using namespace Qt::Literals::StringLiterals;
using namespace KMail;

static bool isCursorAtEndOfLine(const QTextCursor &cursor)
{
    QTextCursor testCursor = cursor;
    testCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    return !testCursor.hasSelection();
}

static void insertSignatureHelper(const QString &signature,
                                  KPIMTextEdit::RichTextComposer *textEdit,
                                  KIdentityManagementCore::Signature::Placement placement,
                                  bool isHtml,
                                  bool addNewlines)
{
    if (!signature.isEmpty()) {
        // Save the modified state of the document, as inserting a signature
        // shouldn't change this. Restore it at the end of this function.
        bool isModified = textEdit->document()->isModified();

        // Move to the desired position, where the signature should be inserted
        QTextCursor cursor = textEdit->textCursor();
        QTextCursor oldCursor = cursor;
        cursor.beginEditBlock();

        if (placement == KIdentityManagementCore::Signature::End) {
            cursor.movePosition(QTextCursor::End);
        } else if (placement == KIdentityManagementCore::Signature::Start) {
            cursor.movePosition(QTextCursor::Start);
        } else if (placement == KIdentityManagementCore::Signature::AtCursor) {
            cursor.movePosition(QTextCursor::StartOfLine);
        }
        textEdit->setTextCursor(cursor);

        QString lineSep;
        if (addNewlines) {
            if (isHtml) {
                lineSep = u"<br>"_s;
            } else {
                lineSep = u'\n';
            }
        }

        // Insert the signature and newlines depending on where it was inserted.
        int newCursorPos = -1;
        QString headSep;
        QString tailSep;

        if (placement == KIdentityManagementCore::Signature::End) {
            // There is one special case when re-setting the old cursor: The cursor
            // was at the end. In this case, QTextEdit has no way to know
            // if the signature was added before or after the cursor, and just
            // decides that it was added before (and the cursor moves to the end,
            // but it should not when appending a signature). See bug 167961
            if (oldCursor.position() == textEdit->toPlainText().length()) {
                newCursorPos = oldCursor.position();
            }
            headSep = lineSep;
        } else if (placement == KIdentityManagementCore::Signature::Start) {
            // When prepending signatures, add a couple of new lines before
            // the signature, and move the cursor to the beginning of the QTextEdit.
            // People tends to insert new text there.
            newCursorPos = 0;
            headSep = lineSep + lineSep;
            if (!isCursorAtEndOfLine(cursor)) {
                tailSep = lineSep;
            }
        } else if (placement == KIdentityManagementCore::Signature::AtCursor) {
            if (!isCursorAtEndOfLine(cursor)) {
                tailSep = lineSep;
            }
        }

        const QString full_signature = headSep + signature + tailSep;
        if (isHtml) {
            textEdit->insertHtml(full_signature);
        } else {
            textEdit->insertPlainText(full_signature);
        }

        cursor.endEditBlock();
        if (newCursorPos != -1) {
            oldCursor.setPosition(newCursorPos);
        }

        textEdit->setTextCursor(oldCursor);
        textEdit->ensureCursorVisible();

        textEdit->document()->setModified(isModified);

        if (isHtml) {
            textEdit->activateRichText();
        }
    }
}

void SignatureRichTextEditor::insertIntoTextEdit(const KIdentityManagementCore::Signature &sig,
                                                 KIdentityManagementCore::Signature::Placement placement,
                                                 KIdentityManagementCore::Signature::AddedText addedText,
                                                 KPIMTextEdit::RichTextComposer *textEdit,
                                                 bool forceDisplay)
{
    if (!forceDisplay) {
        if (!sig.isEnabledSignature()) {
            return;
        }
    }
    QString signature;
    if (addedText & KIdentityManagementCore::Signature::AddSeparator) {
        signature = sig.withSeparator();
    } else {
        signature = sig.rawText();
    }
    insertSignatureHelper(signature,
                          textEdit,
                          placement,
                          (sig.isInlinedHtml() && sig.type() == KIdentityManagementCore::Signature::Inlined),
                          (addedText & KIdentityManagementCore::Signature::AddNewLines));

    // We added the text of the signature above, now it is time to add the images as well.
    if (sig.isInlinedHtml()) {
        const auto embeddedImgs = sig.embeddedImages();
        for (const KIdentityManagementCore::Signature::EmbeddedImagePtr &image : embeddedImgs) {
            textEdit->composerControler()->composerImages()->loadImage(image->image, image->name, image->name);
        }
    }
}
