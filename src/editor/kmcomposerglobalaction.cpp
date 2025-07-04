/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kmcomposerglobalaction.h"
#include "kmcomposerwin.h"

#include <PimCommon/LineEditWithAutoCorrection>

#include <KLineEdit>

#include "editor/kmcomposereditorng.h"

KMComposerGlobalAction::KMComposerGlobalAction(KMComposerWin *composerWin, QObject *parent)
    : QObject(parent)
    , mComposerWin(composerWin)
{
}

KMComposerGlobalAction::~KMComposerGlobalAction() = default;

void KMComposerGlobalAction::slotUndo()
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->undo();
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->undo();
    } else if (::qobject_cast<KLineEdit *>(fw)) {
        static_cast<KLineEdit *>(fw)->undo();
    }
}

void KMComposerGlobalAction::slotRedo()
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->redo();
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->redo();
    } else if (::qobject_cast<KLineEdit *>(fw)) {
        static_cast<KLineEdit *>(fw)->redo();
    }
}

void KMComposerGlobalAction::slotCut()
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->cut();
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->cut();
    } else if (::qobject_cast<KLineEdit *>(fw)) {
        static_cast<KLineEdit *>(fw)->cut();
    }
}

void KMComposerGlobalAction::slotCopy()
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->copy();
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->copy();
    } else if (::qobject_cast<KLineEdit *>(fw)) {
        static_cast<KLineEdit *>(fw)->copy();
    }
}

void KMComposerGlobalAction::slotPaste()
{
    QWidget *const fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }
    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->paste();
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->paste();
    } else if (::qobject_cast<KLineEdit *>(fw)) {
        static_cast<KLineEdit *>(fw)->paste();
    }
}

void KMComposerGlobalAction::slotMarkAll()
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->selectAll();
    } else if (::qobject_cast<KLineEdit *>(fw)) {
        static_cast<KLineEdit *>(fw)->selectAll();
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->selectAll();
    }
}

void KMComposerGlobalAction::slotInsertEmoticon(const QString &str)
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->insertPlainText(str);
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->insertPlainText(str);
    }
}

void KMComposerGlobalAction::slotInsertText(const QString &str)
{
    QWidget *fw = mComposerWin->focusWidget();
    if (!fw) {
        return;
    }

    if (::qobject_cast<PimCommon::LineEditWithAutoCorrection *>(fw)) {
        static_cast<PimCommon::LineEditWithAutoCorrection *>(fw)->insertPlainText(str);
    } else if (::qobject_cast<KMComposerEditorNg *>(fw)) {
        static_cast<QTextEdit *>(fw)->insertPlainText(str);
    }
}

#include "moc_kmcomposerglobalaction.cpp"
