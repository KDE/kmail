/*
  Copyright (c) 2015-2017 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmcomposerglobalaction.h"
#include "kmcomposerwin.h"

#include <pimcommon/lineeditwithautocorrection.h>

#include <KLineEdit>

#include <editor/kmcomposereditorng.h>

KMComposerGlobalAction::KMComposerGlobalAction(KMComposerWin *composerWin, QObject *parent)
    : QObject(parent)
    , mComposerWin(composerWin)
{
}

KMComposerGlobalAction::~KMComposerGlobalAction()
{
}

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
