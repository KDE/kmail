/*  -*- mode: C++; c-file-style: "gnu" -*-
    simplestringlisteditor.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2001 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "simplestringlisteditor.h"

#include <kinputdialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kdialog.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>

//********************************************************
// SimpleStringListEditor
//********************************************************

SimpleStringListEditor::SimpleStringListEditor( QWidget * parent,
                                                ButtonCode buttons,
                                                const QString & addLabel,
                                                const QString & removeLabel,
                                                const QString & modifyLabel,
                                                const QString & addDialogLabel )
  : QWidget( parent ),
    mAddButton(0), mRemoveButton(0), mModifyButton(0),
    mUpButton(0), mDownButton(0),
    mAddDialogLabel( addDialogLabel.isEmpty() ?
                     i18n("New entry:") : addDialogLabel )
{
  QHBoxLayout * hlay = new QHBoxLayout( this );
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  mListBox = new QListWidget( this );
  mListBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
  hlay->addWidget( mListBox, 1 );

  if ( buttons == None ) {
    kDebug() << "SimpleStringListBox called with no buttons."
                "Consider using a plain QListBox instead!";
  }

  QVBoxLayout * vlay = new QVBoxLayout(); // inherits spacing
  hlay->addLayout( vlay );

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
    connect( mListBox, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(slotModify()) );
  }

  if ( buttons & Up ) {
    if ( !(buttons & Down) ) {
      kDebug() << "Are you sure you want to use an Up button"
                  "without a Down button??";
    }
    mUpButton = new KPushButton( QString(), this );
    mUpButton->setIcon( KIcon( "go-up" ) );
    mUpButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
    mUpButton->setAutoDefault( false );
    mUpButton->setEnabled( false ); // no selection yet
    vlay->addWidget( mUpButton );
    connect( mUpButton, SIGNAL(clicked()),
             this, SLOT(slotUp()) );
  }

  if ( buttons & Down ) {
    if ( !(buttons & Up) ) {
      kDebug() << "Are you sure you want to use a Down button"
                  "without an Up button??";
    }
    mDownButton = new KPushButton( QString(), this );
    mDownButton->setIcon( KIcon( "go-down" ) );
    mDownButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
    mDownButton->setAutoDefault( false );
    mDownButton->setEnabled( false ); // no selection yet
    vlay->addWidget( mDownButton );
    connect( mDownButton, SIGNAL(clicked()),
             this, SLOT(slotDown()) );
  }

  vlay->addStretch( 1 ); // spacer

  connect( mListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
           this, SLOT(slotSelectionChanged()) );
  connect( mListBox, SIGNAL(itemSelectionChanged()),
           this, SLOT(slotSelectionChanged()) );
}

void SimpleStringListEditor::setStringList( const QStringList & strings ) {
  mListBox->clear();
  mListBox->addItems( strings );
}

void SimpleStringListEditor::appendStringList( const QStringList & strings ) {
  mListBox->addItems( strings );
}

QStringList SimpleStringListEditor::stringList() const {
  QStringList result;
  const int numberOfItem(mListBox->count());
  for ( int i = 0; i < numberOfItem; i++ )
    result << ( mListBox->item( i )->text() );
  return result;
}

bool SimpleStringListEditor::containsString( const QString & str ) {
  const int numberOfItem(mListBox->count());
  for ( int i = 0; i < numberOfItem; i++ ) {
    if ( mListBox->item( i )->text() == str )
      return true;
  }
  return false;
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
    kDebug() << "SimpleStringListEditor: Cannot change text of"
                   "Up and Down buttons: they don't contains text!";
    return;
  default:
    if ( button & All )
      kDebug() << "No such button!";
    else
      kDebug() << "Can only set text for one button at a time!";
    return;
  }

  kDebug() << "The requested button has not been created!";
}

void SimpleStringListEditor::slotAdd() {
  bool ok = false;
  QString newEntry = KInputDialog::getText( i18n("New Value"),
                                            mAddDialogLabel, QString(),
                                            &ok, this );
  // let the user verify the string before adding
  emit aboutToAdd( newEntry );
  if ( ok && !newEntry.isEmpty() && !containsString( newEntry )) {
      mListBox->addItem( newEntry );
      emit changed();
  }
}

void SimpleStringListEditor::slotRemove() {
    QList<QListWidgetItem *> selectedItems = mListBox->selectedItems();
    if(selectedItems.isEmpty())
        return;
    Q_FOREACH(QListWidgetItem *item, selectedItems) {
        delete mListBox->takeItem(mListBox->row(item));
    }
    emit changed();
}

void SimpleStringListEditor::slotModify() {
  QListWidgetItem* item = mListBox->currentItem();
  if ( !item ) return;

  bool ok = false;
  QString newText = KInputDialog::getText( i18n("Change Value"),
                                           mAddDialogLabel, item->text(),
                                           &ok, this );
  emit aboutToAdd( newText );
  if ( !ok || newText.isEmpty() || newText == item->text() ) return;

  item->setText( newText );
  emit changed();
}

QList<QListWidgetItem*> SimpleStringListEditor::selectedItems() const 
{
  QList<QListWidgetItem*> listWidgetItem;
  const int numberOfFilters = mListBox->count();
  for ( int i = 0; i<numberOfFilters; ++i ) {
    if ( mListBox->item(i)->isSelected() ) {
      listWidgetItem << mListBox->item(i);
    }
  }
  return listWidgetItem;
}

void SimpleStringListEditor::slotUp() {

  QList<QListWidgetItem*> listWidgetItem = selectedItems();
  if ( listWidgetItem.isEmpty() ) {
    return;
  }

  const int numberOfItem( listWidgetItem.count() );
  if ( ( numberOfItem == 1 ) && ( mListBox->currentRow() == 0 ) ) {
    kDebug() << "Called while the _topmost_ filter is selected, ignoring.";
    return;
  }
  bool wasMoved = false;

  for ( int i = 0; i<numberOfItem; ++i ) {
    const int posItem = mListBox->row( listWidgetItem.at( i ) );
    if ( posItem == i ) {
      continue;
    }
    QListWidgetItem *item = mListBox->takeItem( posItem );
    mListBox->insertItem( posItem - 1, item );
    
    wasMoved = true;
  }
  
  if ( wasMoved ) {
    emit changed();
  }  
}

void SimpleStringListEditor::slotDown() {
  QList<QListWidgetItem*> listWidgetItem = selectedItems();
  if ( listWidgetItem.isEmpty() ) {
    return;
  }

  const int numberOfElement( mListBox->count() );
  const int numberOfItem( listWidgetItem.count() );
  if ( ( numberOfItem == 1 ) && ( mListBox->currentRow() == numberOfElement - 1 ) ) {
    kDebug() << "Called while the _last_ filter is selected, ignoring.";
    return;
  }

  int j = 0;
  bool wasMoved = false;
  for ( int i = numberOfItem-1; i>= 0; --i, j++ ) {
    const int posItem = mListBox->row( listWidgetItem.at( i ) );
    if ( posItem == ( numberOfElement-1 -j ) ) {
      continue;
    }
    QListWidgetItem *item = mListBox->takeItem( posItem );
    mListBox->insertItem( posItem + 1, item );
    wasMoved = true;
  }

  if ( wasMoved ) {
    emit changed();
  }
}

void SimpleStringListEditor::slotSelectionChanged() {

  QList<QListWidgetItem *> lstSelectedItems = mListBox->selectedItems();
  const int numberOfItemSelected( lstSelectedItems.count() );
  const bool uniqItemSelected = ( numberOfItemSelected == 1 );
  // if there is one, item will be non-null (ie. true), else 0
  // (ie. false):
  if ( mRemoveButton )
    mRemoveButton->setEnabled( !lstSelectedItems.isEmpty() );

  if ( mModifyButton )
    mModifyButton->setEnabled( uniqItemSelected );

  const int currentIndex = mListBox->currentRow();

  const bool aItemIsSelected = !lstSelectedItems.isEmpty();
  const bool allItemSelected = ( mListBox->count() == numberOfItemSelected );
  const bool theLast = ( currentIndex >= mListBox->count() - 1 );
  const bool theFirst = ( currentIndex == 0 );

  if ( mUpButton ) {
    mUpButton->setEnabled( aItemIsSelected && ( ( uniqItemSelected && !theFirst ) ||
                         ( !uniqItemSelected ) ) && !allItemSelected );
  }
  if ( mDownButton ) {
    mDownButton->setEnabled( aItemIsSelected &&
                            ( ( uniqItemSelected && !theLast ) ||
                            ( !uniqItemSelected ) ) && !allItemSelected );
  }
}



#include "simplestringlisteditor.moc"
