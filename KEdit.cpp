 /*

  $Id$
 
  kedit, a simple text editor for the KDE project

  Requires the Qt widget libraries, available at no cost at 
  http://www.troll.no
  
  Copyright (C) 1997 Bernd Johannes Wuebben   
  wuebben@math.cornell.edu

  parts:
  Alexander Sanda <alex@darkstar.ping.at>  
 
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
  
  KEdit, simple editor class, hacked version of the original by 

 
  */

#include "KEdit.h"
#include "KEdit.moc"


KEdit::KEdit(KApplication *a, QWidget *parent, const char *name, 
	     const char *fname) : QMultiLineEdit(parent, name){


    mykapp = a;
    filename = fname;
    filename.detach();

    modified = FALSE;

    // set some defaults

    line_pos = col_pos = 0;
    fill_column_is_set = TRUE;
    word_wrap_is_set = TRUE;
    fill_column_value = 79;
    autoIndentMode = false;
    
    make_backup_copies = TRUE;
    
    installEventFilter( this );     

    srchdialog = NULL;
    replace_dialog= NULL;

    connect(this, SIGNAL(textChanged()), this, SLOT(setModified()));
    setContextSens();
}



KEdit::~KEdit(){

}


int KEdit::currentLine(){

  computePosition();
  return line_pos;

};

int KEdit::currentColumn(){

  computePosition();
  return col_pos;

}


bool KEdit::WordWrap(){

  return word_wrap_is_set;

}

void  KEdit::setWordWrap(bool flag ){

  word_wrap_is_set = flag;
}
    
bool  KEdit::FillColumnMode(){

  return fill_column_is_set;
}

void  KEdit::setFillColumnMode(int line){

  if (line <= 0) {
    fill_column_is_set = FALSE;
    fill_column_value = 0;
  }
  else{
    fill_column_is_set = TRUE;
    fill_column_value = line;
  }

}


int KEdit::loadFile(QString name, int mode){

    int fdesc;
    struct stat s;
    char *mb_caption = "Load file:";
    char *addr;
    QFileInfo info(name);

    if(!info.exists()){
      QMessageBox::message("Sorry","The specified File does not exist","OK");
// TODO: finer error control
      return KEDIT_RETRY;
    }

    if(!info.isReadable()){
      QMessageBox::message("Sorry","You do not have read permission to this file.","OK");
// TODO: finer error control
      return KEDIT_RETRY;
    }


    fdesc = open(name, O_RDONLY);

    if(fdesc == -1) {
        switch(errno) {
        case EACCES:
            QMessageBox::message("Sorry", 
				 "You have do not have Permission to \n"\
				 "read this Document", "Ok");
// TODO: finer error control
            return KEDIT_OS_ERROR;

        default:
            QMessageBox::message(mb_caption, 
				 "An Error occured while trying to open this Document", 
				 "Ok");
// TODO: finer error control
            return KEDIT_OS_ERROR;
        }
    }
    
    emit loading();
    mykapp->processEvents();

    fstat(fdesc, &s);
    addr = (char *)mmap(0, s.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fdesc, 0);

    setAutoUpdate(FALSE);

    disconnect(this, SIGNAL(textChanged()), this, SLOT(setModified()));

// The following is a horrible hack that we need to put up with until
// Qt 1.3 comes along. The problem is that QMultiLineEdit::setText(char*)
// is O(n^2) which makes loading larger files very slow. We resort here to 
// a workaroud which load the file line by line. I use a memmap construction
// but probably a QTextStream is just about as fast.

    if(mode & OPEN_INSERT) {
      
      register long int  i;
      char *beginning_of_line;
      beginning_of_line = addr;
      int line, col;
      
      char c = '\n';

      if(s.st_size != 0){
	c = *(addr + s.st_size - 1 ); // save the last character of the file
      }
      
      // this boolean will indicate whether we already have inserted the first line.
      bool past_first_line = false;

      getCursorPosition(&line,&col);

      // let's save the position of the cursor
      int save_line = line;
      int save_col = col;

      for(i = 0; i < s.st_size; i++){

	if( *(addr + i) == '\n' ){

	  *(addr + i) = '\0';

	  if(!past_first_line){
	    QString string;
	    string = beginning_of_line;
	    string += '\n';
	    insertAt(string,line,col);
	    past_first_line = true;
	  }
	  else{
	    insertLine(beginning_of_line,line);
	  }

	  line ++;
	  beginning_of_line = addr + i + 1;

	}
      }

      // What if the last char of the file wasn't a newline?
      // In this case we have to manually insert the last "couple" of characters.
      // This routine could be avoided if I knew for sure that I could
      // memmap a file into a block of memory larger than the file size.
      // In that case I would simply put a zero after the last char in the file.
      // and the above would go through almost unmodified. Well, I don't, so 
      // here we go:

      if( c != '\n'){ // we're in here if s.st_size != 0 and last char != '\n'

	char* buf = (char*)malloc(addr + i - 1 - beginning_of_line + 2);
	strncpy(buf, beginning_of_line, addr + i - 1 - beginning_of_line +1);
	buf[ addr + i - 1 - beginning_of_line + 2 ] = '\0';
	append(buf);

	free(buf);
      
      }
      // restore the initial Curosor Position
      setCursorPosition(save_line,save_col);

    }
    else{ // Not inserting but loading a completely new file.

      register long int  i;
      char *beginning_of_line;
      beginning_of_line = addr;

      // eradicate the old text.
      this->clear();
      char c = '\n';

      if(s.st_size != 0){
	c = *(addr + s.st_size - 1 ); // save the last character of the file
      }

      for(i = 0; i < s.st_size; i++){
	if( *(addr + i) == '\n' ){
	  *(addr + i) = '\0';
	  append(beginning_of_line);
	  beginning_of_line = addr + i + 1;
	}
      }

      // Same consideration as above:

      if( c != '\n'){ // we're in here if s.st_size != 0 and last char != '\n'

	char* buf = (char*)malloc(addr + i - 1 - beginning_of_line + 2);
	strncpy(buf, beginning_of_line, addr + i - 1 - beginning_of_line +1);
	buf[ addr + i - 1 - beginning_of_line + 2 ] = '\0';
	append(buf);

	free(buf);
      
      }
    }

    setAutoUpdate(TRUE);
    repaint();

    connect(this, SIGNAL(textChanged()), this, SLOT(setModified()));    

    munmap(addr, s.st_size);
        
    if ( mode == OPEN_INSERT)
      toggleModified(TRUE);
    else
      toggleModified(FALSE);
    
    
    if(!(mode == OPEN_INSERT)){
        filename = name;
	filename.detach();
    }

    if( mode == OPEN_READONLY)
      this->setReadOnly(TRUE);
    else
      this->setReadOnly(FALSE);
    

    emit(fileChanged());
    setFocus();

    return KEDIT_OK;
}


