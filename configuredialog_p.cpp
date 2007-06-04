#ifndef KDE_USE_FINAL
#define QT_NO_CAST_ASCII
#endif
// configuredialog_p.cpp: classes internal to ConfigureDialog
// see configuredialog.cpp for details.

// This must be first
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// my header:
#include "configuredialog_p.h"

// other KMail headers:
#include "globalsettings.h"
#include "kmacctcachedimap.h"

// other kdenetwork headers: (none)

// other KDE headers:
#include <kconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfiggroup.h>
#include <kbuttongroup.h>

// Qt headers:
#include <QTabWidget>
#include <QRadioButton>
#include <QLabel>
#include <QLayout>
//Added by qt3to4:
#include <QPixmap>
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
  QLabel *l = new QLabel( i18n("&New identity:"), page );
  l->setBuddy( mLineEdit );
  hlay->addWidget( l );
  hlay->addWidget( mLineEdit, 1 );
  connect( mLineEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotEnableOK(const QString&)) );

  mButtonGroup = new KButtonGroup( page );
  mButtonGroup->hide();

  // row 1: radio button
  QRadioButton *radio = new QRadioButton( i18n("&With empty fields"), mButtonGroup );
  radio->setChecked( true );
  vlay->addWidget( radio );

  // row 2: radio button
  radio = new QRadioButton( i18n("&Use Control Center settings"), mButtonGroup );
  vlay->addWidget( radio );

  // row 3: radio button
  radio = new QRadioButton( i18n("&Duplicate existing identity"), mButtonGroup );
  vlay->addWidget( radio );

  // row 4: combobox with existing identities and label
  hlay = new QHBoxLayout(); // inherits spacing
  vlay->addLayout( hlay );
  mComboBox = new QComboBox( page );
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

NewIdentityDialog::DuplicateMode NewIdentityDialog::duplicateMode() const {
  int id = mButtonGroup->selected();
  assert( id == (int)Empty
	  || id == (int)ControlCenter
	  || id == (int)ExistingEntry );
  return static_cast<DuplicateMode>( id );
}

void NewIdentityDialog::slotEnableOK( const QString & proposedIdentityName ) {
  // OK button is disabled if
  QString name = proposedIdentityName.trimmed();
  // name isn't empty
  if ( name.isEmpty() ) {
    enableButtonOk( false );
    return;
  }
  // or name doesn't yet exist.
  for ( int i = 0 ; i < mComboBox->count() ; i++ )
    if ( mComboBox->itemText(i) == name ) {
      enableButtonOk( false );
      return;
    }
  enableButtonOk( true );
}

ListView::ListView( QWidget *parent, int visibleItem )
  : K3ListView( parent )
{
  setVisibleItem(visibleItem);
}


void ListView::resizeEvent( QResizeEvent *e )
{
  K3ListView::resizeEvent(e);
  resizeColums();
}


void ListView::showEvent( QShowEvent *e )
{
  K3ListView::showEvent(e);
  resizeColums();
}


void ListView::resizeColums()
{
  int c = columns();
  if( c == 0 )
  {
    return;
  }

  int w1 = viewport()->width();
  int w2 = w1 / c;
  int w3 = w1 - (c-1)*w2;

  for( int i=0; i<c-1; i++ )
  {
    setColumnWidth( i, w2 );
  }
  setColumnWidth( c-1, w3 );
}


void ListView::setVisibleItem( int visibleItem, bool updateSize )
{
  mVisibleItem = qMax( 1, visibleItem );
  if( updateSize == true )
  {
    QSize s = sizeHint();
    setMinimumSize( s.width() + verticalScrollBar()->sizeHint().width() +
		    lineWidth() * 2, s.height() );
  }
}


QSize ListView::sizeHint() const
{
  QSize s = Q3ListView::sizeHint();

  int h = fontMetrics().height() + 2*itemMargin();
  if( h % 2 > 0 ) { h++; }

  s.setHeight( h*mVisibleItem + lineWidth()*2 + header()->sizeHint().height());
  return s;
}


static QString flagPng = QString::fromLatin1("/flag.png");

