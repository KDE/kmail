/**
 * syntaxhighlighter.h
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

#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <qtextedit.h>
#include <qsyntaxhighlighter.h>
#include <qcolor.h>
#include <qstringlist.h>

class QAccel;
class QTimer;
class KSpell;

namespace KMail {

class MessageHighlighter : public QSyntaxHighlighter
{
public:
    enum SyntaxMode {
	PlainTextMode,
	RichTextMode
    };
    MessageHighlighter( QTextEdit *textEdit, 
			bool colorQuoting = false, 
			QColor QuoteColor0 = black,
			QColor QuoteColor1 = QColor( 0x00, 0x80, 0x00 ),
			QColor QuoteColor2 = QColor( 0x00, 0x80, 0x00 ),
			QColor QuoteColor3 = QColor( 0x00, 0x80, 0x00 ),
			SyntaxMode mode = PlainTextMode );
    ~MessageHighlighter();

    int highlightParagraph( const QString& text, int endStateOfLastPara );

private:
    QColor col1, col2, col3, col4, col5;
    SyntaxMode sMode;
    bool mEnabled;
};

class SpellChecker : public MessageHighlighter
{
public:
    SpellChecker( QTextEdit *textEdit,
		  QColor spellColor = red, 
		  bool colorQuoting = false, 
		  QColor QuoteColor0 = black,
		  QColor QuoteColor1 = QColor( 0x00, 0x80, 0x00 ),
		  QColor QuoteColor2 = QColor( 0x00, 0x80, 0x00 ),
		  QColor QuoteColor3 = QColor( 0x00, 0x80, 0x00 ) );
    ~SpellChecker();

    virtual int highlightParagraph( const QString& text,
				    int endStateOfLastPara );
    virtual bool isMisspelled( const QString& word ) = 0;
    bool intraWordEditing() { return mIntraWordEditing; }
    void setIntraWordEditing(bool editing) { mIntraWordEditing = editing; }
    static QStringList personalWords();

private:
    void flushCurrentWord();

    QString currentWord;
    int currentPos;
    bool alwaysEndsWithSpace;
    QColor mColor;
    bool mIntraWordEditing;
};

class DictSpellChecker : public QObject, public SpellChecker
{
Q_OBJECT

public:
    DictSpellChecker( QTextEdit *textEdit,
		      bool spellCheckingActive = true, 
		      bool autoEnable = true, 
		      QColor spellColor = red, 
		      bool colorQuoting = false, 
		      QColor QuoteColor0 = black,
		      QColor QuoteColor1 = QColor( 0x00, 0x80, 0x00 ),
		      QColor QuoteColor2 = QColor( 0x00, 0x70, 0x00 ),
		      QColor QuoteColor3 = QColor( 0x00, 0x60, 0x00 ) );
    ~DictSpellChecker();

    virtual bool isMisspelled( const QString& word );
    static void dictionaryChanged();

signals:
    void activeChanged(const QString &);

protected:
    QString spellKey();
    bool eventFilter(QObject* o, QEvent* e);

protected slots:
    void slotMisspelling (const QString & originalword, const QStringList & suggestions, unsigned int pos);
    void slotRehighlight();
    void slotDictionaryChanged();
    void slotSpellReady( KSpell *spell );
    void slotAutoDetection();

private:
    static QDict<int> dict;
    QDict<int> mAutoDict;
    QDict<int> mAutoIgnoreDict;
    static QObject *sDictionaryMonitor;
    KSpell *mSpell;
    QTimer *rehighlightRequest;
    QString mSpellKey;
    int mWordCount, mErrorCount;
    bool mActive, mAutomatic, mAutoReady;
};

}; //namespace KMail

#endif
