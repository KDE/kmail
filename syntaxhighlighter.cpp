/**
 * syntaxhighlighter.cpp
 *
 * Copyright (c) 2003 Trolltech AS
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "syntaxhighlighter.h"
#include <klocale.h>
#include <qcolor.h>
#include <qregexp.h>
#include <qsyntaxhighlighter.h>
#include <qtimer.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kspell.h>
#include "kmkernel.h"
#include <kapplication.h>

static int dummy, dummy2, dummy3;
static int *Okay = &dummy;
static int *NotOkay = &dummy2;
static int *Ignore = &dummy3;

namespace KMail {

MessageHighlighter::MessageHighlighter( QTextEdit *textEdit,
					bool colorQuoting, 
					QColor depth0,
					QColor depth1,
					QColor depth2,
					QColor depth3,
					SyntaxMode mode )
    : QSyntaxHighlighter( textEdit ), sMode( mode )
{
    mEnabled = colorQuoting;
    col1 = depth0;
    col2 = depth1;
    col3 = depth2;
    col4 = depth3;
    col5 = col1;
}

MessageHighlighter::~MessageHighlighter()
{
}

int MessageHighlighter::highlightParagraph( const QString &text, int )
{
    if (!mEnabled)
	return 0;
    QString simplified = text;
    simplified = simplified.replace( QRegExp( "\\s" ), "" ).replace( "|", ">" );
    while ( simplified.startsWith( ">>>>" ) )
	simplified = simplified.mid(3);
    if	( simplified.startsWith( ">>>" ) || simplified.startsWith( "> >	>" ) )
	setFormat( 0, text.length(), col2 );
    else if	( simplified.startsWith( ">>" )	|| simplified.startsWith( "> >"	) )
	setFormat( 0, text.length(), col3 );
    else if	( simplified.startsWith( ">" ) )
	setFormat( 0, text.length(), col4 );
    else
	setFormat( 0, text.length(), col5 );
    return 0;
}

SpellChecker::SpellChecker( QTextEdit *textEdit,
			    QColor spellColor, 
			    bool colorQuoting, 
			    QColor depth0,
			    QColor depth1,
			    QColor depth2,
			    QColor depth3 )
    : MessageHighlighter( textEdit, colorQuoting, depth0, depth1, depth2, depth3 ),
      alwaysEndsWithSpace( TRUE )
{
  mColor = spellColor;
  mIntraWordEditing = false;
}

SpellChecker::~SpellChecker()
{
}

int SpellChecker::highlightParagraph( const QString& text,
				      int parano )
{
    if (parano == -2)
	parano = 0;
    // leave #includes, diffs, and quoted replies alone
    QString diffAndCo( ">|" );

    bool isCode = diffAndCo.find(text[0]) != -1;

    if ( !text.endsWith(" ") )
	alwaysEndsWithSpace = FALSE;

    MessageHighlighter::highlightParagraph( text, -2 );

    if ( !isCode ) {
        int para, index;
	textEdit()->getCursorPosition( &para, &index );
	int len = text.length();
	if ( alwaysEndsWithSpace )
	    len--;

	currentPos = 0;
	currentWord = "";
	for ( int i = 0; i < len; i++ ) {
	    if ( text[i].isSpace() || text[i] == '-' ) {
		if ((para != parano) ||
		    !intraWordEditing() ||
		    (i - currentWord.length() > (uint)index) ||
		    (i < index)) {
		    flushCurrentWord();
		} else {
		    currentWord = "";
		}
		currentPos = i + 1;
	    } else {
		currentWord += text[i];
	    }
	}
	if ( !text[len - 1].isLetter() ||
	     (uint)(index + 1) != text.length() ||
	     para != parano)
	    flushCurrentWord();
    }
    return ++parano;
}

QStringList SpellChecker::personalWords()
{
    QStringList l;
    l.append( "KMail" );
    l.append( "KOrganizer" );
    l.append( "KAddressBook" );
    l.append( "KHTML" );
    l.append( "KIO" );
    l.append( "KJS" );
    l.append( "Konqueror" );
    l.append( "KSpell" );
    l.append( "Kontact" );
    l.append( "Qt" );
    return l;
}

void SpellChecker::flushCurrentWord()
{
    while ( currentWord[0].isPunct() ) {
	currentWord = currentWord.mid( 1 );
	currentPos++;
    }

    QChar ch;
    while ( (ch = currentWord[(int) currentWord.length() - 1]).isPunct() &&
	     ch != '(' && ch != '@' )
	currentWord.truncate( currentWord.length() - 1 );

    if ( !currentWord.isEmpty() ) {
	if ( isMisspelled(currentWord) )
	    setFormat( currentPos, currentWord.length(), mColor );
//	    setMisspelled( currentPos, currentWord.length(), true );
    }
    currentWord = "";
}

QDict<int> DictSpellChecker::dict( 50021 );
QObject *DictSpellChecker::sDictionaryMonitor = 0;

DictSpellChecker::DictSpellChecker( QTextEdit *textEdit,
				    bool spellCheckingActive , 
				    bool autoEnable, 
				    QColor spellColor, 
				    bool colorQuoting, 
				    QColor depth0,
				    QColor depth1,
				    QColor depth2,
				    QColor depth3 )
    : SpellChecker( textEdit, spellColor,
		    colorQuoting, depth0, depth1, depth2, depth3 )
{
    mAutoReady = false;
    mWordCount = 0;
    mErrorCount = 0;
    mActive = spellCheckingActive;
    mAutomatic = autoEnable;
    textEdit->installEventFilter( this );
    textEdit->viewport()->installEventFilter( this );
    rehighlightRequest = new QTimer();
    connect(rehighlightRequest,SIGNAL(timeout()),
	    this,SLOT(slotRehighlight()));
    mSpell = 0;
    mSpellKey = spellKey();
    if (!sDictionaryMonitor)
	sDictionaryMonitor = new QObject();
    slotDictionaryChanged();
    startTimer(2*1000);
}

DictSpellChecker::~DictSpellChecker()
{
    delete mSpell;
}

void DictSpellChecker::slotSpellReady( KSpell *spell )
{
    connect( sDictionaryMonitor, SIGNAL( destroyed() ),
	     this, SLOT( slotDictionaryChanged() ));
    mSpell = spell;
    QStringList l = SpellChecker::personalWords();
    for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
        mSpell->addPersonal( *it );
    }
    connect( spell, SIGNAL( misspelling (const QString &, const QStringList &, unsigned int) ),
	     this, SLOT( slotMisspelling (const QString &, const QStringList &, unsigned int)));
    rehighlightRequest->start(0, true);
}

bool DictSpellChecker::isMisspelled( const QString& word )
{
    // Normally isMisspelled would look up a dictionary and return
    // true or false, but kspell is asynchronous and slow so things
    // get tricky...

    // For auto detection ignore signature and reply prefix
    if (!mAutoReady)
	mAutoIgnoreDict.replace( word, Ignore );

    // "dict" is used as a cache to store the results of KSpell
    if (!dict.isEmpty() && dict[word] == NotOkay) {
	if (mAutoReady && (mAutoDict[word] != NotOkay)) {
	    if (!mAutoIgnoreDict[word]) {
		++mErrorCount;
		if (mAutoDict[word] != Okay)
		    ++mWordCount;
	    }
	    mAutoDict.replace( word, NotOkay );
	}
	return true && mActive;
    }
    if (!dict.isEmpty() && dict[word] == Okay) {
	if (mAutoReady && !mAutoDict[word]) {
	    mAutoDict.replace( word, Okay );
	    if (!mAutoIgnoreDict[word])
		++mWordCount;
	}
	return false;
    }

    // there is no 'spelt correctly' signal so default to Okay
    dict.replace( word, Okay );

    // yes I tried checkWord, the docs lie and it didn't give useful signals :-(
    if (mSpell)
	mSpell->check( word, false );
    return false;
}

void DictSpellChecker::slotMisspelling (const QString & originalword, const QStringList & suggestions, unsigned int)
{
    kdDebug(5006) << suggestions.join(" ").latin1() << endl;
    dict.replace( originalword, NotOkay );

    // this is slow but since kspell is async this will have to do for now
    rehighlightRequest->start(0, true);
}

void DictSpellChecker::dictionaryChanged()
{
    QObject *oldMonitor = sDictionaryMonitor;
    sDictionaryMonitor = new QObject();
    dict.clear();
    delete oldMonitor;
}

void DictSpellChecker::slotRehighlight()
{
    rehighlight();
    QTimer::singleShot(0, this, SLOT(slotAutoDetection()));
}

void DictSpellChecker::slotDictionaryChanged()
{
    delete mSpell;
    mSpell = 0;
    mWordCount = 0;
    mErrorCount = 0;
    mAutoDict.clear();
    new KSpell(0, i18n("Incremental Spellcheck - KMail"), this,
			 SLOT(slotSpellReady(KSpell*)));
}

QString DictSpellChecker::spellKey()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs(config,"KSpell");
    config->reparseConfiguration();
    QString key;
    key += QString::number(config->readNumEntry("KSpell_NoRootAffix", 0));
    key += '/';
    key += QString::number(config->readNumEntry("KSpell_RunTogether", 0));
    key += '/';
    key += config->readEntry("KSpell_Dictionary", "");
    key += '/';
    key += QString::number(config->readNumEntry("KSpell_DictFromList", FALSE));
    key += '/';
    key += QString::number(config->readNumEntry ("KSpell_Encoding", KS_E_ASCII));
    key += '/';
    key += QString::number(config->readNumEntry ("KSpell_Client", KS_CLIENT_ISPELL));
    return key;
}


// Automatic spell checking support
// In auto spell checking mode disable as-you-type spell checking
// iff more than one third of words are spelt incorrectly.
//
// Words in the signature and reply prefix are ignored.
// Only unique words are counted.
void DictSpellChecker::slotAutoDetection()
{
    if (!mAutoReady)
	return;
    bool savedActive = mActive;
    if (mAutomatic) {
	if (mActive && (mErrorCount * 3 >= mWordCount))
	    mActive = false;
	else if (!mActive && (mErrorCount * 3 < mWordCount))
	    mActive = true;
    }
    if (mActive != savedActive) {
	if (mWordCount > 1)
	    if (mActive)
		emit activeChanged( i18n("As-you-type spell checking enabled.") );
	    else
		emit activeChanged( i18n("Too many misspelled words. "
                                         "As-you-type spell checking disabled.") );
	rehighlightRequest->start(100, true);
    }
}

bool DictSpellChecker::eventFilter(QObject* o, QEvent* e)
{
    if (o == textEdit() && (e->type() == QEvent::FocusIn)) {
	if (mSpell && mSpellKey != spellKey()) {
	    mSpellKey = spellKey();
	    DictSpellChecker::dictionaryChanged();
	}
    }

    if (o == textEdit() && (e->type() == QEvent::KeyPress)) {
	QKeyEvent *k = (QKeyEvent*)e;
	mAutoReady = true;
	if (k->key() == Key_Enter ||
	    k->key() == Key_Return ||
	    k->key() == Key_Up ||
	    k->key() == Key_Down ||
	    k->key() == Key_Left ||
	    k->key() == Key_Right ||
	    k->key() == Key_PageUp ||
	    k->key() == Key_PageDown ||
	    k->key() == Key_Home) {
	    if (intraWordEditing()) {
		setIntraWordEditing(false);
		rehighlightRequest->start(0, true);
	    }
	} else {
	    setIntraWordEditing(true);
	}
	if (k->key() == Key_Space ||
	    k->key() == Key_Enter ||
	    k->key() == Key_Return)
	    QTimer::singleShot(0, this, SLOT(slotAutoDetection()));
    }

    if (o == textEdit()->viewport() &&
	(e->type() == QEvent::MouseButtonPress)) {
	if (intraWordEditing()) {
	    setIntraWordEditing(false);
	    rehighlightRequest->start(0, true);
	}
    }

    return false;
}

} //namespace KMail

#include "syntaxhighlighter.moc"