int KEdit::insertFile(){

    QFileDialog *box;
    QString file_to_insert;
      
    box = new QFileDialog( this, "fbox", TRUE);
    
    box->setCaption("Select Document to Insert");

    box->show();
    
    if (!box->result()) {
      delete box;
      return KEDIT_USER_CANCEL;
    }
    
    if(box->selectedFile().isEmpty()) {  /* no selection */
      delete box;
      return KEDIT_USER_CANCEL;
    }
    
    file_to_insert = box->selectedFile();
    file_to_insert.detach();
    
    delete box;

    
    int result = loadFile(file_to_insert, OPEN_INSERT);

    if (result == KEDIT_OK )
      setModified();

    return  result;


}

int KEdit::openFile(int mode)
{
    QString fname;
    QFileDialog *box;
    

    if( isModified() ) {           
      if((QMessageBox::query("Message", 
			     "The current Document has been modified.\n"\
			     "Would you like to save it?"))) {

	if (doSave() != KEDIT_OK){
	  
	  QMessageBox::message("Sorry", "Could not Save the Document", "OK");
	  return KEDIT_OS_ERROR;     

	}
      }
    }
            
    box = new QFileDialog( this, "fbox", TRUE);
    
    box->setCaption("Select Document to Open");

    box->show();
    
    if (!box->result())   /* cancelled */
      return KEDIT_USER_CANCEL;
    if(box->selectedFile().isEmpty()) {  /* no selection */
      return KEDIT_USER_CANCEL;
    }
    
    fname =  box->selectedFile();
    delete box;
    
    int result =  loadFile(fname, mode);
    
    if ( result == KEDIT_OK )
      toggleModified(FALSE);
    
    return result;
	
        
}

int KEdit::newFile(){


    if( isModified() ) {           
      if((QMessageBox::query("Message", 
			     "The current Document has been modified.\n"\
			     "Would you like to save it?"))) {

	if (doSave() != KEDIT_OK){
	  
	  QMessageBox::message("Sorry", "Could not Save the Document", "OK");
	  return KEDIT_OS_ERROR;     

	}
      }
    }

    this->clear();
    toggleModified(FALSE);

    setFocus();

    filename = "Untitled";
       
    computePosition();
    emit(fileChanged());

    return KEDIT_OK;
            
        
}



