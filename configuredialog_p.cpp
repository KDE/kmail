#ifndef KDE_USE_FINAL
#define QT_NO_CAST_ASCII
#endif
// configuredialog_p.cpp: classes internal to ConfigureDialog
// see configuredialog.cpp for details.


// my header:
#include "configuredialog_p.h"

// other KMail headers:
#include "globalsettings.h"
#include "configuredialoglistview.h"
#include "kmkernel.h"
#include <messageviewer/invitationsettings.h>

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
