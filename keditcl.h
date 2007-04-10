/* This file is part of the KDE libraries

   Copyright (C) 1996 Bernd Johannes Wuebben <wuebben@math.cornell.edu>
   Copyright (C) 2000 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KEDITCL_H
#define KEDITCL_H

#include <Q3MultiLineEdit>

#include <kdialog.h>

class QDropEvent;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QTextStream;
class KHistoryComboBox;
class KIntNumInput;
class Q3VButtonGroup;
class QMenu;

class KDEUI_EXPORT KEdGotoLine : public KDialog
{
    Q_OBJECT

public:
    explicit KEdGotoLine( QWidget *parent=0, bool modal=true );
    int getLineNumber();

public Q_SLOTS:
    void selected( int );

private:
    KIntNumInput *lineNum;

private:
    class KEdGotoLinePrivate;
    KEdGotoLinePrivate *d;
};

///
class KDEUI_EXPORT KEdFind : public KDialog
{
    Q_OBJECT
    Q_PROPERTY( QString text READ getText WRITE setText )
    Q_PROPERTY( Qt::CaseSensitivity caseSensitivity READ case_sensitive WRITE setCaseSensitive )
    Q_PROPERTY( bool direction READ get_direction WRITE setDirection )
public:

    explicit KEdFind( QWidget *parent = 0, bool modal=true);
    ~KEdFind();

    QString getText() const;
    void setText(const QString &string);
    void setCaseSensitive( Qt::CaseSensitivity b );
    Qt::CaseSensitivity case_sensitive() const;
    void setDirection( bool b );
    bool get_direction() const;

    /**
     * @returns the combobox containing the history of searches. Can be used
     * to save and restore the history.
     */
    KHistoryComboBox *searchCombo() const;

protected Q_SLOTS:
    void slotUser1( void );
    void textSearchChanged ( const QString & );

protected:
  Q3VButtonGroup* group;

private:
    QCheckBox *sensitive;
    QCheckBox *direction;

Q_SIGNALS:

    void search();
    void done();
private:
    class KEdFindPrivate;
    KEdFindPrivate *d;
};

///
class KDEUI_EXPORT KEdReplace : public KDialog
{
    Q_OBJECT

public:

    explicit KEdReplace ( QWidget *parent = 0, bool modal=true );
    ~KEdReplace();

    QString 	getText();
    QString 	getReplaceText();
    void 	setText(QString);

    /**
     * @returns the combobox containing the history of searches. Can be used
     * to save and restore the history.
     */
    KHistoryComboBox *searchCombo() const;

    /**
     * @returns the combobox containing the history of replaces. Can be used
     * to save and restore the history.
     */
    KHistoryComboBox *replaceCombo() const;

    Qt::CaseSensitivity case_sensitive();
    bool 	get_direction();

private Q_SLOTS:
    void slotUser1( void );
    void slotUser2( void );
    void slotUser3( void );
    void textSearchChanged ( const QString & );
	void slotCancel();

private:
    QCheckBox 	*sensitive;
    QCheckBox 	*direction;

Q_SIGNALS:
    void replace();
    void find();
    void replaceAll();
    void done();
private:
    class KEdReplacePrivate;
    KEdReplacePrivate *d;
};


/**
 * A simple text editor for the %KDE project.
 * @deprecated Use KTextEditor::Editor or KTextEdit instead.
 *
 * @author Bernd Johannes Wuebben <wuebben@math.cornell.edu>, Waldo Bastian <bastian@kde.org>
 **/

class KDEUI_EXPORT_DEPRECATED KEdit : public Q3MultiLineEdit
{
    Q_OBJECT

public:
    /**
     * The usual constructor.
     **/
    explicit KEdit (QWidget *_parent=0);

    ~KEdit();

    /**
     * Search directions.
     * @internal
     **/
    enum { NONE,
	   FORWARD,
	   BACKWARD };
    /**
     * Insert text from the text stream into the edit widget.
     **/
    void insertText(QTextStream *);

    /**
     * Save text from the edit widget to a text stream.
     * If @p softWrap is false soft line wrappings are replaced with line-feeds
     * If @p softWrap is true soft line wrappings are ignored.
     **/
    void saveText(QTextStream *, bool softWrap = false);

    /**
     *  Let the user select a font and set the font of the textwidget to that
     * selected font.
     **/
    void 	selectFont();

    /**
     * Present a search dialog to the user
     **/
    void 	search();

