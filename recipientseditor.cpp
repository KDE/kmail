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
#include "distributionlistdialog.h"

#include <libemailfunctions/email.h>

#include <kdebug.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kiconloader.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qscrollview.h>
#include <qcombobox.h>
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
    case Undefined:
      break;
  }

  return i18n("<Undefined RecipientType>");
}

QStringList Recipient::allTypeLabels()
{
  QStringList types;
  types.append( typeLabel( To ) );
  types.append( typeLabel( Cc ) );
  types.append( typeLabel( Bcc ) );
  return types;
}


RecipientComboBox::RecipientComboBox( QWidget *parent )
  : QComboBox( parent )
{
}

void RecipientComboBox::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Key_Right ) emit rightPressed();
  else QComboBox::keyPressEvent( ev );
}


void RecipientLineEdit::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Key_Backspace  &&  text().isEmpty() ) {
    ev->accept();
    emit deleteMe();
  } else if ( ev->key() == Key_Left && cursorPosition() == 0 ) {
    emit leftPressed();
  } else if ( ev->key() == Key_Right && cursorPosition() == (int)text().length() ) {
    emit rightPressed();
  } else {
    KMLineEdit::keyPressEvent( ev );
  }
}

RecipientLine::RecipientLine( QWidget *parent )
  : QWidget( parent ), mRecipientsCount( 0 )
{
  QBoxLayout *topLayout = new QHBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );

  QStringList recipientTypes = Recipient::allTypeLabels();

  mCombo = new RecipientComboBox( this );
  mCombo->insertStringList( recipientTypes );
  topLayout->addWidget( mCombo );

  mEdit = new RecipientLineEdit( this );
  topLayout->addWidget( mEdit );
  connect( mEdit, SIGNAL( returnPressed() ), SLOT( slotReturnPressed() ) );
  connect( mEdit, SIGNAL( deleteMe() ), SLOT( slotPropagateDeletion() ) );
  connect( mEdit, SIGNAL( textChanged( const QString & ) ),
    SLOT( analyzeLine( const QString & ) ) );
  connect( mEdit, SIGNAL( focusUp() ), SLOT( slotFocusUp() ) );
  connect( mEdit, SIGNAL( focusDown() ), SLOT( slotFocusDown() ) );
  connect( mEdit, SIGNAL( rightPressed() ), SIGNAL( rightPressed() ) );

  connect( mEdit, SIGNAL( leftPressed() ), mCombo, SLOT( setFocus() ) );
  connect( mCombo, SIGNAL( rightPressed() ), mEdit, SLOT( setFocus() ) );

  int height = mEdit->minimumSizeHint().height();

  kdDebug() << "HEIGHT: " << height << endl;

  mRemoveButton = new QPushButton( this );
  mRemoveButton->setPixmap( SmallIcon( "clear_left" ) );
  topLayout->addWidget( mRemoveButton );
  connect( mRemoveButton, SIGNAL( clicked() ), SLOT( slotPropagateDeletion() ) );

  mCombo->setFixedHeight( height );
  mRemoveButton->setFixedHeight( height );
}

void RecipientLine::slotFocusUp()
{
  emit upPressed( this );
}

void RecipientLine::slotFocusDown()
{
  emit downPressed( this );
}

void RecipientLine::analyzeLine( const QString &text )
{
  QStringList r = KPIM::splitEmailAddrList( text );
  if ( int( r.count() ) != mRecipientsCount ) {
    mRecipientsCount = r.count();
    emit countChanged();
  }
}

int RecipientLine::recipientsCount()
{
  return mRecipientsCount;
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

void RecipientLine::setRecipientType( Recipient::Type type )
{
  mCombo->setCurrentItem( Recipient::typeToId( type ) );
}

Recipient::Type RecipientLine::recipientType() const
{
  return Recipient::idToType( mCombo->currentItem() );
}

void RecipientLine::activate()
{
  mEdit->setFocus();
}

bool RecipientLine::isActive()
{
  return mEdit->hasFocus();
}

bool RecipientLine::isEmpty()
{
  return mEdit->text().isEmpty();
}

void RecipientLine::slotReturnPressed()
{
  emit returnPressed( this );
}

void RecipientLine::slotPropagateDeletion()
{
  emit deleteLine( this );
}

void RecipientLine::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Key_Up ) {
    emit upPressed( this );
  } else if ( ev->key() == Key_Down ) {
    emit downPressed( this );
  }
}

