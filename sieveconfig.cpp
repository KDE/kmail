/*  -*- c++ -*-
    sieveconfig.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sieveconfig.h"

#include <knuminput.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfigbase.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <klineedit.h>


namespace KMail {

  void SieveConfig::readConfig( const KConfigBase & config ) {
    mManagesieveSupported = config.readBoolEntry( "sieve-support", false );
    mReuseConfig = config.readBoolEntry( "sieve-reuse-config", true );

    int port = config.readNumEntry( "sieve-port", 2000 );
    if ( port < 1 || port > USHRT_MAX ) port = 2000;
    mPort = static_cast<unsigned short>( port );

    mAlternateURL = config.readEntry( "sieve-alternate-url" );
    mVacationFileName = config.readEntry( "sieve-vacation-filename", "kmail-vacation.siv" );
  }

  void SieveConfig::writeConfig( KConfigBase & config ) const {
    config.writeEntry( "sieve-support", managesieveSupported() );
    config.writeEntry( "sieve-reuse-config", reuseConfig() );
    config.writeEntry( "sieve-port", port() );
    config.writeEntry( "sieve-alternate-url", mAlternateURL.url() );
    config.writeEntry( "sieve-vacation-filename", mVacationFileName );
  }


  SieveConfigEditor::SieveConfigEditor( QWidget * parent, const char * name )
    : QWidget( parent, name )
  {
    // tmp. vars:
    int row = -1;
    QLabel * label;

    QGridLayout * glay = new QGridLayout( this, 5, 2, 0, KDialog::spacingHint() );
    glay->setRowStretch( 4, 1 );
    glay->setColStretch( 1, 1 );


    // "Server supports sieve" checkbox:
    ++row;
    mManagesieveCheck = new QCheckBox( i18n("&Server supports Sieve"), this );
    glay->addMultiCellWidget( mManagesieveCheck, row, row, 0, 1 );

    connect( mManagesieveCheck, SIGNAL(toggled(bool)), SLOT(slotEnableWidgets()) );

    // "reuse host and login config" checkbox:
    ++row;
    mSameConfigCheck = new QCheckBox( i18n("&Reuse host and login configuration"), this );
    mSameConfigCheck->setChecked( true );
    mSameConfigCheck->setEnabled( false );
    glay->addMultiCellWidget( mSameConfigCheck, row, row, 0, 1 );

    connect( mSameConfigCheck, SIGNAL(toggled(bool)), SLOT(slotEnableWidgets()) );

    // "Managesieve port" spinbox and label:
    ++row;
    mPortSpin = new KIntSpinBox( 1, USHRT_MAX, 1, 2000, 10, this );
    mPortSpin->setEnabled( false );
    label = new QLabel( mPortSpin, i18n("Managesieve &port:"), this );
    glay->addWidget( label, row, 0 );
    glay->addWidget( mPortSpin, row, 1 );

    // "Alternate URL" lineedit and label:
    ++row;
    mAlternateURLEdit = new KLineEdit( this );
    mAlternateURLEdit->setEnabled( false );
    glay->addWidget( new QLabel( mAlternateURLEdit, i18n("&Alternate URL:"), this ), row, 0 );
    glay->addWidget( mAlternateURLEdit, row, 1 );

    // row 4 is spacer

  }

  void SieveConfigEditor::slotEnableWidgets() {
    bool haveSieve = mManagesieveCheck->isChecked();
    bool reuseConfig = mSameConfigCheck->isChecked();

    mSameConfigCheck->setEnabled( haveSieve );
    mPortSpin->setEnabled( haveSieve && reuseConfig );
    mAlternateURLEdit->setEnabled( haveSieve && !reuseConfig );
  }

  bool SieveConfigEditor::managesieveSupported() const {
    return mManagesieveCheck->isChecked();
  }

  void SieveConfigEditor::setManagesieveSupported( bool enable ) {
    mManagesieveCheck->setChecked( enable );
  }

  bool SieveConfigEditor::reuseConfig() const {
    return mSameConfigCheck->isChecked();
  }

  void SieveConfigEditor::setReuseConfig( bool reuse ) {
    mSameConfigCheck->setChecked( reuse );
  }

  unsigned short SieveConfigEditor::port() const {
    return static_cast<unsigned short>( mPortSpin->value() );
  }

  void SieveConfigEditor::setPort( unsigned short port ) {
    mPortSpin->setValue( port );
  }

  KURL SieveConfigEditor::alternateURL() const {
    KURL url ( mAlternateURLEdit->text() );
    if ( !url.isValid() )
      return KURL();

    if ( url.hasPass() )
      url.setPass( QString::null );

    return url;
  }

  void SieveConfigEditor::setAlternateURL( const KURL & url ) {
    mAlternateURLEdit->setText( url.url() );
  }

  void SieveConfigEditor::setConfig( const SieveConfig & config ) {
    setManagesieveSupported( config.managesieveSupported() );
    setReuseConfig( config.reuseConfig() );
    setPort( config.port() );
    setAlternateURL( config.alternateURL() );
  }

} // namespace KMail

#include "sieveconfig.moc"
