/*  -*- c++ -*-
    vacationdialog.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vacationdialog.h"

#include <kmime_header_parsing.h>
using KMime::Types::AddrSpecList;
using KMime::Types::AddressList;
using KMime::Types::MailboxList;
using KMime::HeaderParsing::parseAddressList;

#include <knuminput.h>
#include <klocale.h>
#include <kdebug.h>
#include <kwin.h>
#include <kapplication.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qvalidator.h>

namespace KMail {

  VacationDialog::VacationDialog( const QString & caption, QWidget * parent,
				  const char * name, bool modal )
    : KDialogBase( Plain, caption, Ok|Cancel|Default, Ok, parent, name, modal )
  {
    KWin::setIcons( winId(), kapp->icon(), kapp->miniIcon() );

    static const int rows = 7;
    int row = -1;

    QGridLayout * glay = new QGridLayout( plainPage(), rows, 2, 0, spacingHint() );
    glay->setColStretch( 1, 1 );

    // explanation label:
    ++row;
    glay->addMultiCellWidget( new QLabel( i18n("Configure vacation "
					       "notifications to be sent:"),
					  plainPage() ), row, row, 0, 1 );

    // Activate checkbox:
    ++row;
    mActiveCheck = new QCheckBox( i18n("&Activate vacation notifications"), plainPage() );
    glay->addMultiCellWidget( mActiveCheck, row, row, 0, 1 );

    // Message text edit:
    ++row;
    glay->setRowStretch( row, 1 );
    mTextEdit = new QTextEdit( plainPage(), "mTextEdit" );
    mTextEdit->setTextFormat( QTextEdit::PlainText );
    glay->addMultiCellWidget( mTextEdit, row, row, 0, 1 );

    // "Resent only after" spinbox and label:
    ++row;
    mIntervalSpin = new KIntSpinBox( 1, 356, 1, 7, 10, plainPage(), "mIntervalSpin" );
    connect(mIntervalSpin, SIGNAL( valueChanged( int )), SLOT( slotIntervalSpinChanged( int ) ) );
    glay->addWidget( new QLabel( mIntervalSpin, i18n("&Resend notification only after:"), plainPage() ), row, 0 );
    glay->addWidget( mIntervalSpin, row, 1 );

    // "Send responses for these addresses" lineedit and label:
    ++row;
    mMailAliasesEdit = new QLineEdit( plainPage(), "mMailAliasesEdit" );
    glay->addWidget( new QLabel( mMailAliasesEdit, i18n("&Send responses for these addresses:"), plainPage() ), row, 0 );
    glay->addWidget( mMailAliasesEdit, row, 1 );

    // "Send responses also to SPAM mail" checkbox:
    ++row;
    mSpamCheck = new QCheckBox( i18n("Do not send vacation replies to spam messages"), plainPage(), "mSpamCheck" );
    mSpamCheck->setChecked( true );
    glay->addMultiCellWidget( mSpamCheck, row, row, 0, 1 );

    //  domain checkbox and linedit:
    ++row;
    mDomainCheck = new QCheckBox( i18n("Only react to mail coming from domain"), plainPage(), "mDomainCheck" );
    mDomainCheck->setChecked( false );
    mDomainEdit = new QLineEdit( plainPage(), "mDomainEdit" );
    mDomainEdit->setEnabled( false );
    mDomainEdit->setValidator( new QRegExpValidator( QRegExp( "[a-zA-Z0-9+-]+(?:\\.[a-zA-Z0-9+-]+)*" ), mDomainEdit ) );
    glay->addWidget( mDomainCheck, row, 0 );
    glay->addWidget( mDomainEdit, row, 1 );
    connect( mDomainCheck, SIGNAL(toggled(bool)),
             mDomainEdit, SLOT(setEnabled(bool)) );

    Q_ASSERT( row == rows - 1 );
  }

  VacationDialog::~VacationDialog() {
    kdDebug(5006) << "~VacationDialog()" << endl;
  }

  bool VacationDialog::activateVacation() const {
    return mActiveCheck->isChecked();
  }

  void VacationDialog::setActivateVacation( bool activate ) {
    mActiveCheck->setChecked( activate );
  }

  QString VacationDialog::messageText() const {
    return mTextEdit->text().stripWhiteSpace();
  }

  void VacationDialog::setMessageText( const QString & text ) {
    mTextEdit->setText( text );
    const int height = ( mTextEdit->fontMetrics().lineSpacing() + 1 ) * 11;
    mTextEdit->setMinimumHeight( height );
  }

  int VacationDialog::notificationInterval() const {
    return mIntervalSpin->value();
  }

  void VacationDialog::setNotificationInterval( int days ) {
    mIntervalSpin->setValue( days );
  }

  AddrSpecList VacationDialog::mailAliases() const {
    QCString text = mMailAliasesEdit->text().latin1(); // ### IMAA: !ok
    AddressList al;
    const char * s = text.begin();
    parseAddressList( s, text.end(), al );

    AddrSpecList asl;
    for ( AddressList::const_iterator it = al.begin() ; it != al.end() ; ++it ) {
      const MailboxList & mbl = (*it).mailboxList;
      for ( MailboxList::const_iterator jt = mbl.begin() ; jt != mbl.end() ; ++jt )
	asl.push_back( (*jt).addrSpec );
    }
    return asl;
  }

  void VacationDialog::setMailAliases( const AddrSpecList & aliases ) {
    QStringList sl;
    for ( AddrSpecList::const_iterator it = aliases.begin() ; it != aliases.end() ; ++it )
      sl.push_back( (*it).asString() );
    mMailAliasesEdit->setText( sl.join(", ") );
  }

  void VacationDialog::setMailAliases( const QString & aliases ) {
    mMailAliasesEdit->setText( aliases );
  }

  void VacationDialog::slotIntervalSpinChanged ( int value ) {
    mIntervalSpin->setSuffix( i18n(" day", " days", value) );
  }

  QString VacationDialog::domainName() const {
    return mDomainCheck->isChecked() ? mDomainEdit->text() : QString::null ;
  }

  void VacationDialog::setDomainName( const QString & domain ) {
    mDomainEdit->setText( domain );
    if ( !domain.isEmpty() )
      mDomainCheck->setChecked( true );
  }

  bool VacationDialog::sendForSpam() const {
    return !mSpamCheck->isChecked();
  }

  void VacationDialog::setSendForSpam( bool enable ) {
    mSpamCheck->setChecked( !enable );
  }


  /* virtual*/
  void KMail::VacationDialog::enableDomainAndSendForSpam( bool enable ) {
      mDomainCheck->setEnabled( enable );
      mDomainEdit->setEnabled( enable );
      mSpamCheck->setEnabled( enable );
  }


} // namespace KMail

#include "vacationdialog.moc"