void RecipientLine::setComboWidth( int w )
{
  mCombo->setFixedWidth( w );
}

void RecipientLine::fixTabOrder( QWidget *previous )
{
  setTabOrder( previous, mCombo );
  setTabOrder( mCombo, mEdit );
  setTabOrder( mEdit, mRemoveButton );
}

QWidget *RecipientLine::tabOut() const
{
  return mRemoveButton;
}

void RecipientLine::clear()
{
  mEdit->clear();
}

RecipientsView::RecipientsView( QWidget *parent )
  : QScrollView( parent )
{
  setHScrollBarMode( AlwaysOff );
  setLineWidth( 0 );

  addLine();
}

RecipientLine *RecipientsView::activeLine()
{
  return mLines.last();
}

RecipientLine *RecipientsView::emptyLine()
{
  RecipientLine *line;
  for( line = mLines.first(); line; line = mLines.next() ) {
    if ( line->isEmpty() ) return line;
  }

  return 0;
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
  connect( line, SIGNAL( rightPressed() ), SIGNAL( focusRight() ) );
  connect( line, SIGNAL( deleteLine( RecipientLine * ) ),
    SLOT( slotDecideLineDeletion( RecipientLine * ) ) );
  connect( line, SIGNAL( countChanged() ), SLOT( calculateTotal() ) );

  if ( mLines.last() ) {
    line->setRecipientType( mLines.last()->recipientType() );
    line->fixTabOrder( mLines.last()->tabOut() );
  }

  mLines.append( line );

  line->setComboWidth( mFirstColumnWidth );

  mLineHeight = line->minimumSizeHint().height();

  line->resize( viewport()->width(), mLineHeight );

  resizeView();

  calculateTotal();

  ensureVisible( 0, mLines.count() * mLineHeight );

  return line;
}

void RecipientsView::calculateTotal()
{
  int count = 0;
  int empty = 0;

  RecipientLine *line;
  for( line = mLines.first(); line; line = mLines.next() ) {
    if ( line->isEmpty() ) ++empty;
    else count += line->recipientsCount();
  }

  if ( empty == 0 ) addLine();
  
  emit totalChanged( count, mLines.count() );
}

void RecipientsView::slotReturnPressed( RecipientLine *line )
{
  if ( !line->recipient().isEmpty() ) {
    RecipientLine *empty = emptyLine();
    if ( !empty ) empty = addLine();
    activateLine( empty );
  }
}

void RecipientsView::slotDownPressed( RecipientLine *line )
{
  int pos = mLines.find( line );
  if ( pos >= (int)mLines.count() - 1 ) {
    emit focusDown();
  } else if ( pos >= 0 ) {
    activateLine( mLines.at( pos + 1 ) );
  }
}

void RecipientsView::slotUpPressed( RecipientLine *line )
{
  int pos = mLines.find( line );
  if ( pos > 0 ) {
    activateLine( mLines.at( pos - 1 ) );
  } else {
    emit focusUp();
  }
}

void RecipientsView::slotDecideLineDeletion( RecipientLine *line )
{
  if ( mLines.count() == 1 ) {
    line->clear();
  } else {
    mCurDelLine = line;
    QTimer::singleShot( 0, this, SLOT( slotDeleteLine( ) ) );
  }
}

void RecipientsView::slotDeleteLine()
{
  RecipientLine *line = mCurDelLine;
  int pos = mLines.find( line );

  int newPos;
  if ( pos == 0 ) newPos = pos + 1;
  else newPos = pos - 1;
  mLines.at( newPos )->activate();

  mLines.remove( line );
  removeChild( line );
  delete line;

  for( uint i = pos; i < mLines.count(); ++i ) {
    RecipientLine *line = mLines.at( i );
    moveChild( line, childX( line ), childY( line ) - mLineHeight );
  }
  
  calculateTotal();
  
  resizeView();
}

