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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <qtextedit.h>
#include <qtimer.h>
#include <qsyntaxhighlighter.h>
#include <qcolor.h>
#include <qstringlist.h>

class QAccel;
class KSpell;

namespace KMail {

class MessageHighlighter : public QSyntaxHighlighter
{
public:
    enum SyntaxMode {
	PlainTextMode,
	RichTextMode
    };
    MessageHighlighter( QTextEdit *textEdit, SyntaxMode mode = PlainTextMode );
    ~MessageHighlighter();

    int highlightParagraph( const QString& text, int endStateOfLastPara );

private:
    QColor col1, col2, col3, col4, col5;
    SyntaxMode sMode;
};

class SpellChecker : public MessageHighlighter
{
public:
    SpellChecker( QTextEdit *textEdit );
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
    DictSpellChecker( QTextEdit *textEdit );
    ~DictSpellChecker();

    virtual bool isMisspelled( const QString& word );
    static void dictionaryChanged();

signals:
    void activeChanged(bool);

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
    bool mRehighlightRequested;
    QString mSpellKey;
    int mWordCount, mErrorCount;
    bool mActive, mAutomatic, mAutoReady;
};

}; //namespace KMail

#endif
