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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include <kmkernel.h>
#include <kapplication.h>

static int dummy, dummy2;
static int *Okay = &dummy;
static int *NotOkay = &dummy2;

namespace KMail {

MessageHighlighter::MessageHighlighter( QTextEdit *textEdit, SyntaxMode mode )
    : QSyntaxHighlighter( textEdit ), sMode( mode )
{
  KConfig *config = KMKernel::config();

  // block defines the lifetime of KConfigGroupSaver
  KConfigGroupSaver saver(config, "Reader");
  QColor defaultColor1( 0x00, 0x80, 0x00 ); // defaults from kmreaderwin.cpp
  QColor defaultColor2( 0x00, 0x70, 0x00 );
  QColor defaultColor3( 0x00, 0x60, 0x00 );
  col1 = QColor(kapp->palette().active().text());
  col2 = config->readColorEntry( "QuotedText3", &defaultColor3 );
  col3 = config->readColorEntry( "QuotedText2", &defaultColor2 );
  col4 = config->readColorEntry( "QuotedText1", &defaultColor1 );
  col5 = col1;
}

MessageHighlighter::~MessageHighlighter()
{
}

int MessageHighlighter::highlightParagraph( const QString &text, int )
{
    QString simplified = text;
    simplified = simplified.replace( QRegExp( "\\s" ), "" );
    if ( simplified.startsWith( ">>>>" ) )
	setFormat( 0, text.length(), col1 );
    else if	( simplified.startsWith( ">>>" ) || simplified.startsWith( "> >	>" ) )
	setFormat( 0, text.length(), col2 );
    else if	( simplified.startsWith( ">>" )	|| simplified.startsWith( "> >"	) )
	setFormat( 0, text.length(), col3 );
    else if	( simplified.startsWith( ">" ) )
	setFormat( 0, text.length(), col4 );
    else
	setFormat( 0, text.length(), col5 );
    return 0;
}

SpellChecker::SpellChecker( QTextEdit *textEdit )
: MessageHighlighter( textEdit ), alwaysEndsWithSpace( TRUE )
{
  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Reader");
  QColor c = QColor("red");
  mColor = config->readColorEntry("NewMessage", &c);
}

SpellChecker::~SpellChecker()
{
}

int SpellChecker::highlightParagraph( const QString& text,
				      int endStateOfLastPara )
{
    // leave German, Norwegian, and Swedish alone
    QRegExp norwegian( "[\xc4-\xc6\xd6\xd8\xdc\xdf\xe4-\xe6\xf6\xf8\xfc]" );

    // leave #includes, diffs, and quoted replies alone
    QString diffAndCo( "#+-<>" );

    bool isCode = ( text.stripWhiteSpace().endsWith(";") ||
		    diffAndCo.find(text[0]) != -1 );
    bool isNorwegian = ( text.find(norwegian) != -1 );
    isNorwegian = false; //DS: disable this, hopefully KSpell can handle these languages.

    if ( !text.endsWith(" ") )
	alwaysEndsWithSpace = FALSE;

    MessageHighlighter::highlightParagraph( text, endStateOfLastPara );

    if ( !isCode && !isNorwegian ) {
	int len = text.length();
	if ( alwaysEndsWithSpace )
	    len--;

	currentPos = 0;
	currentWord = "";
	for ( int i = 0; i < len; i++ ) {
	    if ( text[i].isSpace() || text[i] == '-' ) {
		flushCurrentWord();
		currentPos = i + 1;
	    } else {
		currentWord += text[i];
	    }
	}
	if ( !text[len - 1].isLetter() )
	    flushCurrentWord();
    }
    return endStateOfLastPara;
}

QStringList SpellChecker::personalWords()
{
    QStringList l;
    l.append( "KMail" );
    l.append( "KOrganizer" );
    l.append( "KHTML" );
    l.append( "KIO" );
    l.append( "KJS" );
    l.append( "Konqueror" );
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
	bool isPlainWord = TRUE;
	for ( int i = 0; i < (int) currentWord.length(); i++ ) {
	    QChar ch = currentWord[i];
	    if ( ch.upper() == ch ) {
		isPlainWord = FALSE;
		break;
	    }
	}

	if ( isPlainWord && currentWord.length() > 2 &&
	     isMisspelled(currentWord) )
            setFormat( currentPos, currentWord.length(), mColor );
    }
    currentWord = "";
}

QDict<int> DictSpellChecker::dict( 50021 );
QObject *DictSpellChecker::sDictionaryMonitor = 0;

DictSpellChecker::DictSpellChecker( QTextEdit *textEdit )
    : SpellChecker( textEdit )
{
    mRehighlightRequested = false;
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
    if (!mRehighlightRequested) {
	mRehighlightRequested = true;
	QTimer::singleShot(0, this, SLOT(slotRehighlight()));
    }
}

bool DictSpellChecker::isMisspelled( const QString& word )
{
    // Normally isMisspelled would look up a dictionary and return
    // true or false, but kspell is asynchronous and slow so things
    // get tricky...

    // "dict" is used as a cache to store the results of KSpell
    if (!dict.isEmpty() && dict[word] == NotOkay)
	return true;
    if (!dict.isEmpty() && dict[word] == Okay)
	return false;

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
    if (!mRehighlightRequested) {
	mRehighlightRequested = true;
	QTimer::singleShot(0, this, SLOT(slotRehighlight()));
    }
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
    mRehighlightRequested = false;
    rehighlight();
}

void DictSpellChecker::slotDictionaryChanged()
{
    delete mSpell;
    mSpell = 0;
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

void DictSpellChecker::timerEvent(QTimerEvent *e)
{
    if (mSpell && mSpellKey != spellKey()) {
	mSpellKey = spellKey();
	DictSpellChecker::dictionaryChanged();
    }
}

} //namespace KMail

#include "syntaxhighlighter.moc"