void RecipientsView::resizeView()
{
  resizeContents( viewport()->width(), mLines.count() * mLineHeight );

  if ( mLines.count() < 6 ) {
    setFixedHeight( mLineHeight * mLines.count() );
  }
}

void RecipientsView::activateLine( RecipientLine *line )
{
  line->activate();
  ensureVisible( 0, childY( line ) );
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

void RecipientsView::setFocus()
{
  if ( mLines.last()->isActive() ) setFocusBottom();
  else setFocusTop();
}

void RecipientsView::setFocusTop()
{
  RecipientLine *line = mLines.first();
  if ( line ) line->activate();
  else kdWarning() << "No first" << endl;
}

void RecipientsView::setFocusBottom()
{
  RecipientLine *line = mLines.last();
  if ( line ) line->activate();
  else  kdWarning() << "No last" << endl;
}

void RecipientsView::setFirstColumnWidth( int w )
{
  mFirstColumnWidth = w;

  QPtrListIterator<RecipientLine> it( mLines );
  RecipientLine *line;
  while( ( line = it.current() ) ) {
    line->setComboWidth( mFirstColumnWidth );
    ++it;
  }

  resizeView();
}


RecipientsToolTip::RecipientsToolTip( RecipientsView *view, QWidget *parent )
  : QToolTip( parent ), mView( view )
{
}

QString RecipientsToolTip::line( const Recipient &r )
{
  QString txt = r.email();

  return "&nbsp;&nbsp;" + QStyleSheet::escape( txt ) + "<br/>";
}

void RecipientsToolTip::maybeTip( const QPoint & p )
{
  QString text = "<qt>";

  QString to;
  QString cc;
  QString bcc;

  Recipient::List recipients = mView->recipients();
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    switch( (*it).type() ) {
      case Recipient::To:
        to += line( *it );
        break;
      case Recipient::Cc:
        cc += line( *it );
        break;
      case Recipient::Bcc:
        bcc += line( *it );
        break;
      default:
        break;
    }
  }

  text += i18n("<b>To:</b><br/>") + to;
  if ( !cc.isEmpty() ) text += i18n("<b>CC:</b><br/>") + cc;
  if ( !bcc.isEmpty() ) text += i18n("<b>BCC:</b><br/>") + bcc;

  text.append( "</qt>" );

  QRect geometry( p + QPoint( 2, 2 ), QPoint( 400, 100 ) );

  tip( QRect( p.x() - 20, p.y() - 20, 40, 40 ), text, geometry );
}


SideWidget::SideWidget( RecipientsView *view, QWidget *parent )
  : QWidget( parent ), mView( view ), mRecipientPicker( 0 )
{
  QBoxLayout *topLayout = new QVBoxLayout( this );

  topLayout->addStretch( 1 );

  mTotalLabel = new QLabel( this );
  mTotalLabel->setAlignment( AlignCenter );
  topLayout->addWidget( mTotalLabel );
  mTotalLabel->hide();

  topLayout->addStretch( 1 );

  new RecipientsToolTip( view, mTotalLabel );

  mDistributionListButton = new QPushButton( "Save List...", this );
  topLayout->addWidget( mDistributionListButton );
  mDistributionListButton->hide();
  connect( mDistributionListButton, SIGNAL( clicked() ),
    SIGNAL( saveDistributionList() ) );

  mSelectButton = new QPushButton( "Se&lect...", this );
  topLayout->addWidget( mSelectButton );
  connect( mSelectButton, SIGNAL( clicked() ), SLOT( pickRecipient() ) );

  initRecipientPicker();
}

SideWidget::~SideWidget()
{
}

RecipientsPicker* SideWidget::picker() const
{
  return mRecipientPicker;
}

void SideWidget::setFocus()
{
  mSelectButton->setFocus();
}

void SideWidget::initRecipientPicker()
{
  if ( mRecipientPicker ) return;

  mRecipientPicker = new RecipientsPicker( this );
  connect( mRecipientPicker, SIGNAL( pickedRecipient( const Recipient & ) ),
    SIGNAL( pickedRecipient( const Recipient & ) ) );

  mPickerPositioner = new KWindowPositioner( this, mRecipientPicker );
}

