/*
  $Id$
 
  KEdit, a simple text editor for the KDE project

  Copyright (C) 1996 Bernd Johannes Wuebben   
                     wuebben@math.cornell.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
  */


#ifndef __KEDIT_H__
#define __KEDIT_H__

#include <qpopmenu.h>
#include <qmenubar.h>
#include <qstrlist.h>
#include <qapp.h>
#include <qkeycode.h>
#include <qaccel.h>
#include <qregexp.h> 
#include <qobject.h>
#include <qmlined.h>
#include <qlined.h>
#include <qradiobt.h>
#include <qfiledlg.h>
#include <qchkbox.h>
#include <qmsgbox.h>
#include <qcombo.h>
#include <qpushbt.h>
#include <qgrpbox.h>
#include <qregexp.h>
#include <qtstream.h>
#include <qkeycode.h>
#include <qfileinf.h> 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
  
#include <kapp.h>
#include <kfontdialog.h>

///
class KIntLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  KIntLineEdit( QWidget *parent = 0, const char *name = 0 ) 
    : QLineEdit( parent, name ) {};
  
  int getValue() { return atoi( text() ); };

protected:
	
  void keyPressEvent( QKeyEvent *e ) {
    char key = e->ascii();
    
    if( isdigit( key ) 
	|| ( e->key() == Key_Return) || ( e->key() == Key_Enter    )
	|| ( e->key() == Key_Delete) || ( e->key() == Key_Backspace)
	|| ( e->key() == Key_Left  ) || ( e->key() == Key_Right    )){

      QLineEdit::keyPressEvent( e );
      return;
    } else {
      e->ignore();
      return;
    }
  };
};

///
class KEdGotoLine : public QDialog
{
	Q_OBJECT

public:

	KEdGotoLine( QWidget *parent = 0, const char *name = 0 );

	int getLineNumber();
	KIntLineEdit *lineNum;

private:
	QPushButton *ok, *cancel;
	QGroupBox *frame;
	void resizeEvent(QResizeEvent *);
	void focusInEvent(QFocusEvent *);

public slots:

	void selected( int );
};

///
class KEdSrch : public QDialog
{
    Q_OBJECT

public:

    KEdSrch ( QWidget *parent = 0, const char *name=0);

    QString getText();
    void setText(QString string);
    bool case_sensitive();
    bool get_direction();

protected:
    void focusInEvent( QFocusEvent *);
    void resizeEvent(QResizeEvent *);

private:

    QPushButton *ok, *cancel;
    QCheckBox *sensitive;
    QCheckBox *direction;
    QGroupBox *frame1;
    QLineEdit *value;

signals:

    void search_signal();
    void search_done_signal();

public slots:
     
    void done_slot();
    void ok_slot();

};

///
class KEdReplace : public QDialog
{
    Q_OBJECT

public:

    KEdReplace ( QWidget *parent = 0, const char *name=0);

    QString 	getText();
    QString 	getReplaceText();
    void 	setText(QString);
    QLineEdit 	*value;
    QLineEdit 	*replace_value;
    QLabel 	*label;
    bool 	case_sensitive();
    bool 	get_direction();

protected:

    void focusInEvent( QFocusEvent *);
    void resizeEvent ( QResizeEvent *);

private:

    QPushButton *ok, *cancel, *replace, *replace_all;
    QCheckBox 	*sensitive;
    QCheckBox 	*direction;
    QGroupBox 	*frame1;


signals:

    void replace_signal();
    void find_signal();
    void replace_all_signal();
    void replace_done_signal();

public slots:
     
    void done_slot();
    void replace_slot();
    void replace_all_slot();
    void ok_slot();

};


///
class KEdit : public QMultiLineEdit
{
    Q_OBJECT
    
public:

    KEdit (KApplication *a=NULL,QWidget *parent=NULL, const char *name=NULL, 
	   const char *filename=NULL);

    ~KEdit();

    
    enum { NONE, 
	   FORWARD, 
	   BACKWARD };

    enum { KEDIT_OK 		= 0, 
	   KEDIT_OS_ERROR 	= 1, 
	   KEDIT_USER_CANCEL 	= 2 ,
	   KEDIT_RETRY 		= 3,
	   KEDIT_NOPERMISSIONS 	= 4};

    enum { OPEN_READWRITE 	= 1, 
	   OPEN_READONLY 	= 2, 
	   OPEN_INSERT 		= 4 };
    

    /// Opens a new untitled document in the text widget
      /** Opens a new untitled document in the text widget The user is given
          a chance to save the current document if the current document has
	  been modified.
       */
    int  	newFile();

