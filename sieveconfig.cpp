/*  -*- c++ -*-
    sieveconfig.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "sieveconfig.h"

#include <knuminput.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfigbase.h>

#include <QLayout>
#include <QCheckBox>
#include <QLabel>
//Added by qt3to4:
#include <QGridLayout>
#include <klineedit.h>


namespace KMail {

  void SieveConfig::readConfig( const KConfigGroup & config ) {
    mManagesieveSupported = config.readEntry( "sieve-support", false );
    mReuseConfig = config.readEntry( "sieve-reuse-config", true );

    int port = config.readEntry( "sieve-port", 2000 );
    if ( port < 1 || port > USHRT_MAX ) port = 2000;
    mPort = static_cast<unsigned short>( port );

    mAlternateURL = config.readEntry( "sieve-alternate-url" );
    mVacationFileName = config.readEntry( "sieve-vacation-filename", "kmail-vacation.siv" );
    if ( mVacationFileName.isEmpty() )
      mVacationFileName = "kmail-vacation.siv";
  }

  void SieveConfig::writeConfig( KConfigGroup & config ) const {
    config.writeEntry( "sieve-support", managesieveSupported() );
    config.writeEntry( "sieve-reuse-config", reuseConfig() );
    config.writeEntry( "sieve-port", (int)port() );
    config.writeEntry( "sieve-alternate-url", mAlternateURL.url() );
    config.writeEntry( "sieve-vacation-filename", mVacationFileName );
  }

  QString SieveConfig::toString() const
  {
    QString result;
    result += "SieveConfig: \n";
    result += "  sieve-vacation-filename: " + mVacationFileName + "\n\n";
    return result;
  }

  SieveConfigEditor::SieveConfigEditor( QWidget * parent )
    : QWidget( parent )
  {
    // tmp. vars:
    int row = -1;
    QLabel * label;

    QGridLayout * glay = new QGridLayout( this );
    glay->setSpacing( KDialog::spacingHint() );
    glay->setMargin( 0 );
    glay->setRowStretch( 4, 1 );
    glay->setColumnStretch( 1, 1 );


    // "Server supports sieve" checkbox:
    ++row;
    mManagesieveCheck = new QCheckBox( i18n("&Server supports Sieve"), this );
    glay->addWidget( mManagesieveCheck, row, 0, 1, 2 );

    connect( mManagesieveCheck, SIGNAL(toggled(bool)), SLOT(slotEnableWidgets()) );

    // "reuse host and login config" checkbox:
    ++row;
    mSameConfigCheck = new QCheckBox( i18n("&Reuse host and login configuration"), this );
    mSameConfigCheck->setChecked( true );
    mSameConfigCheck->setEnabled( false );
    glay->addWidget( mSameConfigCheck, row, 0, 1, 2 );

    connect( mSameConfigCheck, SIGNAL(toggled(bool)), SLOT(slotEnableWidgets()) );

    // "Managesieve port" spinbox and label:
    ++row;
    mPortSpin = new KIntSpinBox( 1, USHRT_MAX, 1, 2000, this );
    mPortSpin->setEnabled( false );
    label = new QLabel( i18n("Managesieve &port:"), this );
    label->setBuddy( mPortSpin );
    glay->addWidget( label, row, 0 );
    glay->addWidget( mPortSpin, row, 1 );

    // "Alternate URL" lineedit and label:
    ++row;
    mAlternateURLEdit = new KLineEdit( this );
    mAlternateURLEdit->setEnabled( false );
    label = new QLabel( i18n("&Alternate URL:"), this );
    label->setBuddy( mAlternateURLEdit );
    glay->addWidget( label, row, 0 );
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

  KUrl SieveConfigEditor::alternateURL() const {
    KUrl url ( mAlternateURLEdit->text() );
    if ( !url.isValid() )
      return KUrl();

    if ( url.hasPass() )
      url.setPass( QString() );

    return url;
  }

  void SieveConfigEditor::setAlternateURL( const KUrl & url ) {
    mAlternateURLEdit->setText( url.url() );
  }


  QString SieveConfigEditor::vacationFileName() const {
    return mVacationFileName;
  }

  void SieveConfigEditor::setVacationFileName( const QString& name ) {
    mVacationFileName = name;
  }

  void SieveConfigEditor::setConfig( const SieveConfig & config ) {
    setManagesieveSupported( config.managesieveSupported() );
    setReuseConfig( config.reuseConfig() );
    setPort( config.port() );
    setAlternateURL( config.alternateURL() );
    setVacationFileName( config.vacationFileName() );
  }

} // namespace KMail

#include "sieveconfig.moc"
