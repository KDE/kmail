/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "kmcomposereditorng.h"
#include "dialog/spellcheckerconfigdialog.h"
#include "job/dndfromarkjob.h"
#include "kmail_debug.h"
#include "kmcomposerwin.h"
#include "kmkernel.h"
#include "util.h"

#include <KPIMTextEdit/RichTextComposerEmailQuoteHighlighter>
#include <KToggleAction>
#include <MessageCore/ColorUtil>
#include <MessageCore/MessageCoreSettings>
#include <QCheckBox>
#include <QMenu>
#include <QMimeData>
#include <Sonnet/ConfigDialog>
#include <sonnet/dictionarycombobox.h>

namespace
{
inline QString textSnippetMimeType()
{
    return QStringLiteral("text/x-kmail-textsnippet");
}
}

KMComposerEditorNg::KMComposerEditorNg(KMComposerWin *win, QWidget *parent)
    : MessageComposer::RichTextComposerNg(parent)
    , mComposerWin(win)
{
    setSpellCheckingConfigFileName(QStringLiteral("kmail2rc"));
    setAutocorrection(KMKernel::self()->composerAutoCorrection());
    createHighlighter();
}

KMComposerEditorNg::~KMComposerEditorNg() = default;

void KMComposerEditorNg::addExtraMenuEntry(QMenu *menu, QPoint pos)
{
    Q_UNUSED(pos)
    const QList<QAction *> lstAct = mComposerWin->pluginToolsActionListForPopupMenu();
    for (QAction *a : lstAct) {
        menu->addSeparator();
        menu->addAction(a);
    }

    menu->addSeparator();
    const QList<KToggleAction *> lstTa = mComposerWin->customToolsList();
    for (KToggleAction *ta : lstTa) {
        menu->addAction(ta);
    }
}

bool KMComposerEditorNg::canInsertFromMimeData(const QMimeData *source) const
{
    if (source->hasImage() && source->hasFormat(QStringLiteral("image/png"))) {
        return true;
    }
    if (source->hasFormat(textSnippetMimeType())) {
        return true;
    }
    if (source->hasUrls()) {
        return true;
    }

    if (DndFromArkJob::dndFromArk(source)) {
        return true;
    }

    return MessageComposer::RichTextComposerNg::canInsertFromMimeData(source);
}

void KMComposerEditorNg::insertFromMimeData(const QMimeData *source)
{
    if (source->hasFormat(textSnippetMimeType())) {
        Q_EMIT insertSnippet();
        return;
    }

    if (!mComposerWin->insertFromMimeData(source, false)) {
        MessageComposer::RichTextComposerNg::insertFromMimeData(source);
    }
}

void KMComposerEditorNg::setHighlighterColors(KPIMTextEdit::RichTextComposerEmailQuoteHighlighter *highlighter)
{
    QColor color1 = MessageCore::ColorUtil::self()->quoteLevel1DefaultTextColor();
    QColor color2 = MessageCore::ColorUtil::self()->quoteLevel2DefaultTextColor();
    QColor color3 = MessageCore::ColorUtil::self()->quoteLevel3DefaultTextColor();
    const QColor misspelled = MessageCore::ColorUtil::self()->misspelledDefaultTextColor();

    if (!MessageCore::MessageCoreSettings::self()->useDefaultColors()) {
        color1 = MessageCore::MessageCoreSettings::self()->quotedText1();
        color2 = MessageCore::MessageCoreSettings::self()->quotedText2();
        color3 = MessageCore::MessageCoreSettings::self()->quotedText3();
    }

    highlighter->setQuoteColor(Qt::black /* ignored anyway */, color1, color2, color3, misspelled);
}

QString KMComposerEditorNg::smartQuote(const QString &msg)
{
    return mComposerWin->smartQuote(msg);
}

void KMComposerEditorNg::showSpellConfigDialog(const QString &configFileName)
{
    Q_UNUSED(configFileName)
    QPointer<SpellCheckerConfigDialog> dialog = new SpellCheckerConfigDialog(this);
    if (!spellCheckingLanguage().isEmpty()) {
        dialog->setLanguage(spellCheckingLanguage());
    }

    if (dialog->exec()) {
        setSpellCheckingLanguage(dialog->language());
    }
    delete dialog;
}

MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus KMComposerEditorNg::convertPlainText(MessageComposer::TextPart *textPart)
{
    if (mComposerWin->convertPlainText(textPart) == MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Converted) {
        return MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::Converted;
    }
    return MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus::NotConverted;
}

bool KMComposerEditorNg::processModifyText(QKeyEvent *event)
{
    if (!mComposerWin->processModifyText(event)) {
        return MessageComposer::RichTextComposerNg::processModifyText(event);
    }
    return true;
}