    /// Saves the file if necessary under the current file name
      /** Saves the file if necessary under the current file name. If the current file
	name is Untitled, as it is after a call to newFile(), this routing will
	call saveAs().
       */
    int 	doSave();

    /// Saves the file as filename
      /** Saves the file as filename
       */
    int 	doSave( const char *filename );

    /// Allows the user to save the file under a new name
	  /** Allows the user to save the file under a new name
	   */
    int 	saveAs();

    /// Let the user open an new file
    /** This will present an open file dialog and open  the file specified by the user,
        if possible. The return codes are KEDIT_OK, KEDIT_USER_CANCEL and
	KEDIT_OS_ERROR. The user will be given a chance to save the current file if
	it has been modified. mode is one of OPEN_READONLY, OPEN_READWRITE.
	OPEN_READONLY means  that the user will not be able to insert characters
	into the document. 
     */
    int 	openFile( int mode );

    /// Lets the user insert a file at the current cursor position
    /** Calling this method will let the user insert a file at the current cursor 
        position. Return codes are KEDIT_OK, KEDIT_USER_CANCEL, KDEDIT_OS_ERROR.
    */
    int 	insertFile();

    /// Loads the file filemane in the editor
	/** Loads the file filename into editor. The possible modes are
            OPEN_READONLY, OPEN_READWRITE, OPEN_INSERT.
	    OPEN_READONLY means  that the user will not be able to insert characters
	    into the document. OPEN_INSERT means that the file will be inserted
	    into the current document at the current cursor position.
	   */
    int 	loadFile( QString filename , int mode );
      

    /// Returns the filename of the current file.
    /** Returns the filename of the currnet file. You can use setName() to set the 
        filename of the current file
    */
    QString 	getName();

    /// Sets the filename of the current file.
    /** Sets the filename of the currnet file. You can use getName() to set the 
        filename of the current file
    */
    void 	setName( const char *_name );

    /// Returns the currently marked text.
    /** Returns the currently marked text.
     */
    QString 	markedText();

    /// Lets the user select a font and sets the font of the textwidget.
    /** Lets the user select a font and sets the font of the textwidget to that 
        selected font.
    */
    void 	selectFont();

    /// Presents a search dialog to the user
    /** Presents a search dialog to the user
     */
    void 	Search();
    
    /// Repeats the last search specified on the search dialog.
    /** Repeasts the last search specified on the search dialog. If the user
        hasn't searched for anything until now, this method will simply return
	without doing anything.
    */
    int 	repeatSearch();

    /// Presents a Search and Replace Dialog to the User.
    /**  Presents a Search and Replace Dialog to the User.
     */
    void 	Replace();

    /// Presents a "Goto Line" dialog to the User
    /** Presents a "Goto Line" dialog to the User
     */
    void 	doGotoLine();

    /// Returns true if the document has been modified.
    /**Returns true if the document has been modified.
    */
    bool 	isModified();
    
    /// Toggles the modification status of the document
    /** Toggles the modification status of the document. TRUE = Modified, 
        FALSE = UNMODIFIED. Methods such as doSave() rely on this to see whether
	the document needs to be saved.
    */
    void 	toggleModified( bool );

    ///  Sets Indent Mode
    /**  Sets the Indent Mode. TRUE = Indent  mode on, FALSE = Indent mode off. 
     */
    void 	setAutoIndentMode( bool );

    /// Returns the Indent Mode  
    /** Returns the Indent Mode. TRUE = Indent  mode on, FALSE = Indent mode off. 
     */
    bool        AutoIndentMode(){ return autoIndentMode; };

    /// Install a Popup Menue for KEdit. 
    /** Install a Popup Menue for KEdit. The Popup Menu will be activated on
        a right mouse button press event.
     */
    void 	installRBPopup( QPopupMenu* );

    /// Return the current Line number
	  /** Returns the current line number, that is the line the cursor is on.
	   */
    int 	currentLine();

    /// Returns the current Column number
	  /** This returns the actual column number the cursor is on. This call differs
	    from QMultiLineEdit::getCursorPosition in that it returns the actual cursor
	    position and not the character position. Use currentLine() and currentColumn()
	    if you want to display the current line or column in the status bar for 
	    example.
	    */
    int 	currentColumn();

    /// Returns TRUE if word wrap is on
	   /** Returns TRUE if word wrap is on. You also need to specify the fill column
	       with setFillColumnMode() otherwise wordwrap is not in effect.
	       */
    bool 	WordWrap();

    /// Turn word wrap mode on or off. 
	   /** You also need to specify the fill column
	       with setFillColumnMode() otherwise wordwrap is not in effect.
	       */
    void 	setWordWrap(bool );
    
