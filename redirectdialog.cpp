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


#include "redirectdialog.h"

#include "kmkernel.h"

#include <akonadi/contact/emailaddressselectiondialog.h>
#include <kpimutils/email.h>
#include "messageviewer/autoqpointer.h"
#include <messagecomposer/composerlineedit.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kvbox.h>

#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QFrame>
#include <QTreeView>

using namespace KMail;

RedirectDialog::RedirectDialog( QWidget *parent, bool immediate )
  : KDialog( parent )
{
  setCaption( i18n( "Redirect Message" ) );
  setButtons( User1|User2|Cancel );
  setDefaultButton( immediate ? User1 : User2 );
  QFrame *vbox = new KVBox( this );
  setMainWidget( vbox );
  mLabelTo = new QLabel( i18n( "Select the recipient &addresses "
                               "to redirect to:" ), vbox );

  KHBox *hbox = new KHBox( vbox );
  hbox->setSpacing(4);
  mEditTo = new MessageComposer::ComposerLineEdit( true, hbox );
  mEditTo->setObjectName( "toLine" );
  mEditTo->setRecentAddressConfig( KMKernel::config().data() );
  mEditTo->setMinimumWidth( 300 );

  mBtnTo = new QPushButton( QString(), hbox );
  mBtnTo->setObjectName( "toBtn" );
  mBtnTo->setIcon( KIcon( "help-contents" ) );
  mBtnTo->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtnTo->setMinimumSize( mBtnTo->sizeHint() * 1.2 );
  mBtnTo->setToolTip( i18n("Use the Address-Selection Dialog") );
  mBtnTo->setWhatsThis( i18n("This button opens a separate dialog "
                                 "where you can select recipients out "
                                 "of all available addresses." ) );

  connect( mBtnTo, SIGNAL(clicked()), SLOT(slotAddrBook()) );

  connect( mEditTo, SIGNAL( textChanged ( const QString & ) ), SLOT( slotEmailChanged( const QString & ) ) );
  mLabelTo->setBuddy( mBtnTo );
  mEditTo->setFocus();

  setButtonGuiItem( User1, KGuiItem( i18n("&Send Now"), "mail-send" ) );
  setButtonGuiItem( User2, KGuiItem( i18n("Send &Later"), "mail-queue" ) );
  connect(this,SIGNAL(user1Clicked()),this, SLOT(slotUser1()));
  connect(this,SIGNAL(user2Clicked()),this, SLOT(slotUser2()));
  enableButton( User1, false );
  enableButton( User2, false );
}


void RedirectDialog::slotEmailChanged( const QString & text )
{
  enableButton( User1, !text.isEmpty() );
  enableButton( User2, !text.isEmpty() );
}

//-----------------------------------------------------------------------------
void RedirectDialog::slotUser1()
{
  mImmediate = true;
  accept();
}

//-----------------------------------------------------------------------------
void RedirectDialog::slotUser2()
{
  mImmediate = false;
  accept();
}

//-----------------------------------------------------------------------------
void RedirectDialog::accept()
{
  mResentTo = mEditTo->text();
  if ( mResentTo.isEmpty() ) {
    KMessageBox::sorry( this,
        i18n("You cannot redirect the message without an address."),
        i18n("Empty Redirection Address") );
  }
  else done( Ok );
}


//-----------------------------------------------------------------------------
void RedirectDialog::slotAddrBook()
{
  MessageViewer::AutoQPointer<Akonadi::EmailAddressSelectionDialog> dlg( new Akonadi::EmailAddressSelectionDialog( this ) );
  dlg->view()->view()->setSelectionMode( QAbstractItemView::MultiSelection );

  mResentTo = mEditTo->text();

  if ( dlg->exec() != KDialog::Rejected && dlg ) {
    QStringList addresses;
    foreach ( const Akonadi::EmailAddressSelectionView::Selection &selection, dlg->selectedAddresses() )
      addresses << selection.quotedEmail();

    if ( !mResentTo.isEmpty() )
      addresses.prepend( mResentTo );

    mEditTo->setText( addresses.join( ", " ) );
    mEditTo->setModified( true );
  }
}


#include "redirectdialog.moc"
