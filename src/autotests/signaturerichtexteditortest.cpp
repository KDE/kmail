/*
    SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
    SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2008 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "signaturerichtexteditortest.h"

#include "identity/signaturerichtexteditor_p.h"
#include <KActionCollection>
#include <KPIMTextEdit/RichTextComposer>
#include <KPIMTextEdit/RichTextComposerControler>
#include <KPIMTextEdit/RichTextComposerImages>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::Literals::StringLiterals;
using namespace KIdentityManagementCore;
using namespace KMail;

QTEST_MAIN(SignatureRichTextEditorTest)

static void setCursorPos(QTextEdit &edit, int pos)
{
    QTextCursor cursor(edit.document());
    cursor.setPosition(pos);
    edit.setTextCursor(cursor);
}

void SignatureRichTextEditorTest::testTextEditInsertion()
{
    KPIMTextEdit::RichTextComposer edit;
    edit.createActions(new KActionCollection(this));
    Signature sig;
    sig.setEnabledSignature(true);
    sig.setText(u"Hello World"_s);

    // Test inserting signature at start, with separators. Make sure two new
    // lines are inserted before the signature

    edit.setPlainText(u"Bla Bla"_s);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::Start, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.textMode(), KPIMTextEdit::RichTextComposer::Plain);
    QCOMPARE(edit.toPlainText(), u"\n\n-- \nHello World\nBla Bla"_s);

    // Test inserting signature at end. make sure cursor position is preserved
    edit.clear();
    edit.setPlainText(u"Bla Bla"_s);
    setCursorPos(edit, 4);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::End, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.toPlainText(), u"Bla Bla\n-- \nHello World"_s);
    QCOMPARE(edit.textCursor().position(), 4);

    // test inserting a signature at cursor position. make sure the signature is inserted
    // to the beginning of the line and make sure modified state is preserved
    edit.clear();
    edit.setPlainText(u"Bla Bla"_s);
    setCursorPos(edit, 4);
    edit.document()->setModified(false);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::AtCursor, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.toPlainText(), u"-- \nHello World\nBla Bla"_s);
    QCOMPARE(edit.textCursor().position(), 20);
    QVERIFY(!edit.document()->isModified());

    // make sure undo undoes everything in one go
    edit.undo();
    QCOMPARE(edit.toPlainText(), u"Bla Bla"_s);

    // test inserting signature without separator.
    // make sure cursor position and modified state is preserved.
    edit.clear();
    edit.setPlainText(u"Bla Bla"_s);
    setCursorPos(edit, 4);
    edit.document()->setModified(true);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::End, Signature::AddNewLines, &edit);
    QCOMPARE(edit.toPlainText(), u"Bla Bla\nHello World"_s);
    QCOMPARE(edit.textCursor().position(), 4);
    QVERIFY(edit.document()->isModified());

    sig.setText(u"Hello<br>World"_s);
    sig.setInlinedHtml(true);

    // test that html signatures turn html on and have correct line endings (<br> vs \n)
    edit.clear();
    edit.setPlainText(u"Bla Bla"_s);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::End, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.textMode(), KPIMTextEdit::RichTextComposer::Rich);
    QCOMPARE(edit.toPlainText(), u"Bla Bla\n-- \nHello\nWorld"_s);
}

void SignatureRichTextEditorTest::testBug167961()
{
    KPIMTextEdit::RichTextComposer edit;
    edit.createActions(new KActionCollection(this));
    Signature sig;
    sig.setEnabledSignature(true);
    sig.setText(u"BLA"_s);

    // Test that the cursor is still at the start when appending a sig into
    // an empty text edit
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::End, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.textCursor().position(), 0);

    // When prepending a sig, the cursor should also be at the start, see bug 211634
    edit.clear();
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::Start, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.textCursor().position(), 0);
}

void SignatureRichTextEditorTest::testLinebreaks()
{
    Signature sig;
    sig.setEnabledSignature(true);
    sig.setType(Signature::Inlined);
    sig.setInlinedHtml(true);
    sig.setText(u"Hans Mustermann<br>Musterstr. 42"_s);

    KPIMTextEdit::RichTextComposer edit;
    edit.createActions(new KActionCollection(this));
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::Start, Signature::AddNothing, &edit);
    QCOMPARE(edit.toPlainText(), u"Hans Mustermann\nMusterstr. 42"_s);

    edit.clear();
    sig.setText(u"<p>Hans Mustermann</p><br>Musterstr. 42"_s);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::Start, Signature::AddSeparator, &edit);
    QEXPECT_FAIL("", "This test is probably bogus, since Qt doesn't seem to produce HTML like this anymore.", Continue);
    QCOMPARE(edit.toPlainText(), u"-- \nHans Mustermann\nMusterstr. 42"_s);
}

void SignatureRichTextEditorTest::testImages()
{
    QTemporaryDir imageDirectory;
    QVERIFY(imageDirectory.isValid());

    KPIMTextEdit::RichTextComposer edit;
    edit.createActions(new KActionCollection(this));
    QImage image(16, 16, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::green);

    Signature sig;
    sig.setEnabledSignature(true);
    sig.setInlinedHtml(true);
    sig.setImageLocation(imageDirectory.path());
    sig.setText(u"Bla<img src=\"folder-new.png\">Bla<img src=\"arrow-up.png\">Bla"_s);
    sig.addImage(image, u"folder-new.png"_s);
    sig.addImage(image, u"arrow-up.png"_s);
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::End, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.composerControler()->composerImages()->embeddedImages().count(), 2);

    edit.clear();
    sig.setText(u"<img src=\"folder-new.png\">"_s);
    sig.setEmbeddedImages({sig.embeddedImages().first()});
    SignatureRichTextEditor::insertIntoTextEdit(sig, Signature::End, Signature::AddSeparator | Signature::AddNewLines, &edit);
    QCOMPARE(edit.composerControler()->composerImages()->embeddedImages().count(), 1);
}

#include "moc_signaturerichtexteditortest.cpp"
