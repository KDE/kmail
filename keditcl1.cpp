 /*

  $Id$
 
  KEdit, a simple text editor for the KDE project
  
  Copyright (C) 1997 Bernd Johannes Wuebben   
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
  
  KEdit, simple editor class, hacked version of the original by 

 
  */

#include "keditcl.h"
#include <klocale.h>
#include <kapp.h>


#include "keditcl.moc"

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
    fill_column_value = 80;
    autoIndentMode = false;
    reduce_white_on_justify = true;

    current_directory = QDir::currentDirPath();

    make_backup_copies = TRUE;
    
    installEventFilter( this );     

    srchdialog = NULL;
    replace_dialog= NULL;
    file_dialog = NULL;
    gotodialog = NULL;

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

void  KEdit::setFillColumnMode(int line, bool set){
  fill_column_is_set = set;
  fill_column_value = line;


}


int KEdit::loadFile(QString name, int mode){

    int fdesc;
    struct stat s;
    char *addr;
    QFileInfo info(name);

    if(!info.exists()){
      QMessageBox::warning(
		  this,
		  klocale->translate("Sorry"),
		  klocale->translate("The specified File does not exist"),
		  klocale->translate("OK"),
		  "",
		  "",
		  0,0
		  );

      return KEDIT_RETRY;
    }

    if(info.isDir()){
      QMessageBox::warning(
		   this,
		   klocale->translate("Sorry:"),
		   klocale->translate("You have specificated a directory"),
		   klocale->translate("OK"),
		  "",
		  "",
		  0,0
		   );		

      return KEDIT_RETRY;
    }


   if(!info.isReadable()){
      QMessageBox::warning(
		  this,
		  klocale->translate("Sorry"),
                  klocale->translate("You do not have read permission to this file."),
                  klocale->translate("OK"),
		  "",
		  "",
		  0,0
		  );


      return KEDIT_RETRY;
    }


    fdesc = open(name, O_RDONLY);

    if(fdesc == -1) {
        switch(errno) {
        case EACCES:

	  QMessageBox::warning(
		  this,
		  klocale->translate("Sorry"),
                  klocale->translate("You do not have read permission to this file."),
                  klocale->translate("OK"),
		  "",
		  "",
		  0,0
		  );

	  return KEDIT_OS_ERROR;

        default:
            QMessageBox::warning(
		  this, 
		  klocale->translate("Sorry"),				 
		  klocale->translate("An Error occured while trying to open this Document"),
                  klocale->translate("OK"),
		  "",
		  "",
		  0,0
		  );
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

    box = getFileDialog(klocale->translate("Select Document to Insert"));

    box->show();
    
    if (!box->result()) {
      return KEDIT_USER_CANCEL;
    }
    
    if(box->selectedFile().isEmpty()) {  /* no selection */
      return KEDIT_USER_CANCEL;
    }
    
    file_to_insert = box->selectedFile();
    file_to_insert.detach();
    
    
    int result = loadFile(file_to_insert, OPEN_INSERT);

    if (result == KEDIT_OK )
      setModified();

    return  result;


}

int KEdit::openFile(int mode)
{
    QString fname;
    QFileDialog *box;

  int result;

    if( isModified() ) {           
      switch( QMessageBox::warning( 
			 this,
			 klocale->translate("Warning:"), 	
			 klocale->translate("The current Document has been modified.\n"\
					    "Would you like to save it?"),
			 klocale->translate("Yes"),
			 klocale->translate("No"),
			 klocale->translate("Cancel"),
                                  0, 2 
			 )
	      )
	{
	case 0: // Yes or Enter

	result = doSave();

	if ( result == KEDIT_USER_CANCEL)
	  return KEDIT_USER_CANCEL;

	if (result != KEDIT_OK){

	  switch(QMessageBox::warning(
			   this,
			   klocale->translate("Sorry:"), 
			   klocale->translate("Could not save the document.\n"\
                                              "Open a new document anyways?"), 
			   klocale->translate("Yes"),
                           klocale->translate("No"),
			       "",
			       0,1
			   )
		 )
	    {

	    case 0:
	      break;
	    case 1:
	      return KEDIT_USER_CANCEL;
	      break;
	    }

	}

        break;

	case 1: // No 

	  break;
	case 2: // cancel
	  return KEDIT_USER_CANCEL;
	  break;

	}
    }
            
    box = getFileDialog(klocale->translate("Select Document to Open"));
    
    box->show();
    
    if (!box->result())   /* cancelled */
      return KEDIT_USER_CANCEL;
    if(box->selectedFile().isEmpty()) {  /* no selection */
      return KEDIT_USER_CANCEL;
    }
    
    fname =  box->selectedFile();
    
    int result2 =  loadFile(fname, mode);
    
    if ( result2 == KEDIT_OK )
      toggleModified(FALSE);
    
    return result2;
	
        
}

int KEdit::newFile(){

  int result;

    if( isModified() ) {           
      switch( QMessageBox::warning( 
			 this,
			 klocale->translate("Warning:"), 	
			 klocale->translate("The current Document has been modified.\n"\
					    "Would you like to save it?"),
			 klocale->translate("Yes"),
			 klocale->translate("No"),
			 klocale->translate("Cancel"),
                                  0, 2 )
	      )
	{
	
	case 0: // Yes or Enter

	  result = doSave();

	  if ( result == KEDIT_USER_CANCEL)
	    return KEDIT_USER_CANCEL;

	  if (result != KEDIT_OK){

	    switch(QMessageBox::warning(this,
			   klocale->translate("Sorry:"), 
			   klocale->translate("Could not save the document.\n"\
                                              "Create a new document anyways?"), 
			   klocale->translate("Yes"),
                           klocale->translate("No"),
			       "",
			       0,1
				      )){

	  case 0:
	    break;
	  case 1:
	    return KEDIT_USER_CANCEL;
	    break;
	  }

	}

        break;

      case 1: // No 

        break;
      case 2: // cancel
	 return KEDIT_USER_CANCEL;
	 break;

      }
    }
    

    this->clear();
    toggleModified(FALSE);

    setFocus();

    filename = klocale->translate("Untitled");
       
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


  if ((e->state() & ControlButton ) && (e->key() == Key_K) ){

    int line = 0;
    int col  = 0;
    QString killstring;

    if(!killing){
      killbufferstring = "";
      killtrue = false;
      lastwasanewline = false;
    }

    getCursorPosition(&line,&col);
    killstring = textLine(line);
    killstring.detach();
    killstring = killstring.mid(col,killstring.length());


    if(!killbufferstring.isEmpty() && !killtrue && !lastwasanewline){
      killbufferstring += "\n";
    }

    if( (killstring.length() == 0) && !killtrue){
      killbufferstring += "\n";
      lastwasanewline = true;
    }

    if(killstring.length() > 0){

      killbufferstring += killstring;
      lastwasanewline = false;
      killtrue = true;

    }else{

      lastwasanewline = false;
      killtrue = !killtrue;

    }

    killing = true;

    QMultiLineEdit::keyPressEvent(e);
    setModified();
    emit CursorPositionChanged();
    return;
  }

  if ((e->state() & ControlButton ) && (e->key() == Key_Y) ){

    int line = 0;
    int col  = 0;

    getCursorPosition(&line,&col);

    if(!killtrue)
      killbufferstring += '\n';

    insertAt(killbufferstring,line,col);

    killing = false;
    setModified();
    emit CursorPositionChanged();
    return;
  }

  killing = false;

  if (e->key() == Key_Insert){
    this->setOverwriteMode(!this->isOverwriteMode());
    emit toggle_overwrite_signal();
    return;
  }

  if( fill_column_value > 20 ){

    if ((e->state() & ControlButton ) && (e->key() == Key_J) ){

	int templine,tempcol;
	getCursorPosition(&templine,&tempcol);	
	setAutoUpdate(false);
	int upperbound;

	bool did_format = format2(par,upperbound);

	if(!did_format)
	  return;
    
	int num_of_rows = 0;
	QString newpar;

	for( int k = 0; k < (int)par.count(); k++){
	  newpar += par.at(k);
	  if(k != (int)par.count() -1 )
	    newpar += '\n';
	}
	insertLine(newpar,upperbound);
	//printf("%s\n",newpar.data());
	newpar = "";
	num_of_rows = par.count();
	par.clear();
    
	setCursorPosition(templine,tempcol);

	setAutoUpdate(true);
	repaint();
    
	computePosition();
	setModified();
	emit CursorPositionChanged();

	return;
    }
  }

  
  if(fill_column_is_set && word_wrap_is_set ){

    // word break algorithm
    if(isprint(e->ascii()) || e->key() == Key_Tab || e->key() == Key_Return || 
       e->key() == Key_Enter){

	if (e->key() == Key_Return || e->key() == Key_Enter){ 
	  mynewLine();
	}
	else{
	  if (e->key() == Key_Tab){
	    if (isReadOnly())
	      return;
	    QMultiLineEdit::insertChar((char)'\t');
	  }
	  else{
	    QMultiLineEdit::keyPressEvent(e); 
	  }
	}

	int templine,tempcol;
	getCursorPosition(&templine,&tempcol);	
	//	printf("GETCURSOR %d %d\n",templine,tempcol);
	
	computePosition();
	//	printf("LINEPOS %d\n",col_pos);
	setAutoUpdate(false);
	
	bool did_break = format(par);
	int num_of_rows = 0;

	if(did_break){

	  QString newpar;

	  for( int k = 0; k < (int)par.count(); k++){
	    newpar += par.at(k);
	    if(k != (int)par.count() -1 )
	      newpar += '\n';
	  }
	  insertLine(newpar,templine);
	  //printf("%s\n",newpar.data());
	  newpar = "";
	  num_of_rows = par.count();
	  par.clear();


	  if(col_pos +1 > fill_column_value){
	    setCursorPosition(templine+1,cursor_offset);
	    //	    printf("SETCURSOR1 %d %d\n",templine +1,cursor_offset);
	  }
	  else{
	    setCursorPosition(templine,tempcol);
	    //	    printf("SETCURSOR2 %d %d\n",templine ,tempcol);

	  }
	}

	setAutoUpdate(true);

	// Let's try to reduce flicker by updating only what we need to update
	// printf("NUMOFROWS %d\n",num_of_rows);

	if(did_break){
	  int y1  = -1;
	  int y2  = -1;

	  rowYPos(templine,&y1);
	  rowYPos(templine + num_of_rows -1,&y2);

	  if(y1 == -1)
	    y1 = 0;

	  if(y2 == -1)
	    y2 = this->height();

	  repaint(0,y1,this->width(),y2);
	}

	computePosition();
	setModified();
	emit CursorPositionChanged();
	return;
    }
    
    QMultiLineEdit::keyPressEvent(e); 
    computePosition();
    emit CursorPositionChanged();
    return;

  } // end do_wordbreak && fillcolumn_set
  

  // fillcolumn but no wordbreak

  if (fill_column_is_set){

    if (e->key() == Key_Tab){
      if (isReadOnly())
	return;
      QMultiLineEdit::insertChar((char)'\t');
      emit CursorPositionChanged();
      return;
    }

    if(e->key() == Key_Return || e->key() == Key_Enter){
    
      mynewLine();
      emit CursorPositionChanged();
      return;

    }

    if(isprint(e->ascii())){
    
      if( col_pos >= fill_column_value ){ 
	  mynewLine();
      }

    }

    QMultiLineEdit::keyPressEvent(e);
    emit CursorPositionChanged();
    return;

  }

  // default action
  if(e->key() == Key_Return || e->key() == Key_Enter){
    
    mynewLine();
    emit CursorPositionChanged();
    return;

  }

  if (e->key() == Key_Tab){
    if (isReadOnly())
      return;
    QMultiLineEdit::insertChar((char)'\t');
    emit CursorPositionChanged();
    return;
  }

  QMultiLineEdit::keyPressEvent(e);
  emit CursorPositionChanged();

}


bool KEdit::format(QStrList& par){

  QString mstring;
  QString pstring;

  int space_pos;
  int right; /* char to right of space */

  int templine,tempcol;

  getCursorPosition(&templine,&tempcol);
  mstring = textLine(templine);

  /*  if((int)mstring.length() <= fill_column_value)
    return false;*/

  int l = 0;
  int k = 0;

  for( k = 0, l = 0; k < (int) mstring.length() && l <= fill_column_value; k++){
    
    if(mstring.data()[k] == '\t')
      l +=8 - l%8;
    else
      l ++;

  }

  if( l <= fill_column_value)
    return false;

  // ########################## TODO consider a autoupdate(false) aroudn getpar #####
  getpar(templine,par);
  // ########################## TODO consider a autoupdate(false) aroudn getpar #####

  /*
    printf("\n");
    for ( int i = 0 ; i < (int)par.count() ; i ++){
       printf("%s\n",par.at(i));
    }
    printf("\n");
    */

  for ( int i = 0 ; i < (int)par.count() ; i ++){
    //    printf("par.count %d line %d\n",par.count(),i);
    k = 0;
    l = 0;
    int last_ok = 0;
    pstring = par.at(i);

    /*    if((int)pstring.length() <= fill_column_value)
      break;*/

    for( k = 0, l = 0; k < (int) pstring.length() && l <= fill_column_value; k++){
    
      if(pstring.data()[k] == '\t')
        l +=8 - l%8;
      else
	l ++;

      if( l <= fill_column_value )
	last_ok = k;
    }

    if( l <= fill_column_value)
      break;

    mstring = pstring.left( k );
    space_pos = mstring.findRev( " ", -1, TRUE );
    if(space_pos == -1) // well let try to find a TAB then ..
          space_pos = mstring.findRev( "\t", -1, TRUE );

    right = col_pos - space_pos - 1;
  
    if( space_pos == -1 ){ 

      // no space to be found on line, just break, what else could we do?
      par.remove(i);
      par.insert(i,pstring.left(last_ok+1));

      if(i < (int)par.count() - 1){
	QString temp1 = par.at(i+1);
	QString temp2;
	if(autoIndentMode){
	  temp1 = temp1.mid(prefixString(temp1).length(),temp1.length());
	}
	temp2 = pstring.mid(last_ok +1,pstring.length()) + (QString) " " + temp1;
	temp1 = temp2.copy();
	if(autoIndentMode)
	  temp1 = prefixString(pstring) + temp1;
	par.remove(i+1);
	par.insert(i+1,temp1);

      }
      else{
	if(autoIndentMode)
	  par.append(prefixString(pstring) + pstring.mid(last_ok + 1,pstring.length()));
	else
	  par.append(pstring.mid(last_ok +1,pstring.length()));
      }
      if(i==0){
	cursor_offset = pstring.length() - last_ok -1;
	if(autoIndentMode)
	  cursor_offset += prefixString(pstring).length();
	//printf("CURSOROFFSET1 %d\n",cursor_offset);
      }
    }
    else{
    
      par.remove(i);
      par.insert(i,pstring.left(space_pos));

      if(i < (int)par.count() - 1){
	QString temp1 = par.at(i+1);
	QString temp2;
	if(autoIndentMode){
	  temp1 = temp1.mid(prefixString(temp1).length(),temp1.length());
	}
	temp2 = pstring.mid(space_pos +1,pstring.length()) + (QString) " " + temp1;
	temp1 = temp2.copy();
	if(autoIndentMode)
	  temp1 = prefixString(pstring) + temp1;
	par.remove(i+1);
	par.insert(i+1,temp1);
      }
      else{
	if(autoIndentMode)
	  par.append(prefixString(pstring) + pstring.mid(space_pos + 1,pstring.length()));
	else
	  par.append(pstring.mid(space_pos+1,pstring.length()));
      }
      if(i==0){
	cursor_offset = pstring.length() - space_pos -1;
	if(autoIndentMode)
	  cursor_offset += prefixString(pstring).length();
	//	printf("CURSOROFFSET2 %d\n",cursor_offset);
      }
    }
    
  }

  return true;

}


void KEdit::getpar(int line,QStrList& par){

  QString linestr;

  par.clear();
  int num = numLines();
  for(int i = line ; i < num ; i++){
    linestr = textLine(line);
    if(linestr.isEmpty())
      break;
    par.append(linestr);
    removeLine(line);
  }

}


bool KEdit::format2(QStrList& par, int& upperbound){

  QString mstring;
  QString pstring;
  QString prefix;
  int right, space_pos;
  int fill_column = fill_column_value;

  int templine,tempcol;
  getCursorPosition(&templine,&tempcol);

  // ########################## TODO consider a autoupdate(false) around getpar #####
  getpar2(templine,par,upperbound,prefix);
  // ########################## TODO consider a autoupdate(false) around getpar #####

  int k = 0;
  int l = 0;
  int last_ok = 0;

  for( k = 0, l = 0; k < (int) prefix.length(); k++){
    
    if(prefix.data()[k] == '\t')
      l +=8 - l%8;
    else
      l ++;
  }

  fill_column_value = fill_column_value - l;

  // safety net --
  if(fill_column_value < 10 )
    fill_column_value = 10;

  if(par.count() == 0 ){
    fill_column_value = fill_column;
    return false;
  }
  
  /*
   printf("PASS 1\n");	
   for ( int i = 0 ; i < (int)par.count() ; i ++){
     printf("%s\n",par.at(i));
   }
   printf("\n");
   */
   // first pass: break overfull lines

  for ( int i = 0 ; i < (int)par.count() ; i ++){
    //    printf("par.count %d line %d\n",par.count(),i);
    k = 0;
    l = 0;
    last_ok = 0;
    pstring = par.at(i);

    for( k = 0, l = 0; k < (int) pstring.length() && l <= fill_column_value; k++){
    
      if(pstring.data()[k] == '\t')
        l +=8 - l%8;
      else
	l ++;

      if( l <= fill_column_value )
	last_ok = k;
    }

    if( l <= fill_column_value)
      continue;

    mstring = pstring.left( k );
    space_pos = mstring.findRev( " ", -1, TRUE );
    if(space_pos == -1) // well let try to find a TAB then ..
          space_pos = mstring.findRev( "\t", -1, TRUE );

    right = col_pos - space_pos - 1;
  
    if( space_pos == -1 ){ 

      // no space to be found on line, just break, what else could we do?
      par.remove(i);
      par.insert(i,pstring.left(last_ok+1));

      if(i < (int)par.count() - 1){
	QString temp1 = par.at(i+1);
	QString temp2;
	if(autoIndentMode){
	  temp1 = temp1.mid(prefixString(temp1).length(),temp1.length());
	}
	temp2 = pstring.mid(last_ok +1,pstring.length()) + (QString) " " + temp1;
	temp1 = temp2.copy();
	if(autoIndentMode)
	  temp1 = prefixString(pstring) + temp1;
	par.remove(i+1);
	par.insert(i+1,temp1);

      }
      else{
	if(autoIndentMode)
	  par.append(prefixString(pstring) + pstring.mid(last_ok + 1,pstring.length()));
	else
	  par.append(pstring.mid(last_ok +1,pstring.length()));
      }
      if(i==0){
	cursor_offset = pstring.length() - last_ok -1;
	if(autoIndentMode)
	  cursor_offset += prefixString(pstring).length();
	//printf("CURSOROFFSET1 %d\n",cursor_offset);
      }
    }
    else{
    
      par.remove(i);
      par.insert(i,pstring.left(space_pos));

      if(i < (int)par.count() - 1){
	QString temp1 = par.at(i+1);
	QString temp2;
	if(autoIndentMode){
	  temp1 = temp1.mid(prefixString(temp1).length(),temp1.length());
	}
	temp2 = pstring.mid(space_pos +1,pstring.length()) + (QString) " " + temp1;
	temp1 = temp2.copy();
	if(autoIndentMode)
	  temp1 = prefixString(pstring) + temp1;
	par.remove(i+1);
	par.insert(i+1,temp1);
      }
      else{
	if(autoIndentMode)
	  par.append(prefixString(pstring) + pstring.mid(space_pos + 1,pstring.length()));
	else
	  par.append(pstring.mid(space_pos+1,pstring.length()));
      }
      if(i==0){
	cursor_offset = pstring.length() - space_pos -1;
	if(autoIndentMode)
	  cursor_offset += prefixString(pstring).length();
	//	printf("CURSOROFFSET2 %d\n",cursor_offset);
      }
    }
    
  }

  // Second Pass let's see whether we can fill each line more
  /*
   printf("PASS 2\n");	
   for ( int i = 0 ; i < (int)par.count() ; i ++){
     printf("%s\n",par.at(i));
   }
   printf("\n");
   */
  for ( int i = 0 ; i < (int)par.count() - 1 ; i ++){
    //    printf("par.count %d line %d\n",par.count(),i);
    int k = 0;
    int l = 0;
    int last_ok = 0;
    int free = 0;
    pstring = par.at(i);

    // printf("LENGTH of line %d = %d\n",i,pstring.length());

    for( k = 0, l = 0; k < (int) pstring.length() && l <= fill_column_value; k++){
    
      if(pstring.data()[k] == '\t')
        l +=8 - l%8;
      else
	l ++;

      if( l <= fill_column_value )
	last_ok = k;
    }

    if( l >= fill_column_value) // line is full go on to next line
      continue;

    free = fill_column_value - l -1; // -1 for the extra space we need to insert
    //    printf("free %d\n",free);

    mstring = par.at(i+1);

    for( k = 0, l = 0; k < (int) mstring.length() && l <= fill_column_value; k++){
    
      if(mstring.data()[k] == '\t')
        l +=8 - l%8;
      else
	l ++;

      if( l <= free )
	last_ok = k;
    }

    if( (int) l <= free){
      
      pstring = pstring + QString(" ") + mstring;
      par.remove(i);
      par.insert(i,pstring);
      par.remove(i+1);
      i --; // check again whether line i is full now

    }
    else{

      space_pos = mstring.findRev( " ", QMIN(free,last_ok), TRUE );
      if(space_pos == -1) // well let's try to find a TAB then ..
	space_pos = mstring.findRev( "\t", QMIN(free,last_ok), TRUE );

      if(space_pos == -1)
	continue; // can't find a word to add to the previous line
    
      pstring = pstring + QString(" ") + mstring.left(space_pos );
      //printf("adding: %s %d\n",mstring.left(space_pos).data(),space_pos);
      par.remove(i);
      par.insert(i,pstring);
      mstring = mstring.mid(space_pos + 1,mstring.length());
      par.remove(i+1);
      par.insert(i+1,mstring);
    }
    
  }

  /*   printf("PASS 3\n");	
   for ( int i = 0 ; i < (int)par.count() ; i ++){
     printf("%s\n",par.at(i));
   }
   printf("\n");
   */
   QString prerun;
   for ( int i = 0 ; i < (int)par.count() ; i ++){
     prerun = prefix + (QString) par.at(i);
     par.remove(i);
     par.insert(i,prerun);
   }
   /*
   printf("PASS 4\n");	
   for ( int i = 0 ; i < (int)par.count() ; i ++){
     printf("%s\n",par.at(i));
   }
   printf("\n");
   */
   fill_column_value = fill_column;
   return true;

}


void KEdit::getpar2(int line,QStrList& par,int& upperbound,QString& prefix){

  QString linestr;
  QString string, string2;
  bool foundone = false;

  int line2;
  line2 = line;

  string  = textLine(line2);
  string = string.stripWhiteSpace();

  if(string.isEmpty()){
    upperbound = 0;
    par.clear();
    return;
  }	


  while(line2 >= 0){
    string  = textLine(line2);
    string2 = string.stripWhiteSpace();

    if(string2.isEmpty()){
      foundone = true;
      break;
    }

    line2 --;
  }

  if(foundone)
    line2 ++;

  if(line2 < 0)
    line2 = 0;

  upperbound = line2;

  par.clear();

  prefix = prefixString(textLine(line2));

  int num = numLines();
  for(int i = line2 ; i < num ; i++){
    linestr = textLine(line2);

    if((linestr.stripWhiteSpace()).isEmpty())
      break;
    if(reduce_white_on_justify){
      linestr = linestr.stripWhiteSpace();
    }
    par.append(linestr);
    removeLine(line2);
  }

}

void KEdit::mynewLine(){


  if (isReadOnly())
    return;

  setModified();

  if(!autoIndentMode){ // if not indent mode
    newLine();
    return;
  }

  int line,col, line2;
  bool found_one = false;

  getCursorPosition(&line,&col);
  line2 = line;

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
    newLine();
    insertAt(string.data(),line2 + 1,0);
  }
  else{
    newLine();
  }
}

void KEdit::setAutoIndentMode(bool mode){

  autoIndentMode = mode;

}


QString KEdit::prefixString(QString string){
  
  // This routine return the whitespace before the first non white space
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

    struct stat st;
    int stat_ok = -1;
    bool exists_already;

    if(!modified) {
      emit saving();
      mykapp->processEvents();
      return KEDIT_OK;
    }

    QFile file(filename);
    QString backup_filename;
    exists_already = file.exists();

    if(exists_already){
      stat_ok = stat(filename.data(), &st);
      backup_filename = filename;
      backup_filename.detach();
      backup_filename += '~';

      rename(filename.data(),backup_filename.data());
    }

    if( !file.open( IO_WriteOnly | IO_Truncate )) {
      rename(backup_filename.data(),filename.data());
      QMessageBox::warning(
			   this,
			   klocale->translate("Sorry"),
			   klocale->translate("Could not save the document\n"),
			   klocale->translate("OK"),
			   "",
			   "",
			   0,0
			   );
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

    if(exists_already)
      chmod(filename.data(),st.st_mode);// preseve filepermissions
    
    return KEDIT_OK;

}

void KEdit::setFileName(char* name){

  filename = name;
  filename.detach();

}

void KEdit::saveasfile(char* name){

  QString filenamebackup;
  filenamebackup = filename;
  filename = name;
  filename.detach();
  saveFile();
  filename = filenamebackup;
  filename.detach();

}

QFileDialog* KEdit::getFileDialog(const char* captiontext){

  if(!file_dialog){

    file_dialog = new QFileDialog(current_directory.data(),"*",this,"file_dialog",TRUE);
  }

  file_dialog->setCaption(captiontext);
  file_dialog->rereadDir();

  return file_dialog;
}

int KEdit::saveAs(){
    
  QFileDialog *box;

  QFileInfo info;
  QString tmpfilename;
  int result;
  
  box = getFileDialog(klocale->translate("Save Document As"));

  QPoint point = this->mapToGlobal (QPoint (0,0));

  QRect pos = this->geometry();
  box->setGeometry(point.x() + pos.width()/2  - box->width()/2,
		   point.y() + pos.height()/2 - box->height()/2, 
		   box->width(),box->height());
try_again:


  box->show();
  
  if (!box->result())
    {
      return KEDIT_USER_CANCEL;
    }
  
  if(box->selectedFile().isEmpty()){
    return KEDIT_USER_CANCEL;
  }
  
  info.setFile(box->selectedFile());
  
  if(info.exists()){

    switch( QMessageBox::warning( 
			   this,
			   klocale->translate("Warning:"), 	
			   klocale->translate("A Document with this Name exists already\n"\
						     "Do you want to overwrite it ?"),
                           klocale->translate("Yes"),
			   klocale->translate("No"),
				  "",
                                  1, 1 )
	    ){
    case 0: // Yes or Enter
        // try again
        break;
    case 1: // No or Escape
        goto try_again;
        break;
    }
  }
  
  
  tmpfilename = filename;
  
  filename = box->selectedFile();
  
  // we need this for saveFile();
  modified = TRUE; 
  
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

    if(info.exists() && !info.isWritable()){

      QMessageBox::warning(
			   this,
			   klocale->translate("Sorry:"), 
			   klocale->translate("You do not have write permission"\
					      "to this file.\n"),
			   klocale->translate("OK"),
			   "",
			   "",
			   0,0
			   );

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

  if (ev->type() == Event_Paint)
	{
	if (srchdialog)
		if (srchdialog->isVisible())
			srchdialog->raise();

	if (replace_dialog)
		if (replace_dialog->isVisible())
			replace_dialog->raise();
	}

  

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
		setCursorPosition( gotodialog->getLineNumber()-1 , 0, FALSE );
		emit CursorPositionChanged();
		setFocus();
	}
}

void  KEdit::setReduceWhiteOnJustify(bool reduce){

  reduce_white_on_justify = reduce;

}
