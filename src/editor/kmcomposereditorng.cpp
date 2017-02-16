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

#include "kmcomposereditorng.h"
#include "kmcomposerwin.h"
#include "kmkernel.h"
#include "util.h"
#include "kmail_debug.h"

#include <qmenu.h>
#include <KToggleAction>
#include <QMimeData>
#include <QCheckBox>
#include <QLabel>
#include <MessageCore/MessageCoreSettings>
#include <Sonnet/ConfigDialog>
#include <KPIMTextEdit/RichTextComposerEmailQuoteHighlighter>
#include <sonnet/dictionarycombobox.h>

namespace
{
inline QString textSnippetMimeType()
{
    return QStringLiteral("text/x-kmail-textsnippet");
}
}

KMComposerEditorNg::KMComposerEditorNg(KMComposerWin *win, QWidget *parent)
    : MessageComposer::RichTextComposerNg(parent),
      mComposerWin(win)
{
    setSpellCheckingConfigFileName(QStringLiteral("kmail2rc"));
    setAutocorrection(KMKernel::self()->composerAutoCorrection());
    createHighlighter();
}

KMComposerEditorNg::~KMComposerEditorNg()
{

}

void KMComposerEditorNg::addExtraMenuEntry(QMenu *menu, QPoint pos)
{
    Q_UNUSED(pos);
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
    QColor misspelled = MessageCore::ColorUtil::self()->misspelledDefaultTextColor();

    if (!MessageCore::MessageCoreSettings::self()->useDefaultColors()) {
        color1 = MessageCore::MessageCoreSettings::self()->quotedText1();
        color2 = MessageCore::MessageCoreSettings::self()->quotedText2();
        color3 = MessageCore::MessageCoreSettings::self()->quotedText3();
    }

    highlighter->setQuoteColor(Qt::black /* ignored anyway */,
                               color1, color2, color3, misspelled);
}

QString KMComposerEditorNg::smartQuote(const QString &msg)
{
    return mComposerWin->smartQuote(msg);
}

void KMComposerEditorNg::showSpellConfigDialog(const QString &configFileName)
{
    Q_UNUSED(configFileName);
    Sonnet::ConfigDialog dialog(this);
    if (!spellCheckingLanguage().isEmpty()) {
        dialog.setLanguage(spellCheckingLanguage());
    }
    // Hackish way to hide the "Enable spell check by default" checkbox
    // Our highlighter ignores this setting, so we should not expose its UI
    QCheckBox *enabledByDefaultCB = dialog.findChild<QCheckBox *>(QStringLiteral("m_checkerEnabledByDefaultCB"));
    if (enabledByDefaultCB) {
        enabledByDefaultCB->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any checkbox named 'm_checkerEnabledByDefaultCB'. Sonnet::ConfigDialog must have changed!";
    }
    QLabel *textLabel = dialog.findChild<QLabel *>(QStringLiteral("textLabel1"));
    if (textLabel) {
        textLabel->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any label named 'textLabel'. Sonnet::ConfigDialog must have changed!";
    }
    Sonnet::DictionaryComboBox *dictionaryComboBox = dialog.findChild<Sonnet::DictionaryComboBox *>(QStringLiteral("m_langCombo"));
    if (dictionaryComboBox) {
        dictionaryComboBox->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any Sonnet::DictionaryComboBox named 'dictionaryComboBox'. Sonnet::ConfigDialog must have changed!";
    }

    if (dialog.exec()) {
        setSpellCheckingLanguage(dialog.language());
    }
}