NewLanguageDialog::NewLanguageDialog( LanguageItemList & suppressedLangs,
				      QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n("New Language") );
  setButtons( Ok|Cancel );
  // layout the page (a combobox with label):
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QHBoxLayout *hlay = new QHBoxLayout( page );
  hlay->setSpacing( spacingHint() );
  hlay->setMargin( 0 );
  mComboBox = new QComboBox( page );
  mComboBox->setEditable( false );
  QLabel *l = new QLabel( i18n("Choose &language:"), page );
  l->setBuddy( mComboBox );
  hlay->addWidget( l );
  hlay->addWidget( mComboBox, 1 );

  QStringList pathList = KGlobal::dirs()->findAllResources( "locale",
                               QString::fromLatin1("*/entry.desktop") );
  // extract a list of language tags that should not be included:
  QStringList suppressedAcronyms;
  for ( LanguageItemList::Iterator lit = suppressedLangs.begin();
	lit != suppressedLangs.end(); ++lit )
    suppressedAcronyms << (*lit).mLanguage;

  // populate the combo box:
  for ( QStringList::ConstIterator it = pathList.begin();
	it != pathList.end(); ++it )
  {
    KConfig entry( *it, KConfig::OnlyLocal);
    KConfigGroup group( &entry, "KCM Locale" );
    // full name:
    QString name = group.readEntry( "Name" );
    // {2,3}-letter abbreviation:
    // we extract it from the path: "/prefix/de/entry.desktop" -> "de"
    QString acronym = (*it).section( '/', -2, -2 );

    if ( !suppressedAcronyms.contains( acronym )  ) {
      // not found:
      QString displayname = QString::fromLatin1("%1 (%2)")
	.arg( name ).arg( acronym );
      QPixmap flag( KStandardDirs::locate("locale", acronym + flagPng ) );
      mComboBox->addItem( flag, displayname );
    }
  }
  if ( !mComboBox->count() ) {
    mComboBox->addItem( i18n("No More Languages Available") );
    enableButtonOk( false );
  } else mComboBox->model()->sort( 0 );
}

QString NewLanguageDialog::language() const
{
  QString s = mComboBox->currentText();
  int i = s.lastIndexOf( '(' );
  return s.mid( i + 1, s.length() - i - 2 );
}


LanguageComboBox::LanguageComboBox( QWidget *parent )
  : QComboBox( parent )
{
}

int LanguageComboBox::insertLanguage( const QString & language )
{
  static QString entryDesktop = QString::fromLatin1("/entry.desktop");
  KConfig entry( KStandardDirs::locate("locale", language + entryDesktop) );
  KConfigGroup group( &entry, "KCM Locale" );
  QString name = group.readEntry( "Name" );
  QString output = QString::fromLatin1("%1 (%2)").arg( name ).arg( language );
  addItem( QPixmap( KStandardDirs::locate("locale", language + flagPng ) ), output );
  return findText(output);
}

QString LanguageComboBox::language() const
{
  QString s = currentText();
  int i = s.lastIndexOf( '(' );
  return s.mid( i + 1, s.length() - i - 2 );
}

void LanguageComboBox::setLanguage( const QString & language )
{
  QString parenthizedLanguage = QString::fromLatin1("(%1)").arg( language );
  for (int i = 0; i < count(); i++)
    // ### FIXME: use .endWith():
    if ( itemText(i).contains( parenthizedLanguage ) ) {
      setCurrentIndex(i);
      return;
    }
}

//
//
//  ProfileDialog
//
//

ProfileDialog::ProfileDialog( QWidget * parent )
  : KDialog( parent )
{
  setCaption( i18n("Load Profile") );
  setButtons( Ok|Cancel );
  // tmp. vars:
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout * vlay = new QVBoxLayout( page );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( 0 );

  mListView = new K3ListView( page );
  mListView->setObjectName( "mListView" );
  mListView->addColumn( i18n("Available Profiles") );
  mListView->addColumn( i18n("Description") );
  mListView->setFullWidth( true );
  mListView->setAllColumnsShowFocus( true );
  mListView->setSorting( -1 );

  QLabel *l = new QLabel(
			       i18n("&Select a profile and click 'OK' to "
				    "load its settings:"), page );
	l->setBuddy( mListView );
	vlay->addWidget( l );
  vlay->addWidget( mListView, 1 );

  setup();

  connect( mListView, SIGNAL(selectionChanged()),
	   SLOT(slotSelectionChanged()) );
  connect( mListView, SIGNAL(doubleClicked ( Q3ListViewItem *, const QPoint &, int ) ),
	   SLOT(slotOk()) );

  connect( this, SIGNAL(finished()), SLOT(deleteLater()) );
  connect( this, SIGNAL(okClicked()), SLOT( slotOk() ) );

  enableButtonOk( false );
}

