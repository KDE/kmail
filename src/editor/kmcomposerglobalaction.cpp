/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "kmcomposerglobalaction.h"
#include "kmcomposerwin.h"

#include <PimCommon/LineEditWithAutoCorrection>

#include <KLineEdit>

#include <editor/kmcomposereditorng.h>

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
    //} else if (::qobject_cast<KLineEdit *>(fw)) {
    // Don't insert emoticon in mail linedit
    // static_cast<KLineEdit *>(fw)->insert(str);
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
    // Don't insert text in mail linedit
    //} else if (::qobject_cast<KLineEdit *>(fw)) {
}
