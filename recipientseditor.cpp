/*
    This file is part of KMail.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    
    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "recipientseditor.h"

#include "recipientspicker.h"
#include "kwindowpositioner.h"

#include <kdebug.h>
#include <kinputdialog.h>
#include <klocale.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qscrollview.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qhbox.h>
#include <qtimer.h>
#include <qpushbutton.h>

Recipient::Recipient( const QString &email, Recipient::Type type )
  : mEmail( email ), mType( type )
{
}

void Recipient::setType( Type type )
{
  mType = type;
}

Recipient::Type Recipient::type() const
{
  return mType;
}

void Recipient::setEmail( const QString &email )
{
  mEmail = email;
}

QString Recipient::email() const
{
  return mEmail;
}

bool Recipient::isEmpty() const
{
  return mEmail.isEmpty();
}

int Recipient::typeToId( Recipient::Type type )
{
  return static_cast<int>( type );
}

Recipient::Type Recipient::idToType( int id )
{
  return static_cast<Type>( id );
}

QString Recipient::typeLabel() const
{
  return typeLabel( mType );
}

QString Recipient::typeLabel( Recipient::Type type )
{
  switch( type ) {
    case To:
      return i18n("To");
    case Cc:
      return i18n("CC");
    case Bcc:
      return i18n("BCC");
    case ReplyTo:
      return i18n("Reply To");
  }

  return i18n("<Undefined RecipientType>");
}

QStringList Recipient::allTypeLabels()
{
  QStringList types;
  types.append( typeLabel( To ) );
  types.append( typeLabel( Cc ) );
  types.append( typeLabel( Bcc ) );
  types.append( typeLabel( ReplyTo ) );
  return types;
}


RecipientLine::RecipientLine( QWidget *parent )
  : QWidget( parent )
{
  QBoxLayout *topLayout = new QHBoxLayout( this );
  
  QStringList recipientTypes = Recipient::allTypeLabels();

  mCombo = new QComboBox( this );
  mCombo->insertStringList( recipientTypes );
  topLayout->addWidget( mCombo );

  mEdit = new QLineEdit( this );
  topLayout->addWidget( mEdit );
  connect( mEdit, SIGNAL( returnPressed() ), SLOT( slotReturnPressed() ) );

  kdDebug() << "HEIGHT: " << mEdit->minimumSizeHint().height() << endl;

  mCombo->setFixedHeight( mEdit->minimumSizeHint().height() );
}

void RecipientLine::setRecipient( const Recipient &rec )
{
  mEdit->setText( rec.email() );
  mCombo->setCurrentItem( Recipient::typeToId( rec.type() ) );
}

void RecipientLine::setRecipient( const QString &email )
{
  setRecipient( Recipient( email ) );
}

Recipient RecipientLine::recipient() const
{
  return Recipient( mEdit->text(),
    Recipient::idToType( mCombo->currentItem() ) );
}

void RecipientLine::activate()
{
  mEdit->setFocus();
}

void RecipientLine::slotReturnPressed()
{
  emit returnPressed( this );
}

void RecipientLine::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Key_Up ) {
    emit upPressed( this );
  } else if ( ev->key() == Key_Down ) {
    emit downPressed( this );
  }
}


RecipientsView::RecipientsView( QWidget *parent )
  : QScrollView( parent )
{
  addLine();
}

RecipientLine *RecipientsView::activeLine()
{
  return mLines.last();
}

RecipientLine *RecipientsView::addLine()
{
  kdDebug() << "RecipientsView::addLine()" << endl;

  RecipientLine *line = new RecipientLine( viewport() );
  addChild( line, 0, mLines.count() * mLineHeight );
  line->show();
  connect( line, SIGNAL( returnPressed( RecipientLine * ) ),
    SLOT( slotReturnPressed( RecipientLine * ) ) );
  connect( line, SIGNAL( upPressed( RecipientLine * ) ),
    SLOT( slotUpPressed( RecipientLine * ) ) );
  connect( line, SIGNAL( downPressed( RecipientLine * ) ),
    SLOT( slotDownPressed( RecipientLine * ) ) );

  mLines.append( line );

  mLineHeight = line->minimumSizeHint().height();

  line->resize( viewport()->width(), mLineHeight );
  ensureVisible( 0, mLines.count() * mLineHeight );

  resizeContents( viewport()->width(), mLines.count() * mLineHeight );

  if ( mLines.count() < 6 ) {
    setFixedHeight( mLineHeight * mLines.count() + 2 );
  }

  emit totalChanged( mLines.count() );

  return line;
}

void RecipientsView::slotReturnPressed( RecipientLine *line )
{
  if ( !line->recipient().isEmpty() ) {
    addLine()->activate();
  }
}

void RecipientsView::slotDownPressed( RecipientLine *line )
{
  int pos = mLines.find( line );
  if ( pos >= 0 && pos < (int)mLines.count() - 1 ) {
    mLines.at( pos + 1 )->activate();
  }
}

void RecipientsView::slotUpPressed( RecipientLine *line )
{
  int pos = mLines.find( line );
  if ( pos > 0 ) {
    mLines.at( pos - 1 )->activate();
  }
}

void RecipientsView::viewportResizeEvent ( QResizeEvent *ev )
{
  kdDebug() << "RESIZE " << ev->size() << endl;

  for( uint i = 0; i < mLines.count(); ++i ) {
    mLines.at( i )->resize( ev->size().width(), mLineHeight );
  }
}

QSize RecipientsView::sizeHint() const
{
  return QSize( 200, mLineHeight * mLines.count() );
}

QSize RecipientsView::minimumSizeHint() const
{
  int height;
  
  uint numLines = 5;
  
  if ( mLines.count() < numLines ) height = mLineHeight * mLines.count();
  else height = mLineHeight * numLines;

  return QSize( 200, height );
}

Recipient::List RecipientsView::recipients() const
{
  Recipient::List recipients;

  QPtrListIterator<RecipientLine> it( mLines );
  RecipientLine *line;
  while( ( line = it.current() ) ) {
    if ( !line->recipient().isEmpty() ) {
      recipients.append( line->recipient() );
    }

    ++it;
  }
  
  return recipients;
}


SideWidget::SideWidget( RecipientsView *view, QWidget *parent )
  : QWidget( parent ), mView( view ), mRecipientPicker( 0 )
{
  QBoxLayout *topLayout = new QVBoxLayout( this );

  mTotalLabel = new QLabel( this );
  mTotalLabel->setAlignment( AlignCenter );
  topLayout->addWidget( mTotalLabel, 1 );
  mTotalLabel->hide();

  QPushButton *button = new QPushButton( "...", this );
  topLayout->addWidget( button );
  connect( button, SIGNAL( clicked() ), SLOT( pickRecipient() ) );

  initRecipientPicker();
}

void SideWidget::initRecipientPicker()
{
  if ( mRecipientPicker ) return;

  mRecipientPicker = new RecipientsPicker( 0 );
  connect( mRecipientPicker, SIGNAL( pickedRecipient( const QString & ) ),
    SIGNAL( pickedRecipient( const QString & ) ) );

  new KWindowPositioner( this, mRecipientPicker );
}

void SideWidget::setTotal( int total )
{
  mTotalLabel->setText( QString::number( total ) );
  if ( total > 1 ) mTotalLabel->show();
  else mTotalLabel->hide();
}

void SideWidget::pickRecipient()
{
#if 0
  QString rec = KInputDialog::getText( "Pick Recipient",
    "Email address of recipient" );
  if ( !rec.isEmpty() ) emit pickedRecipient( rec );
#else
  mRecipientPicker->setRecipients( mView->recipients() );
  mRecipientPicker->show();
  mRecipientPicker->raise();
#endif
}


RecipientsEditor::RecipientsEditor( QWidget *parent )
  : QWidget( parent )
{
  QBoxLayout *topLayout = new QHBoxLayout( this );

  mRecipientsView = new RecipientsView( this );
  topLayout->addWidget( mRecipientsView );

  SideWidget *side = new SideWidget( mRecipientsView, this );
  topLayout->addWidget( side );
  connect( side, SIGNAL( pickedRecipient( const QString & ) ),
    SLOT( slotPickedRecipient( const QString & ) ) );

  connect( mRecipientsView, SIGNAL( totalChanged( int ) ),
    side, SLOT( setTotal( int ) ) );
}

void RecipientsEditor::slotPickedRecipient( const QString &rec )
{
  RecipientLine *line = mRecipientsView->activeLine();
  line->setRecipient( rec );
  
  mRecipientsView->addLine()->activate();
}

Recipient::List RecipientsEditor::recipients() const
{
  return mRecipientsView->recipients();
}

void RecipientsEditor::setRecipientString( const QString &str,
  Recipient::Type type )
{
  clear();

  QStringList r = QStringList::split( ",", str );
  QStringList::ConstIterator it;
  for( it = r.begin(); it != r.end(); ++it ) {
    RecipientLine *line = mRecipientsView->addLine();
    line->setRecipient( Recipient( *it, type ) );
  }
}

QString RecipientsEditor::recipientString( Recipient::Type type )
{
  kdDebug() << "recipientString() " << Recipient::typeLabel( type ) << endl;

  QString str;

  Recipient::List recipients = mRecipientsView->recipients();
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    if ( (*it).type() == type ) {
      if ( !str.isEmpty() ) str += ", ";
      str.append( (*it).email() );
    }
  }
  
  return str;
}

void RecipientsEditor::clear()
{
}

#include "recipientseditor.moc"