void SideWidget::setTotal( int recipients, int lines )
{
#if 0
  kdDebug() << "SideWidget::setTotal() recipients: " << recipients <<
    "  lines: " << lines << endl;
#endif

  QString labelText;
  if ( recipients == 0 ) labelText = i18n("No recipients");
  else labelText = i18n("1 recipient","%n recipients", recipients );
  mTotalLabel->setText( labelText );

  if ( lines > 3 ) mTotalLabel->show();
  else mTotalLabel->hide();

  if ( lines > 2 ) mDistributionListButton->show();
  else mDistributionListButton->hide();
}

void SideWidget::pickRecipient()
{
#if 0
  QString rec = KInputDialog::getText( "Pick Recipient",
    "Email address of recipient" );
  if ( !rec.isEmpty() ) emit pickedRecipient( rec );
#else
  mRecipientPicker->setDefaultType( mView->activeLine()->recipientType() );
  mRecipientPicker->setRecipients( mView->recipients() );
  mRecipientPicker->show();
  mPickerPositioner->reposition();
  mRecipientPicker->raise();
#endif
}


RecipientsEditor::RecipientsEditor( QWidget *parent )
  : QWidget( parent )
{
  QBoxLayout *topLayout = new QHBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );

  mRecipientsView = new RecipientsView( this );
  topLayout->addWidget( mRecipientsView );
  connect( mRecipientsView, SIGNAL( focusUp() ), SIGNAL( focusUp() ) );
  connect( mRecipientsView, SIGNAL( focusDown() ), SIGNAL( focusDown() ) );

  mSideWidget = new SideWidget( mRecipientsView, this );
  topLayout->addWidget( mSideWidget );
  connect( mSideWidget, SIGNAL( pickedRecipient( const Recipient & ) ),
    SLOT( slotPickedRecipient( const Recipient & ) ) );
  connect( mSideWidget, SIGNAL( saveDistributionList() ),
    SLOT( saveDistributionList() ) );

  connect( mRecipientsView, SIGNAL( totalChanged( int, int ) ),
    mSideWidget, SLOT( setTotal( int, int ) ) );
  connect( mRecipientsView, SIGNAL( focusRight() ),
    mSideWidget, SLOT( setFocus() ) );
}

RecipientsEditor::~RecipientsEditor()
{
}

RecipientsPicker* RecipientsEditor::picker() const
{
  return mSideWidget->picker();
}

void RecipientsEditor::slotPickedRecipient( const Recipient &rec )
{
  RecipientLine *line = mRecipientsView->activeLine();
  if ( !line->isEmpty() ) line = mRecipientsView->addLine();

  Recipient r = rec;
  if ( r.type() == Recipient::Undefined ) {
    r.setType( line->recipientType() );
  }

  line->setRecipient( r );
}

void RecipientsEditor::saveDistributionList()
{
  DistributionListDialog *dlg = new DistributionListDialog( this );
  dlg->setRecipients( mRecipientsView->recipients() );
  dlg->show();
}

Recipient::List RecipientsEditor::recipients() const
{
  return mRecipientsView->recipients();
}

void RecipientsEditor::setRecipientString( const QString &str,
  Recipient::Type type )
{
  clear();

  QStringList r = KPIM::splitEmailAddrList( str );
  QStringList::ConstIterator it;
  for( it = r.begin(); it != r.end(); ++it ) {
    RecipientLine *line = mRecipientsView->emptyLine();
    if ( !line ) line = mRecipientsView->addLine();
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

void RecipientsEditor::setFocus()
{
  mRecipientsView->setFocus();
}

void RecipientsEditor::setFocusTop()
{
  mRecipientsView->setFocusTop();
}

void RecipientsEditor::setFocusBottom()
{
  mRecipientsView->setFocusBottom();
}

void RecipientsEditor::setFirstColumnWidth( int w )
{
  mRecipientsView->setFirstColumnWidth( w );
}

void RecipientsEditor::selectRecipients()
{
  mSideWidget->pickRecipient();
}

#include "recipientseditor.moc"