void KEdit::computePosition(){

  int line, col, coltemp;

  getCursorPosition(&line,&col);
  QString linetext = textLine(line);

  // O.K here is the deal: The function getCursorPositoin returns the character
  // position of the cursor, not the screenposition. I.e,. assume the line
  // consists of ab\tc then the character c will be on the screen on position 8
  // whereas getCursorPosition will return 3 if the cursors is on the character c.
  // Therefore we need to compute the screen position from the character position.
  // That's what all the following trouble is all about:
  
  coltemp  = 	col;
  int pos  = 	0;
  int find = 	0;
  int mem  = 	0;
  bool found_one = false;

  // if you understand the following algorithm you are worthy to look at the
  // kedit+ sources -- if not, go away ;-)

  while(find >=0 && find <= coltemp- 1 ){
    find = linetext.find('\t', find, TRUE );
    if( find >=0 && find <= coltemp - 1 ){
      found_one = true;
      pos = pos + find - mem;
      pos = pos + 8  - pos % 8;
      mem = find;
      find ++;
    }
  }

  pos = pos + coltemp - mem ;  // add the number of characters behind the
                               // last tab on the line.

  if (found_one){	       
    pos = pos - 1;
  }

  line_pos = line;
  col_pos = pos;

}


void KEdit::keyPressEvent ( QKeyEvent *e){

  QString* pstring;

  if (e->key() == Key_Tab){
    if (isReadOnly())
      return;
    QMultiLineEdit::insertChar((char)'\t');
    setModified();
    emit CursorPositionChanged();
    return;
  }

  if (e->key() == Key_Insert){
    this->setOverwriteMode(!this->isOverwriteMode());
    emit toggle_overwrite_signal();
    return;
  }
  
  //printf("fill %d word %d value %d\n", fill_column_is_set,
  //   word_wrap_is_set,fill_column_value);

  if(fill_column_is_set && word_wrap_is_set ){

    // word break algorithm
    if(isprint(e->ascii())){
 
      // printf("col_pos %d\n",col_pos);
      if( col_pos > fill_column_value - 1){ 

	if (e->ascii() == 32 ){ // a space we can just break here
	  mynewLine();
	  //  setModified();
	  emit CursorPositionChanged();
	  return;	 
	}

	pstring = getString(line_pos);

	// find a space to break at
	int space_pos = pstring->findRev(" ", -1,TRUE);

	if( space_pos == -1 ){ 
	  
	  // no space to be found on line, just break, what else could we do?
	  mynewLine();
	  computePosition();  

	}
	else{

	  // Can't use insertAt('\n'...) due to a bug in Qt 1.2, and must resort
	  // to the following work around:

	  // go back to the space

	  focusOutEvent(&QFocusEvent(Event_FocusOut));

	  for(uint i = 0; i < (pstring->length() - space_pos -1 ); i++){
	    cursorLeft();
	  }
	  
	  // insert a newline there

	  mynewLine();
	  end(FALSE);

	  focusOutEvent(&QFocusEvent(Event_FocusIn));

	  computePosition();
	}
      }

      QMultiLineEdit::keyPressEvent(e);
      // setModified();
      emit CursorPositionChanged();
      return;
    }
    else{ // not isprint that is some control character or some such
  
      if(e->key() == Key_Return || e->key() == Key_Enter){
    
	mynewLine();
	//setModified();
	emit CursorPositionChanged();
	return;
      }

      QMultiLineEdit::keyPressEvent(e);
      // setModified();
      emit CursorPositionChanged();
      return;

    }
  } // end do_wordbreak && fillcolumn_set
 

  // fillcolumn but no wordbreak

  if (fill_column_is_set){

    if(e->key() == Key_Return || e->key() == Key_Enter){
    
      mynewLine();
      // setModified();
      emit CursorPositionChanged();
      return;

    }

    if(isprint(e->ascii())){
    
      if( col_pos > fill_column_value - 1){ 
	  mynewLine();
	  //  setModified();
      }

    }

    QMultiLineEdit::keyPressEvent(e);
    //setModified();
    emit CursorPositionChanged();
    return;

  }

  // default action
  if(e->key() == Key_Return || e->key() == Key_Enter){
    
    mynewLine();
    //setModified();
    emit CursorPositionChanged();
    return;

  }

  QMultiLineEdit::keyPressEvent(e);
  //setModified();
  emit CursorPositionChanged();

}


void KEdit::mynewLine(){

  if (isReadOnly())
    return;

  setModified();

  if(!autoIndentMode){ // if not indent mode
    newLine();
    return;
  }

  int line,col;
  bool found_one = false;

  getCursorPosition(&line,&col);
  
  QString string, string2;

  while(line >= 0){

    string  = textLine(line);
    string2 = string.stripWhiteSpace();

    if(!string2.isEmpty()){
      string = prefixString(string);
      found_one = TRUE;
      break;
    }

    line --;
  }
      
  // string will now contain those whitespace characters that I need to insert
  // on the next line. 

  if(found_one){

    // don't ask my why I programmed it this way. I am quite sick of the Qt 1.2
    // MultiLineWidget -- It is anoyingly buggy. 
    // I have to put in obscure workarounds all over the place. 

    focusOutEvent(&QFocusEvent(Event_FocusOut));
    newLine();
    
    for(uint i = 0; i < string.length();i++){
      insertChar(string.data()[i]);
    }

    // this f***king doesn't work.
    // insertAt(string.data(),line + 1,0);

    focusInEvent(&QFocusEvent(Event_FocusIn));

  }
  else{
    newLine();
  }
}

