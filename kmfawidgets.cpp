// kmfawidgets.h - KMFilterAction parameter widgets
// Copyright: (c) 2001 Marc Mutz <mutz@kde.org>
// License: GNU Genaral Public License

#include "kmfawidgets.h"
#include "kmaddrbookdlg.h" // for the button in KMFilterActionWithAddress
#include "kmkernel.h"

#include <kiconloader.h>
//#include <klocale.h>

#include <qpushbutton.h>

//=============================================================================
//
// class KMFilterActionWithAddressWidget
//
//=============================================================================

KMFilterActionWithAddressWidget::KMFilterActionWithAddressWidget( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  QHBoxLayout *hbl = new QHBoxLayout(this);
  hbl->setSpacing(4);
  mLineEdit = new QLineEdit(this);
  hbl->addWidget( mLineEdit, 1 /*stretch*/ );
  mBtn = new QPushButton( QString::null ,this );
  mBtn->setPixmap( BarIcon( "contents", KIcon::SizeSmall ) );
  mBtn->setFixedHeight( mLineEdit->sizeHint().height() );
  hbl->addWidget( mBtn );

  connect( mBtn, SIGNAL(clicked()),
	   this, SLOT(slotAddrBook()) );
}

void KMFilterActionWithAddressWidget::slotAddrBook()
{
  KMAddrBookSelDlg dlg( kernel->addrBook() );
  QString txt;

  if ( dlg.exec() == QDialog::Rejected ) return;

  txt = mLineEdit->text().stripWhiteSpace();

  if ( !txt.isEmpty() ) {
    if ( txt.right(1)[0] != ',' )
      txt += ", ";
    else
      txt += ' ';
  }

  mLineEdit->setText( txt + dlg.address() );
}

//--------------------------------------------
#include "kmfawidgets.moc"
