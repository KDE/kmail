/*
    simplestringlisteditor.h

    A simple widget to edit a list of strings.
    Copyright (c) 2001 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
    02111-1307, US
*/

#include "simplestringlisteditor.h"

#include <klineeditdlg.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

#include <qpushbutton.h>
#include <qlayout.h>


//********************************************************
// SimpleStringListEditor
//********************************************************

// small helper function:
static inline QListBoxItem * findSelectedItem( QListBox * lb ) {
  QListBoxItem * item = 0;
  for ( item = lb->firstItem() ; item && !item->isSelected() ;
	item = item->next() ) ;
  return item;
}

SimpleStringListEditor::SimpleStringListEditor( QWidget * parent,
						const char * name,
						ButtonCode buttons,
						const QString & addLabel,
						const QString & removeLabel,
						const QString & modifyLabel,
						const QString & addDialogLabel )
  : QWidget( parent, name ),
    mAddButton(0), mRemoveButton(0), mModifyButton(0),
    mUpButton(0), mDownButton(0),
    mAddDialogLabel( addDialogLabel.isEmpty() ?
		     i18n("New entry:") : addDialogLabel )
{
  QHBoxLayout * hlay = new QHBoxLayout( this, 0, KDialog::spacingHint() );
  
  mListBox = new QListBox( this );
  hlay->addWidget( mListBox, 1 );

  if ( buttons == None )
    kdDebug(5006) << "SimpleStringListBox called with no buttons. "
      "Consider using a plain QListBox instead!" << endl;

  QVBoxLayout * vlay = new QVBoxLayout( hlay ); // inherits spacing

  if ( buttons & Add ) {
    if ( addLabel.isEmpty() )
      mAddButton = new QPushButton( i18n("&Add..."), this );
    else
      mAddButton = new QPushButton( addLabel, this );
    mAddButton->setAutoDefault( false );
    vlay->addWidget( mAddButton );
    connect( mAddButton, SIGNAL(clicked()),
	     this, SLOT(slotAdd()) );
  }
  
  if ( buttons & Remove ) {
    if ( removeLabel.isEmpty() )
      mRemoveButton = new QPushButton( i18n("&Remove"), this );
    else
      mRemoveButton = new QPushButton( removeLabel, this );
    mRemoveButton->setAutoDefault( false );
    mRemoveButton->setEnabled( false ); // no selection yet
    vlay->addWidget( mRemoveButton );
    connect( mRemoveButton, SIGNAL(clicked()),
	     this, SLOT(slotRemove()) );
  }

  if ( buttons & Modify ) {
    if ( modifyLabel.isEmpty() )
      mModifyButton = new QPushButton( i18n("&Modify..."), this );
    else
      mModifyButton = new QPushButton( modifyLabel, this );
    mModifyButton->setAutoDefault( false );
    mModifyButton->setEnabled( false ); // no selection yet
    vlay->addWidget( mModifyButton );
    connect( mModifyButton, SIGNAL(clicked()),
	     this, SLOT(slotModify()) );
  }

  if ( buttons & Up ) {
    if ( !(buttons & Down) )
      kdDebug(5006) << "Are you sure you want to use an Up button "
	"without a Down button??" << endl;
    mUpButton = new QPushButton( QString::null, this );
    mUpButton->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
    mUpButton->setAutoDefault( false );
    mUpButton->setEnabled( false ); // no selection yet
    vlay->addWidget( mUpButton );
    connect( mUpButton, SIGNAL(clicked()),
	     this, SLOT(slotUp()) );
  }

  if ( buttons & Down ) {
    if ( !(buttons & Up) )
      kdDebug(5006) << "Are you sure you want to use a Down button "
	"without an Up button??" << endl;
    mDownButton = new QPushButton( QString::null, this );
    mDownButton->setPixmap( BarIcon( "down", KIcon::SizeSmall ) );
    mDownButton->setAutoDefault( false );
    mDownButton->setEnabled( false ); // no selection yet
    vlay->addWidget( mDownButton );
    connect( mDownButton, SIGNAL(clicked()),
	     this, SLOT(slotDown()) );
  }

  vlay->addStretch( 1 ); // spacer

  connect( mListBox, SIGNAL(selectionChanged()),
	   this, SLOT(slotSelectionChanged()) );
}

void SimpleStringListEditor::setStringList( const QStringList & strings ) {
  mListBox->clear();
  mListBox->insertStringList( strings );
}

void SimpleStringListEditor::appendStringList( const QStringList & strings ) {
  mListBox->insertStringList( strings );
}