void KEdit::setAutoIndentMode(bool mode){

  autoIndentMode = mode;

}


QString KEdit::prefixString(QString string){
  
  // This routine return the whitespace before the first none white space
  // character in string. This is  used in mynewLine() for indent mode.
  // It is assumed that string contains at least one non whitespace character
  // ie \n \r \t \v \f and space
  
  //  printf(":%s\n",string.data());

  int size = string.size();
  char* buffer = (char*) malloc(size + 1);
  strncpy (buffer, string.data(),size - 1);
  buffer[size] = '\0';

  int i;
  for (i = 0 ; i < size; i++){
    if(!isspace(buffer[i]))
      break;
  }

  buffer[i] = '\0';

  QString returnstring = buffer;
  
  free(buffer);

  //  printf(":%s:\n",returnstring.data());
  return returnstring;

}

void KEdit::mousePressEvent (QMouseEvent* e){

  
  QMultiLineEdit::mousePressEvent(e);
  emit CursorPositionChanged();

}

void KEdit::mouseMoveEvent (QMouseEvent* e){

  QMultiLineEdit::mouseMoveEvent(e);
  emit CursorPositionChanged();
  

}


void KEdit::installRBPopup(QPopupMenu* p){

  rb_popup = p;

}

void KEdit::mouseReleaseEvent (QMouseEvent* e){

  
  QMultiLineEdit::mouseReleaseEvent(e);
  emit CursorPositionChanged();

}


int KEdit::saveFile(){


    if(!modified) {
      emit saving();
      mykapp->processEvents();
      return KEDIT_OK;
    }

    QFile file(filename);
    QString backup_filename;

    if(file.exists()){

      backup_filename = filename;
      backup_filename.detach();
      backup_filename += '~';

      rename(filename.data(),backup_filename.data());
    }

    if( !file.open( IO_WriteOnly | IO_Truncate )) {
      rename(backup_filename.data(),filename.data());
      QMessageBox::message("Sorry","Could not save the document\n","OK");
      return KEDIT_OS_ERROR;
    }

    emit saving();
    mykapp->processEvents();
      
    QTextStream t(&file);
    
    int line_count = numLines();

    for(int i = 0 ; i < line_count ; i++){
	t << textLine(i) << '\n';
    }

    modified = FALSE;    
    file.close();
    
    return KEDIT_OK;

}

int KEdit::saveAs()
{

    QFileDialog *box;
    QFileInfo info;
    QString tmpfilename;
    int result;
    
    box = new QFileDialog( this, "box", TRUE);
    box->setCaption("Save Document As");

try_again:

    box->show();

    if (!box->result())
      {
	delete box;
	return KEDIT_USER_CANCEL;
      }

    if(box->selectedFile().isEmpty()){
      delete box;
      return KEDIT_USER_CANCEL;
    }

    info.setFile(box->selectedFile());

    if(info.exists()){
        if(!(QMessageBox::query("Warning:", 
				"A Document with this Name exists already\n"\
				"Do you want to overwrite it ?")))
	  goto try_again;  

    }


    tmpfilename = filename;

    filename = box->selectedFile();

    // we need this for saveFile();
    modified = TRUE; 
    
    delete box;

    result =  saveFile();
    
    if( result != KEDIT_OK)
      filename = tmpfilename; // revert filename
	
    return result;
      
}



int KEdit::doSave()
{

    
  int result = 0;
    
    if(filename == "Untitled") {
      result = saveAs();

      if(result == KEDIT_OK)
	setCaption(filename);

      return result;
    }

    QFileInfo info(filename);
    if(!info.isWritable()){
      QMessageBox::message("Sorry:", 
			   "You do not have write permission to this file.\n","OK");
      return KEDIT_NOPERMISSIONS;
    }
    

    result =  saveFile();
    return result;

}

int KEdit::doSave( const char *_name ){

    QString temp  = filename;
    filename =  _name;
    filename.detach();

    int result = saveFile();

    filename = temp;
    filename.detach();
    return result;
}


void KEdit::setName( const char *_name ){

    filename = _name;
    filename.detach();
}


QString KEdit::getName(){

    return filename;
}


void KEdit::saveBackupCopy(bool par){

  make_backup_copies = par;

}
void KEdit::selectFont(){
 
  QFont font = this->font();
  KFontDialog::getFont(font);
  this->setFont(font);

}

void KEdit::setModified(){

  modified = TRUE;


}

void KEdit::toggleModified( bool _mod ){

    modified = _mod;
}