    /// Returns TRUE if fill column mode is on
	   /**  Returns TRUE if fill column mode is on, that is if the line will
	        be broken automatically when if a character is to be inserted past
		this position.
		*/
    bool 	FillColumnMode();

    /// Set the fill column to column col if line is strictly larger than 0.
	   /**  Set the fill column to column col, if col is strictly larger than 0.
	        If col  is 0, fill column mode is turned off.
	        In fill column mode, the line will
	        be broken automatically at column col, when a character is 
		inserted past column col..
		*/
    void  	setFillColumnMode(int line, bool set);

    /// save a backup copy
	  /** If copy is TRUE KEdit will make a backup copy of the document that
	      is being edited on opening it. The backup copy will receive the
	      suffix ~. The default is TRUE.
	    */
    void       saveBackupCopy(bool copy);

    /// set the name of the file 
	  /** Sets the name of the file if a file is open.
	    */
    void       setFileName(char* name);

    /// save the current file as 'name'
	  /** saves the current file as 'name'
	    */


    void       saveasfile(char* name);

    /// remove tabs and whitespace on the end of lines during a justify operation
	  /** remove tabs and whitespace on the end of lines during a justify operation
	    */
    void       setReduceWhiteOnJustify(bool reduce);

    bool 	format(QStrList& );
    bool 	format2(QStrList& par, int& upperbound);
    void 	getpar(int line,QStrList& par);
    void 	getpar2(int line,QStrList& par,int& upperbound,QString &prefix);

signals:

    /// This signal is emitted when the document in the textwidget has changed
	  /** This signal is emitted when the document in the textwidget has changed
	   */
    void 	fileChanged();

    ///  This signal is emitted whenever the cursor position changed.
	   /** This signal is emitted whenever the cursor position changed.
	       Use this in conjunction with currentLine(), currentColumn() 
	       if you need to know the cursor position.
	       */
    void 	CursorPositionChanged();
    
    /// This signal is emitted just before saving a file.
	  /** This signal is emitted just before saving a file. Since KEdit calls
	      kapp->processEvents(), you have a chance to let the user know what's 	
	      going to happen.
	   */
    void 	saving();

    /// This signal is emitted just before loading a file.
	  /** This signal is emitted just before loading a file. Since KEdit calls
	      kapp->processEvents(), you have a chance to let the user know what's 	
	      going to happen.
	   */
    void 	loading();

    /// This signal is emitted if the user toggles from overwrite to insert mode
	 /** This signal is emitted if the user toggles from overwrite to insert mode.
	     He can do so by pressing the "Insert" Button on a PC keyboard.
	     */
    void 	toggle_overwrite_signal();




public slots:
    
    void setModified();

    void search_slot();

    void searchdone_slot();

    void replace_slot();

    void replace_all_slot();

    void replace_search_slot();

    void replacedone_slot();

    void computePosition();


protected:

    int 	saveFile();

    int 	doSearch(QString s_pattern, bool case_sensitive, 
			 bool regex, bool forward,int line, int col);

    int 	doReplace(QString s_pattern, bool case_sensitive, 
			  bool regex, bool forward,int line, int col,bool replace);

    QFileDialog* getFileDialog(const char* captiontext);


protected:

    bool 	eventFilter	 ( QObject *, QEvent * );
    void 	keyPressEvent 	 ( QKeyEvent *  );
    void 	mousePressEvent  ( QMouseEvent* );
    void 	mouseReleaseEvent( QMouseEvent* );
    void 	mouseMoveEvent 	 ( QMouseEvent* );


private:

    void 	setContextSens();
    void        mynewLine();
    QString 	prefixString(QString);


private:

    QString	killbufferstring;
    QWidget     *p_parent;
    QFileDialog *fbox;
    KEdSrch 	*srchdialog;
    KEdReplace 	*replace_dialog;
    KEdGotoLine *gotodialog;
    QPopupMenu  *rb_popup;
    KApplication* mykapp;
    QFileDialog *file_dialog;
    QStrList	par;

    QString 	filename;
    QString     pattern;
    QString     current_directory;

    bool 	modified;
    bool 	autoIndentMode;
    bool 	can_replace;
    bool	killing;
    bool 	killtrue;
    bool 	lastwasanewline;
    bool        reduce_white_on_justify;
    int		cursor_offset;
    int 	edit_mode;
    int 	last_search;
    int 	last_replace;
    int 	replace_all_line;
    int 	replace_all_col;

    int 	line_pos, col_pos;
    bool        fill_column_is_set;
    bool        word_wrap_is_set;
    int         fill_column_value;
    bool        make_backup_copies;
    
};



#endif
