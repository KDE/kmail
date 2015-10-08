/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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
#include "kmcomposewin.h"
#include "kmkernel.h"
#include "util.h"
#include "kmail_debug.h"

#include <qmenu.h>
#include <QDebug>
#include <KToggleAction>
#include <QMimeData>
#include <QCheckBox>
#include <QLabel>
#include "pimcommon/kactionmenuchangecase.h"
#include <KPIMTextEdit/EMailQuoteHighlighter>
#include "MessageCore/MessageCoreSettings"
#include <Sonnet/ConfigDialog>
#include <messagecomposer/richtextcomposeremailquotehighlighter.h>
#include <sonnet/dictionarycombobox.h>

KMComposerEditorNg::KMComposerEditorNg(KMComposeWin *win, QWidget *parent)
    : MessageComposer::RichTextComposer(parent),
      mComposerWin(win)
{
    setSpellCheckingConfigFileName(QStringLiteral("kmail2rc"));
    setAutocorrection(KMKernel::self()->composerAutoCorrection());
    createHighlighter();
}

KMComposerEditorNg::~KMComposerEditorNg()
{

}

void KMComposerEditorNg::addExtraMenuEntry(QMenu *menu, const QPoint &pos)
{
    Q_UNUSED(pos);
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        menu->addSeparator();
        menu->addAction(mComposerWin->changeCaseMenu());
    }
    menu->addSeparator();
    Q_FOREACH (KToggleAction *ta, mComposerWin->customToolsList()) {
        menu->addAction(ta);
    }
}

bool KMComposerEditorNg::canInsertFromMimeData(const QMimeData *source) const
{
    if (source->hasImage() && source->hasFormat(QStringLiteral("image/png"))) {
        return true;
    }
    if (source->hasFormat(QStringLiteral("text/x-kmail-textsnippet"))) {
        return true;
    }
    if (source->hasUrls()) {
        return true;
    }

    return MessageComposer::RichTextComposer::canInsertFromMimeData(source);
}

void KMComposerEditorNg::insertFromMimeData(const QMimeData *source)
{
    if (source->hasFormat(QStringLiteral("text/x-kmail-textsnippet"))) {
        Q_EMIT insertSnippet();
        return;
    }

    if (!mComposerWin->insertFromMimeData(source, false)) {
        MessageComposer::RichTextComposer::insertFromMimeData(source);
    }
}

void KMComposerEditorNg::setHighlighterColors(MessageComposer::RichTextComposerEmailQuoteHighlighter *highlighter)
{
    QColor color1 = MessageCore::Util::quoteLevel1DefaultTextColor();
    QColor color2 = MessageCore::Util::quoteLevel2DefaultTextColor();
    QColor color3 = MessageCore::Util::quoteLevel3DefaultTextColor();
    QColor misspelled = MessageCore::Util::misspelledDefaultTextColor();

    if (!MessageCore::MessageCoreSettings::self()->useDefaultColors()) {
        color1 = MessageCore::MessageCoreSettings::self()->quotedText1();
        color2 = MessageCore::MessageCoreSettings::self()->quotedText2();
        color3 = MessageCore::MessageCoreSettings::self()->quotedText3();
        misspelled = MessageCore::MessageCoreSettings::self()->misspelledColor();
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