bool KEdit::isModified(){

    return modified;
}



void KEdit::setContextSens(){

}


bool KEdit::eventFilter(QObject *o, QEvent *ev){

  static QPoint tmp_point;

  (void) o;

  if(ev->type() != Event_MouseButtonPress) 
    return FALSE;
    
  QMouseEvent *e = (QMouseEvent *)ev;
  
  if(e->button() != RightButton) 
    return FALSE;

  tmp_point = QCursor::pos();
  
  if(rb_popup)
    rb_popup->popup(tmp_point);

  return TRUE;

}


QString KEdit::markedText(){

  return QMultiLineEdit::markedText();

}

void KEdit::doGotoLine() {

	if( !gotodialog )
		gotodialog = new KEdGotoLine( this, "gotodialog" );

	this->clearFocus();

	gotodialog->show();
	// this seems to be not necessary
	// gotodialog->setFocus();
	if( gotodialog->result() ) {
		setCursorPosition( gotodialog->getLineNumber() , 0, FALSE );
		emit CursorPositionChanged();
		setFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Find Methods
//
 
void KEdit::Search(){
  

  if(!srchdialog){
    srchdialog = new KEdSrch(this, "searchdialog");
    connect(srchdialog,SIGNAL(search_signal()),this,SLOT(search_slot()));
    connect(srchdialog,SIGNAL(search_done_signal()),this,SLOT(searchdone_slot()));
  }

  // If we already searched / replaced something before make sure it shows
  // up in the find dialog line-edit.

  QString string;
  string = srchdialog->getText();
  if(string.isEmpty())
    srchdialog->setText(pattern);

  this->deselect();
  last_search = NONE;

  this->clearFocus();
  srchdialog->show();
  srchdialog->result();
}


void KEdit::search_slot(){

  int line, col;

  if (!srchdialog)
    return;

  QString to_find_string = srchdialog->getText();
  getCursorPosition(&line,&col);
  
  // srchdialog->get_direction() is true if searching backward

  if (last_search != NONE && srchdialog->get_direction()){
    col = col  - pattern.length() - 1 ;
  }

again:
  int  result = doSearch(to_find_string, srchdialog->case_sensitive(),
			 FALSE, (!srchdialog->get_direction()),line,col);
    
  if(result == 0){
    if(!srchdialog->get_direction()){ // forward search
    
      int query = QMessageBox::query("Find", "End of document reached.\n"\
				   "Continue from the beginning?", "Yes","No");
      if (query){
	line = 0;
	col = 0;
	goto again;
      }
    }
    else{ //backward search
      
      int query = QMessageBox::query("Find", "Beginning of document reached.\n"\
				   "Continue from the end?", "Yes","No");
      if (query){
	QString string = textLine( numLines() - 1 );
	line = numLines() - 1;
	col  = string.length();
	last_search = BACKWARD;
	goto again;
      }
    }
  }
  else{
    emit CursorPositionChanged(); 
  }
}



void KEdit::searchdone_slot(){

  if (!srchdialog)
    return;

  srchdialog->hide();
  this->setFocus();
  last_search = NONE;

}

int KEdit::doSearch(QString s_pattern, bool case_sensitive, 
		    bool wildcard, bool forward, int line, int col){

  (void) wildcard; // reserved for possible extension to regex


  int i, length;  
  int pos = -1;

  if(forward){

    QString string;

    for(i = line; i < numLines(); i++) {

      string = textLine(i);

      pos = string.find(s_pattern, i == line ? col : 0, case_sensitive);
      
      if( pos != -1){
      
	length = s_pattern.length();

	setCursorPosition(i,pos,FALSE);
      
	for(int l = 0 ; l < length; l++){
	  cursorRight(TRUE);
	}
      
	setCursorPosition( i , pos + length, TRUE );
	pattern = s_pattern;
	last_search = FORWARD;

	return 1;
      }
    }
  }
  else{ // searching backwards

    QString string;
    
    for(i = line; i >= 0; i--) {

      string = textLine(i);
      int line_length = string.length();

      pos = string.findRev(s_pattern, line == i ? col : line_length , case_sensitive);

      if (pos != -1){

	length = s_pattern.length();

	if( ! (line == i && pos > col ) ){

	  setCursorPosition(i ,pos ,FALSE );
      
	  for(int l = 0 ; l < length; l++){
	    cursorRight(TRUE);
	  }	
      
	  setCursorPosition(i ,pos + length ,TRUE );
	  pattern = s_pattern;
	  last_search = BACKWARD;
	  return 1;

	}
      }

    }
  }
  
  return 0;

}



int KEdit::repeatSearch() {
  
  if(!srchdialog)
      return 0;


  if(pattern.isEmpty()) // there wasn't a previous search
    return 0;

  search_slot();

  this->setFocus();
  return 1;

}


//////////////////////////////////////////////////////////////////////////
//
// Replace Methods
//


void KEdit::Replace(){
  

  if(!replace_dialog){
    
    replace_dialog = new KEdReplace(this, "replace_dialog");
    connect(replace_dialog,SIGNAL(find_signal()),this,SLOT(replace_search_slot()));
    connect(replace_dialog,SIGNAL(replace_signal()),this,SLOT(replace_slot()));
    connect(replace_dialog,SIGNAL(replace_all_signal()),this,SLOT(replace_all_slot()));
    connect(replace_dialog,SIGNAL(replace_done_signal()),this,SLOT(replacedone_slot()));
  
  }

  QString string = replace_dialog->getText();

  if(string.isEmpty())
    replace_dialog->setText(pattern);


  this->deselect();
  last_replace = NONE;

  this->clearFocus();
  //  replace_dialog->setFocus();
  //  replace_dialog->exec();
  replace_dialog->show();
  replace_dialog->result();
}


void KEdit::replace_slot(){

  if (!replace_dialog)
    return;

  if(!can_replace){
    QApplication::beep();
    return;
  }

  int line,col, length;

  QString string = replace_dialog->getReplaceText();
  length = string.length();

  this->cut();

  getCursorPosition(&line,&col);

  insertAt(string,line,col);
  setModified();
  can_replace = FALSE;

  setCursorPosition(line,col);
  for( int k = 0; k < length; k++){
    cursorRight(TRUE);
  }

}

void KEdit::replace_all_slot(){

  if (!replace_dialog)
    return;

  QString to_find_string = replace_dialog->getText();
  getCursorPosition(&replace_all_line,&replace_all_col);
  
  // replace_dialog->get_direction() is true if searching backward

  if (last_replace != NONE && replace_dialog->get_direction()){
    replace_all_col = replace_all_col  - pattern.length() - 1 ;
  }
  
  deselect();

again:

  setAutoUpdate(FALSE);
  int result = 1;

  while(result){

    result = doReplace(to_find_string, replace_dialog->case_sensitive(),
		       FALSE, (!replace_dialog->get_direction()),
		       replace_all_line,replace_all_col,TRUE);

  }

  setAutoUpdate(TRUE);
  update();
    
  if(!replace_dialog->get_direction()){ // forward search
    
    int query = QMessageBox::query("Find", "End of document reached.\n"\
				   "Continue from the beginning?", "Yes","No");
    if (query){
      replace_all_line = 0;
      replace_all_col = 0;
      goto again;
    }
  }
  else{ //backward search
    
    int query = QMessageBox::query("Find", "Beginning of document reached.\n"\
				   "Continue from the end?", "Yes","No");
    if (query){
      QString string = textLine( numLines() - 1 );
      replace_all_line = numLines() - 1;
      replace_all_col  = string.length();
      last_replace = BACKWARD;
      goto again;
    }
  }

  emit CursorPositionChanged(); 

}


void KEdit::replace_search_slot(){

  int line, col;

  if (!replace_dialog)
    return;

  QString to_find_string = replace_dialog->getText();
  getCursorPosition(&line,&col);
  
  // replace_dialog->get_direction() is true if searching backward

  //printf("col %d length %d\n",col, pattern.length());

  if (last_replace != NONE && replace_dialog->get_direction()){
    col = col  - pattern.length() -1;
    if (col < 0 ) {
      if(line !=0){
	col = strlen(textLine(line - 1));
	line --;
      }
      else{

	int query = QMessageBox::query("Find", "Beginning of document reached.\n"\
				   "Continue from the end?", "Yes","No");

	if (query){
	  QString string = textLine( numLines() - 1 );
	  line = numLines() - 1;
	  col  = string.length();
	  last_replace = BACKWARD;
	}
      }
    }
  }

again:

  //  printf("Col %d \n",col);

  int  result = doReplace(to_find_string, replace_dialog->case_sensitive(),
			 FALSE, (!replace_dialog->get_direction()), line, col, FALSE );
    
  if(result == 0){
    if(!replace_dialog->get_direction()){ // forward search
    
      int query = QMessageBox::query("Find", "End of document reached.\n"\
				   "Continue from the beginning?", "Yes","No");
      if (query){
	line = 0;
	col = 0;
	goto again;
      }
    }
    else{ //backward search
      
      int query = QMessageBox::query("Find", "Beginning of document reached.\n"\
				   "Continue from the end?", "Yes","No");
      if (query){
	QString string = textLine( numLines() - 1 );
	line = numLines() - 1;
	col  = string.length();
	last_replace = BACKWARD;
	goto again;
      }
    }
  }
  else{

    emit CursorPositionChanged(); 
  }
}



void KEdit::replacedone_slot(){

  if (!replace_dialog)
    return;
  
  replace_dialog->hide();
  //  replace_dialog->clearFocus();

  this->setFocus();

  last_replace = NONE;
  can_replace  = FALSE;

}



int KEdit::doReplace(QString s_pattern, bool case_sensitive, 
	   bool wildcard, bool forward, int line, int col, bool replace_all){


  (void) wildcard; // reserved for possible extension to regex

  int line_counter, length;  
  int pos = -1;

  QString string;
  QString stringnew;
  QString replacement;
  
  replacement = replace_dialog->getReplaceText();
  line_counter = line;
  replace_all_col = col;

  if(forward){

    int num_lines = numLines();

    while (line_counter < num_lines){
      
      string = "";
      string = textLine(line_counter);

      if (replace_all){
	pos = string.find(s_pattern, replace_all_col, case_sensitive);
      }
      else{
	pos = string.find(s_pattern, line_counter == line ? col : 0, case_sensitive);
      }

      if (pos == -1 ){
	line_counter ++;
	replace_all_col = 0;
	replace_all_line = line_counter;
      }

      if( pos != -1){

	length = s_pattern.length();
	
	if(replace_all){ // automatic

	  stringnew = string.copy();
	  stringnew.replace(pos,length,replacement);

	  removeLine(line_counter);
	  insertLine(stringnew.data(),line_counter);

	  replace_all_col = replace_all_col + replacement.length();
	  replace_all_line = line_counter;

	  setModified();
	}
	else{ // interactive

	  setCursorPosition( line_counter , pos, FALSE );

	  for(int l = 0 ; l < length; l++){
	    cursorRight(TRUE);
	  }

	  setCursorPosition( line_counter , pos + length, TRUE );
	  pattern = s_pattern;
	  last_replace = FORWARD;
	  can_replace = TRUE;

	  return 1;

	}
	
      }
    }
  }
  else{ // searching backwards

    while(line_counter >= 0){

      string = "";
      string = textLine(line_counter);

      int line_length = string.length();

      if( replace_all ){
      	pos = string.findRev(s_pattern, replace_all_col , case_sensitive);
      }
      else{
	pos = string.findRev(s_pattern, 
			   line == line_counter ? col : line_length , case_sensitive);
      }

      if (pos == -1 ){
	line_counter --;

	if(line_counter >= 0){
	  string = "";
	  string = textLine(line_counter);
	  replace_all_col = string.length();
	  
	}
	replace_all_line = line_counter;
      }


      if (pos != -1){
	length = s_pattern.length();

	if(replace_all){ // automatic

	  stringnew = string.copy();
	  stringnew.replace(pos,length,replacement);

	  removeLine(line_counter);
	  insertLine(stringnew.data(),line_counter);

	  replace_all_col = replace_all_col - replacement.length();
	  replace_all_line = line_counter;

	  setModified();

	}
	else{ // interactive

	  //	  printf("line_counter %d pos %d col %d\n",line_counter, pos,col);
	  if( ! (line == line_counter && pos > col ) ){

	    setCursorPosition(line_counter ,pos ,FALSE );
      
	    for(int l = 0 ; l < length; l++){
	      cursorRight(TRUE);
	    }	
	
	    setCursorPosition(line_counter ,pos + length ,TRUE );
	    pattern = s_pattern;

	    last_replace = BACKWARD;
	    can_replace = TRUE;

	    return 1;
	  }
	}
      }
    }
  }
  
  return 0;

}





////////////////////////////////////////////////////////////////////
//
// Find Dialog
//


KEdSrch::KEdSrch(QWidget *parent, const char *name)
    : QDialog(parent, name,TRUE){

    this->setFocusPolicy(QWidget::StrongFocus);
    frame1 = new QGroupBox("Find", this, "frame1");

    value = new QLineEdit( this, "value");
    value->setFocus();
    connect(value, SIGNAL(returnPressed()), this, SLOT(ok_slot()));

    sensitive = new QCheckBox("Case Sensitive", this, "case");
    direction = new QCheckBox("Find Backwards", this, "direction");

    ok = new QPushButton("Find", this, "find");
    connect(ok, SIGNAL(clicked()), this, SLOT(ok_slot()));

    cancel = new QPushButton("Done", this, "cancel");
    connect(cancel, SIGNAL(clicked()), this, SLOT(done_slot()));
    //    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));

    setFixedSize(330, 130);

}

void KEdSrch::focusInEvent( QFocusEvent *)
{
    value->setFocus();
    //value->selectAll();
}

QString KEdSrch::getText() { return value->text(); }

void KEdSrch::setText(QString string){

  value->setText(string);

}

void KEdSrch::done_slot(){

  emit search_done_signal();

}


bool KEdSrch::case_sensitive(){

  return sensitive->isChecked();

}

bool KEdSrch::get_direction(){

  return direction->isChecked();

}


void KEdSrch::ok_slot(){

  QString text;

  text = value->text();

  if (!text.isEmpty())
    emit search_signal();

}


void KEdSrch::resizeEvent(QResizeEvent *){


    frame1->setGeometry(5, 5, width() - 10, 80);
    cancel->setGeometry(width() - 80, height() - 30, 70, 25);
    ok->setGeometry(10, height() - 30, 70, 25);
    value->setGeometry(20, 25, width() - 40, 25);
    sensitive->setGeometry(20, 55, 110, 25);
    direction->setGeometry(width()- 20 - 130, 55, 130, 25);

}



////////////////////////////////////////////////////////////////////
//
//  Replace Dialog
//


KEdReplace::KEdReplace(QWidget *parent, const char *name)
    : QDialog(parent, name,TRUE){


    this->setFocusPolicy(QWidget::StrongFocus);

    frame1 = new QGroupBox("Find:", this, "frame1");

    value = new QLineEdit( this, "value");
    value->setFocus();
    connect(value, SIGNAL(returnPressed()), this, SLOT(ok_slot()));

    replace_value = new QLineEdit( this, "replac_value");
    connect(replace_value, SIGNAL(returnPressed()), this, SLOT(ok_slot()));

    label = new QLabel(this,"Rlabel");
    label->setText("Replace with:");

    sensitive = new QCheckBox("Case Sensitive", this, "case");
    sensitive->setChecked(TRUE);

    direction = new QCheckBox("Find Backwards", this, "direction");
    
    ok = new QPushButton("Find", this, "find");
    connect(ok, SIGNAL(clicked()), this, SLOT(ok_slot()));

    replace = new QPushButton("Replace", this, "rep");
    connect(replace, SIGNAL(clicked()), this, SLOT(replace_slot()));

    replace_all = new QPushButton("Replace All", this, "repall");
    connect(replace_all, SIGNAL(clicked()), this, SLOT(replace_all_slot()));

    cancel = new QPushButton("Done", this, "cancel");
    connect(cancel, SIGNAL(clicked()), this, SLOT(done_slot()));

    setFixedSize(330, 180);

}


void KEdReplace::focusInEvent( QFocusEvent *){

    value->setFocus();
    // value->selectAll();
}

QString KEdReplace::getText() { return value->text(); }

QString KEdReplace::getReplaceText() { return replace_value->text(); }

void KEdReplace::setText(QString string) { 

  value->setText(string); 

}

void KEdReplace::done_slot(){

  emit replace_done_signal();

}


void KEdReplace::replace_slot(){

  emit replace_signal();

}

void KEdReplace::replace_all_slot(){

  emit replace_all_signal();

}


bool KEdReplace::case_sensitive(){

  return sensitive->isChecked();

}


bool KEdReplace::get_direction(){

  return direction->isChecked();

}


void KEdReplace::ok_slot(){

  QString text;
  text = value->text();

  if (!text.isEmpty())
    emit find_signal();

}


void KEdReplace::resizeEvent(QResizeEvent *){

    frame1->setGeometry(5, 5, width() - 10, 135);

    cancel->setGeometry(width() - 80, height() - 30, 70, 25);
    ok->setGeometry(10, height() - 30, 70, 25);
    replace->setGeometry(85, height() - 30, 70, 25);
    replace_all->setGeometry(160, height() - 30, 85, 25);

    value->setGeometry(20, 25, width() - 40, 25);
    replace_value->setGeometry(20, 80, width() - 40, 25);
    
    label->setGeometry(20,55,80,20);
    sensitive->setGeometry(20, 110, 110, 25);
    direction->setGeometry(width()- 20 - 130, 110, 130, 25);

}




void KEdGotoLine::selected(int)
{
	accept();
}

KEdGotoLine::KEdGotoLine( QWidget *parent, const char *name)
	: QDialog( parent, name, TRUE )
{
	frame = new QGroupBox( "Goto Line", this );
	lineNum = new KIntLineEdit( this );
	this->setFocusPolicy( QWidget::StrongFocus );
	connect(lineNum, SIGNAL(returnPressed()), this, SLOT(accept()));

	ok = new QPushButton("Go", this );
	cancel = new QPushButton("Cancel", this ); 

	connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
	resize(300, 120); 

}

void KEdGotoLine::resizeEvent(QResizeEvent *)
{
    frame->setGeometry(5, 5, width() - 10, 80);
    cancel->setGeometry(width() - 80, height() - 30, 70, 25);
    ok->setGeometry(10, height() - 30, 70, 25);
    lineNum->setGeometry(20, 35, width() - 40, 25);
}

void KEdGotoLine::focusInEvent( QFocusEvent *)
{
    lineNum->setFocus();
    lineNum->selectAll();
}

int KEdGotoLine::getLineNumber()
{
	return lineNum->getValue();
}
