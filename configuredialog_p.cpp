#ifndef KDE_USE_FINAL
#define QT_NO_CAST_ASCII
#endif
// configuredialog_p.cpp: classes internal to ConfigureDialog
// see configuredialog.cpp for details.


// my header:
#include "configuredialog_p.h"

// other KMail headers:
#include "globalsettings.h"
#include "kmacctcachedimap.h"
#include "configuredialoglistview.h"

// other kdenetwork headers: (none)

// other KDE headers:
#include <kconfig.h>
#include <kcombobox.h>
#include <kstandarddirs.h>
#include <ktabwidget.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfiggroup.h>
#include <kbuttongroup.h>
#include <kpimidentities/identitymanager.h>

// Qt headers:
#include <QRadioButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QResizeEvent>

// Other headers:
#include <assert.h>


NewIdentityDialog::NewIdentityDialog( const QStringList & identities,
                                      QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n("New Identity") );
  setButtons( Ok|Cancel|Help );
  setHelp( QString::fromLatin1("configure-identity-newidentitydialog") );
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout * vlay = new QVBoxLayout( page );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( 0 );

  // row 0: line edit with label
  QHBoxLayout * hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mLineEdit = new KLineEdit( page );
  mLineEdit->setFocus();
  mLineEdit->setClearButtonShown( true );
  QLabel *l = new QLabel( i18n("&New identity:"), page );
  l->setBuddy( mLineEdit );
  hlay->addWidget( l );
  hlay->addWidget( mLineEdit, 1 );
  connect( mLineEdit, SIGNAL(textChanged(const QString&)),
           this, SLOT(slotEnableOK(const QString&)) );

  mButtonGroup = new QButtonGroup( page );

  // row 1: radio button
  QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), page );
  radio->setChecked( true );
  vlay->addWidget( radio );
  mButtonGroup->addButton( radio, (int)Empty );

  // row 2: radio button
  radio = new QRadioButton( i18n("&Use Control Center settings"), page );
  vlay->addWidget( radio );
  mButtonGroup->addButton( radio, (int)ControlCenter );

  // row 3: radio button
  radio = new QRadioButton( i18n("&Duplicate existing identity"), page );
  vlay->addWidget( radio );
  mButtonGroup->addButton( radio, (int)ExistingEntry );

  // row 4: combobox with existing identities and label
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mComboBox = new KComboBox( page );
  mComboBox->setEditable( false );
  mComboBox->addItems( identities );
  mComboBox->setEnabled( false );
  QLabel *label = new QLabel( i18n("&Existing identities:"), page );
  label->setBuddy( mComboBox );
  label->setEnabled( false );
  hlay->addWidget( label );
  hlay->addWidget( mComboBox, 1 );

  vlay->addStretch( 1 ); // spacer

  // enable/disable combobox and label depending on the third radio
  // button's state:
  connect( radio, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  connect( radio, SIGNAL(toggled(bool)),
           mComboBox, SLOT(setEnabled(bool)) );

  enableButtonOk( false ); // since line edit is empty
}

NewIdentityDialog::DuplicateMode NewIdentityDialog::duplicateMode() const
{
  int id = mButtonGroup->checkedId();
  assert( id == (int)Empty
          || id == (int)ControlCenter
          || id == (int)ExistingEntry );
  return static_cast<DuplicateMode>( id );
}

void NewIdentityDialog::slotEnableOK( const QString & proposedIdentityName )
{
  // OK button is disabled if
  QString name = proposedIdentityName.trimmed();
  // name isn't empty
  if ( name.isEmpty() ) {
    enableButtonOk( false );
    return;
  }
  // or name doesn't yet exist.
  if ( ! kmkernel->identityManager()->isUnique( name ) ) {
    enableButtonOk( false );
    return;
  }
  enableButtonOk( true );
}

ConfigModuleWithTabs::ConfigModuleWithTabs( const KComponentData &instance, QWidget *parent )
  : ConfigModule( instance, parent )
{
  QVBoxLayout *vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( 0 );
  mTabWidget = new KTabWidget( this );
  vlay->addWidget( mTabWidget );
}

void ConfigModuleWithTabs::addTab( ConfigModuleTab* tab, const QString & title )
{
  mTabWidget->addTab( tab, title );
  connect( tab, SIGNAL(changed( bool )),
           this, SIGNAL(changed( bool )) );
}

void ConfigModuleWithTabs::load()
{
  for ( int i = 0 ; i < mTabWidget->count() ; ++i ) {
    ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->widget(i) );
    if ( tab )
      tab->load();
  }
  KCModule::load();
}

void ConfigModuleWithTabs::save()
{
  KCModule::save();
   for ( int i = 0 ; i < mTabWidget->count() ; ++i ) {
    ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->widget(i) );
    if ( tab )
      tab->save();
  }
}

void ConfigModuleWithTabs::defaults()
{
  ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->currentWidget() );
  if ( tab )
    tab->defaults();
  KCModule::defaults();
}

void ConfigModuleTab::load()
{
  doLoadFromGlobalSettings();
  doLoadOther();
}

void ConfigModuleTab::defaults()
{
  // reset settings which are available via GlobalSettings to their defaults
  // (stolen from KConfigDialogManager::updateWidgetsDefault())
  const bool bUseDefaults = GlobalSettings::self()->useDefaults( true );
  doLoadFromGlobalSettings();
  GlobalSettings::self()->useDefaults( bUseDefaults );
  // reset other settings to default values
  doResetToDefaultsOther();
}

void ConfigModuleTab::slotEmitChanged( void )
{
   emit changed( true );
}


#include "configuredialog_p.moc"
