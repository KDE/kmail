// kmfawidgets.h - KMFilterAction parameter widgets
// Copyright: (c) 2001 Marc Mutz <mutz@kde.org>
// License: GNU Genaral Public License

#include "kmfawidgets.h"
#include "kmkernel.h"

#include <kabc/addresseedialog.h> // for the button in KMFilterActionWithAddress
#include <kiconloader.h>
//#include <klocale.h>

#include <qlayout.h>
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
  mLineEdit = new KLineEdit(this);
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
  QString txt;

  KABC::Addressee::List lst = KABC::AddresseeDialog::getAddressees( this );

  if ( lst.empty() )
    return;

  QStringList addrList;

  for( KABC::Addressee::List::iterator itr = lst.begin(); itr != lst.end(); ++itr ) {
    addrList << (*itr).fullEmail();
  }

  txt = mLineEdit->text().stripWhiteSpace();

  if ( !txt.isEmpty() ) {
    if ( txt.right(1)[0] != ',' )
      txt += ", ";
    else
      txt += ' ';
  }

  mLineEdit->setText( txt + addrList.join(",") );
}

//--------------------------------------------
#include "kmfawidgets.moc"