QStringList SimpleStringListEditor::stringList() const {
  QStringList result;
  for ( QListBoxItem * item = mListBox->firstItem() ;
	item ; item = item->next() )
    result << item->text();
  return result;
}

void SimpleStringListEditor::setButtonText( ButtonCode button,
					    const QString & text ) {
  switch ( button ) {
  case Add:
    if ( !mAddButton ) break;
    mAddButton->setText( text );
    return;
  case Remove:
    if ( !mRemoveButton ) break;
    mRemoveButton->setText( text );
    return;
  case Modify:
    if ( !mModifyButton ) break;
    mModifyButton->setText( text );
    return;
  case Up:
  case Down:
    kdDebug(5006) << "SimpleStringListEditor: Cannot change text of "
      "Up and Down buttons: they don't contains text!" << endl;
    return;
  default:
    if ( button & All )
      kdDebug(5006) << "SimpleStringListEditor::setButtonText: No such button!"
		    << endl;
    else
      kdDebug(5006) << "SimpleStringListEditor::setButtonText: Can only set "
	"text for one button at a time!" << endl;
    return;
  }

  kdDebug(5006) << "SimpleStringListEditor::setButtonText: the requested "
    "button has not been created!" << endl;
}

void SimpleStringListEditor::slotAdd() {
  bool ok = false;
  QString newEntry = KLineEditDlg::getText( mAddDialogLabel, QString::null,
					    &ok, this );
  // let the user verify the string before adding
  emit aboutToAdd( newEntry );
  if ( ok && !newEntry.isEmpty() )
    mListBox->insertItem( newEntry );
}

void SimpleStringListEditor::slotRemove() {
  delete findSelectedItem( mListBox ); // delete 0 is well-behaved...
}

void SimpleStringListEditor::slotModify() {
  QListBoxItem * item = findSelectedItem( mListBox );
  if ( !item ) return;

  bool ok = false;
  QString newText = KLineEditDlg::getText( mAddDialogLabel, item->text(),
					   &ok, this );
  emit aboutToAdd( newText );
  if ( !ok || newText.isEmpty() || newText == item->text() ) return;

  int index = mListBox->index( item );
  delete item;
  mListBox->insertItem( newText, index );
  mListBox->setCurrentItem( index );
}

void SimpleStringListEditor::slotUp() {
  QListBoxItem * item = findSelectedItem( mListBox );
  if ( !item || !item->prev() ) return;

  // find the item that we want to insert after:
  QListBoxItem * pprev = item->prev()->prev();
  // take the item from it's current position...
  mListBox->takeItem( item );
  // ...and insert it after the above mentioned item:
  mListBox->insertItem( item, pprev );
  // make sure the old item is still the current one:
  mListBox->setCurrentItem( item );
  // enable and disable controls:
  if ( mRemoveButton )
    mRemoveButton->setEnabled( true );
  if ( mModifyButton )
    mModifyButton->setEnabled( true );
  if ( mUpButton )
    mUpButton->setEnabled( item->prev() );
  if ( mDownButton )
    mDownButton->setEnabled( true );
}

void SimpleStringListEditor::slotDown() {
  QListBoxItem * item  = findSelectedItem( mListBox );
  if ( !item || !item->next() ) return;

  // find the item that we want to insert after:
  QListBoxItem * next = item->next();
  // take the item from it's current position...
  mListBox->takeItem( item );
  // ...and insert it after the above mentioned item:
  if ( next )
    mListBox->insertItem( item, next );
  else
    mListBox->insertItem( item );
  // make sure the old item is still the current one:
  mListBox->setCurrentItem( item );
  // enable and disable controls:
  if ( mRemoveButton )
    mRemoveButton->setEnabled( true );
  if ( mModifyButton )
    mModifyButton->setEnabled( true );
  if ( mUpButton )
    mUpButton->setEnabled( true );
  if ( mDownButton )
    mDownButton->setEnabled( item->next() );
}

void SimpleStringListEditor::slotSelectionChanged() {
  // try to find a selected item:
  QListBoxItem * item = findSelectedItem( mListBox );

  // if there is one, item will be non-null (ie. true), else 0
  // (ie. false):
  if ( mRemoveButton )
    mRemoveButton->setEnabled( item );
  if ( mModifyButton )
    mModifyButton->setEnabled( item );
  if ( mUpButton )
    mUpButton->setEnabled( item && item->prev() );
  if ( mDownButton )
    mDownButton->setEnabled( item && item->next() );
}



#include "simplestringlisteditor.moc"