void ProfileDialog::slotSelectionChanged()
{
  enableButtonOk( mListView->selectedItem() );
}

void ProfileDialog::setup() {
  mListView->clear();
  // find all profiles (config files named "profile-xyz-rc"):
  const QString profileFilenameFilter = QString::fromLatin1("kmail/profile-*-rc");
  mProfileList = KGlobal::dirs()->findAllResources( "data", profileFilenameFilter );

  kDebug(5006) << "Profile manager: found " << mProfileList.count()
		<< " profiles:" << endl;

  // build the list and populate the list view:
  Q3ListViewItem * listItem = 0;
  for ( QStringList::const_iterator it = mProfileList.begin() ;
	it != mProfileList.end() ; ++it ) {
    KConfig _profile( *it, KConfig::NoGlobals  );
    KConfigGroup profile(&_profile, "KMail Profile");
    QString name = profile.readEntry( "Name" );
    if ( name.isEmpty() ) {
      kWarning(5006) << "File \"" << (*it)
		      << "\" doesn't provide a profile name!" << endl;
      name = i18nc("Missing profile name placeholder","Unnamed");
    }
    QString desc = profile.readEntry( "Comment" );
    if ( desc.isEmpty() ) {
      kWarning(5006) << "File \"" << (*it)
		      << "\" doesn't provide a description!" << endl;
      desc = i18nc("Missing profile description placeholder","Not available");
    }
    listItem = new Q3ListViewItem( mListView, listItem, name, desc );
  }
}

void ProfileDialog::slotOk() {
  const int index = mListView->itemIndex( mListView->selectedItem() );
  if ( index < 0 )
    return; // none selected

  assert( index < mProfileList.count() );

  KConfig profile( mProfileList.at( index), KConfig::NoGlobals );
  emit profileSelected( &profile );
}


ConfigModuleWithTabs::ConfigModuleWithTabs( const KComponentData &instance, QWidget *parent, const QStringList &args )
  : ConfigModule( instance, parent, args )
{
  QVBoxLayout *vlay = new QVBoxLayout( this );
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( 0 );
  mTabWidget = new QTabWidget( this );
  vlay->addWidget( mTabWidget );
}

void ConfigModuleWithTabs::addTab( ConfigModuleTab* tab, const QString & title ) {
  mTabWidget->addTab( tab, title );
  connect( tab, SIGNAL(changed( bool )),
	        this, SIGNAL(changed( bool )) );
}

void ConfigModuleWithTabs::load() {
  for ( int i = 0 ; i < mTabWidget->count() ; ++i ) {
    ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->widget(i) );
    if ( tab )
      tab->load();
  }
  KCModule::load();
}

void ConfigModuleWithTabs::save() {
  KCModule::save();
   for ( int i = 0 ; i < mTabWidget->count() ; ++i ) {
    ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->widget(i) );
    if ( tab )
      tab->save();
  }
}

void ConfigModuleWithTabs::defaults() {
  ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->currentWidget() );
  if ( tab )
    tab->defaults();
  KCModule::defaults();
}

void ConfigModuleWithTabs::installProfile(KConfig * /* profile */ ) {
  for ( int i = 0 ; i < mTabWidget->count() ; ++i ) {
    ConfigModuleTab *tab = dynamic_cast<ConfigModuleTab*>( mTabWidget->widget(i) );
    if ( tab )
      tab->installProfile();
  }
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

void ConfigModuleTab::slotEmitChanged( void ) {
   emit changed( true );
}


#include "configuredialog_p.moc"