    /**
     * Repeat the last search specified on the search dialog.
     *
     *  If the user hasn't searched for anything until now, this method
     *   will simply return without doing anything.
     *
     * @return @p true if a search was done. @p false if no search was done.
     **/
    bool 	repeatSearch();

    /**
     * Present a Search and Replace Dialog to the user.
     **/
    void 	replace();

    /**
     * Present a "Goto Line" dialog to the user.
     */
    void 	doGotoLine();

    /**
     * Clean up redundant whitespace from selected text.
     */
    void        cleanWhiteSpace();

    /**
     * Install a context menu for KEdit.
     *
     *  The Popup Menu will be activated on a right mouse button press event.
     */
    void 	installRBPopup( QMenu* );

    /**
     * Retrieve the current line number.
     *
     * The current line is the line the cursor is on.
     **/
    int 	currentLine();

    /**
     * Retrieve the actual column number the cursor is on.
     *
     *  This call differs
     *    from QMultiLineEdit::getCursorPosition() in that it returns the actual cursor
     *    position and not the character position. Use currentLine() and currentColumn()
     *    if you want to display the current line or column in the status bar for
     *    example.
     */
    int 	currentColumn();


    /**
     * Start spellchecking mode.
     */
    void spellcheck_start();

    /**
     * Exit spellchecking mode.
     */
    void spellcheck_stop();

    /**
     * Allow the user to toggle between insert mode and overwrite mode with
     * the "Insert" key. See also toggle_overwrite_signal();
     *
     * The default is false: the user can not toggle.
     */
    void setOverwriteEnabled(bool b);

    QString selectWordUnderCursor();

    Q3PopupMenu *createPopupMenu( const QPoint& pos );

    void setAutoUpdate(bool b);

Q_SIGNALS:
    /** This signal is emitted whenever the cursor position changes.
     *
     * Use this in conjunction with currentLine(), currentColumn()
     * if you need to know the cursor position.
     */
    void 	CursorPositionChanged();

    /**
     * This signal is emitted if the user toggles from insert to overwrite mode
     * or vice versa.
     *
     * The user can do so by pressing the "Insert" button on a PC keyboard.
     *
     * This feature must be activated by calling setOverwriteEnabled(true)
     * first.
     */
    void 	toggle_overwrite_signal();

public Q_SLOTS:
      /**
       * @internal
       **/
    void corrected (const QString &originalword, const QString &newword, unsigned int pos);
      /**
       * @internal
       **/
    void misspelling (const QString &word, const QStringList &, unsigned int pos);
private Q_SLOTS:

      /**
       * @internal
       * Called from search dialog.
       **/
    void search_slot();

      /**
       * @internal
       **/
    void searchdone_slot();

      /**
       * @internal
       **/
    void replace_slot();

      /**
       * @internal
       **/
    void replace_all_slot();

      /**
       * @internal
       **/
    void replace_search_slot();

      /**
       * @internal
       **/
    void replacedone_slot();

      /**
       * Cursor moved...
       */
    void slotCursorPositionChanged();

protected:
    void computePosition();
    int 	doSearch(const QString &s_pattern, Qt::CaseSensitivity case_sensitive,
			 bool regex, bool forward,int line, int col);

    int 	doReplace(const QString &s_pattern, Qt::CaseSensitivity case_sensitive,
			  bool regex, bool forward,int line, int col,bool replace);

      /**
       * Sets line and col to the position pos, considering word wrap.
       **/
    void	posToRowCol(unsigned int pos, unsigned int &line, unsigned int &col);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     */
    virtual void create( WId = 0, bool initializeWindow = true,
                         bool destroyOldWindow = true );

    /**
     * Reimplemented for internal reasons, the API is not affected.
     */
    virtual void ensureCursorVisible();
    virtual void setCursor( const QCursor & );
    virtual void viewportPaintEvent( QPaintEvent* );

protected:

    void 	keyPressEvent 	 ( QKeyEvent *  );

private:
    QTimer* repaintTimer;

    QString	killbufferstring;
    QWidget     *parent;
    KEdFind 	*srchdialog;
    KEdReplace 	*replace_dialog;
    KEdGotoLine *gotodialog;

    QString     pattern;

    bool 	can_replace;
    bool	killing;
    bool 	killtrue;
    bool 	lastwasanewline;
    bool        saved_readonlystate;
    int 	last_search;
    int 	last_replace;
    int 	replace_all_line;
    int 	replace_all_col;

    int 	line_pos, col_pos;
    bool        fill_column_is_set;
    bool        word_wrap_is_set;
    int         fill_column_value;

private:
    class KEditPrivate;
    KEditPrivate *d;
};

#endif
