/*
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#include "redirectdialog.h"

#include "kmkernel.h"

#include <libemailfunctions/email.h>
#include <addressesdialog.h>
using KPIM::AddressesDialog;
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;

#include <klocale.h>
#include <kmessagebox.h>

#include <qvbox.h>
#include <qhbox.h>


using namespace KMail;

RedirectDialog::RedirectDialog( QWidget *parent, const char *name, bool modal )
  : KDialogBase( parent, name, modal, i18n( "Redirect the Message" ),
                 Ok|Cancel, Ok )
{
  QVBox *vbox = makeVBoxMainWidget();
  mLabelTo = new QLabel( i18n( "Recipient addresses to redirect to:" ), vbox );
  
  QHBox *hbox = new QHBox( vbox );
  mEditTo = new KMLineEdit( 0, true, hbox, "toLine" );

  mBtnTo = new QPushButton( "...", hbox, "toBtn" );
  mLabelTo->setBuddy( mBtnTo );
/*
  mBtnTo->setIconType( KIcon::NoGroup, KIcon::Any, true );
  mBtnTo->setIconSize( 16 );
  mBtnTo->setIcon( "gear" );
*/  
  mBtnTo->setEnabled( true );
  
  //enableButtonOK( false );
  
/*
  connect(mEditTo,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
*/
  connect( mBtnTo, SIGNAL(clicked()), SLOT(slotAddrBook()) );
}


//-----------------------------------------------------------------------------
QString RedirectDialog::to()
{
  return resentTo;
}


//-----------------------------------------------------------------------------
void RedirectDialog::accept()
{
  resentTo = mEditTo->text();
  if ( resentTo.isEmpty() ) {
    KMessageBox::sorry( this, 
        i18n("You cannot redirect the message without an address."), 
        i18n("Empty Redirection Address") );
  }
  else done( Ok );
}


//-----------------------------------------------------------------------------
void RedirectDialog::slotAddrBook()
{
  AddressesDialog dlg( this );

  resentTo = mEditTo->text();
  if ( !resentTo.isEmpty() ) {
      QStringList lst = KPIM::splitEmailAddrList( resentTo );
      dlg.setSelectedTo( lst );
  }

  dlg.setRecentAddresses( 
      RecentAddresses::self( KMKernel::config() )->kabcAddresses() );

  // FIXME We have to make changes to the address dialog so that
  // it's impossible to specify Cc or Bcc addresses as we support
  // only the Redirect-To header!
  if (dlg.exec()==QDialog::Rejected) return;

  mEditTo->setText( dlg.to().join(", ") );
  mEditTo->setEdited( true );
}


#include "redirectdialog.moc"
