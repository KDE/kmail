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
#include "widgets/kactionmenuchangecase.h"
#include <KPIMTextEdit/EMailQuoteHighlighter>
#include "messagecore/settings/globalsettings.h"
#include <Sonnet/ConfigDialog>
#include <composer-ng/richtextcomposeremailquotehighlighter.h>

KMComposerEditorNg::KMComposerEditorNg(KMComposeWin *win, QWidget *parent)
    : MessageComposer::RichTextComposer(parent),
      mComposerWin(win)
{
    setSpellCheckingConfigFileName(QStringLiteral("kmail2rc"));
    setAutocorrection(KMKernel::self()->composerAutoCorrection());
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
    menu->addAction(mComposerWin->translateAction());
    menu->addAction(mComposerWin->generateShortenUrlAction());
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
    QColor color1 = KMail::Util::quoteL1Color();
    QColor color2 = KMail::Util::quoteL2Color();
    QColor color3 = KMail::Util::quoteL3Color();
    QColor misspelled = KMail::Util::misspelledColor();

    if (!MessageCore::GlobalSettings::self()->useDefaultColors()) {
        color1 = MessageCore::GlobalSettings::self()->quotedText1();
        color2 = MessageCore::GlobalSettings::self()->quotedText2();
        color3 = MessageCore::GlobalSettings::self()->quotedText3();
        misspelled = MessageCore::GlobalSettings::self()->misspelledColor();
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
    if (dialog.exec()) {
        setSpellCheckingLanguage(dialog.language());
    }
}
