// -*- mode: C++; c-file-style: "gnu" -*-
// kmfilterdlg.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// based on work by Stefan Taferner <taferner@kde.org>
// This code is under the GPL

#include <config.h>
#include "kmfilterdlg.h"

// other KMail headers:
#include "kmsearchpatternedit.h"
#include "kmfiltermgr.h"
#include "kmmainwidget.h"
#include "accountmanager.h"
using KMail::AccountManager;

// other KDE headers:
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kwin.h>
#include <kconfig.h>
#include <kicondialog.h>
#include <kkeybutton.h>
#include <k3listview.h>
#include <kpushbutton.h>
#include <kconfiggroup.h>
#include <kvbox.h>

// Qt headers:
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

// other headers:
#include <assert.h>


// What's this help texts
const char * _wt_filterlist =
I18N_NOOP( "<qt><p>This is the list of defined filters. "
	   "They are processed top-to-bottom.</p>"
	   "<p>Click on any filter to edit it "
	   "using the controls in the right-hand half "
	   "of the dialog.</p></qt>" );
const char * _wt_filterlist_new =
I18N_NOOP( "<qt><p>Click this button to create a new filter.</p>"
	   "<p>The filter will be inserted just before the currently-"
	   "selected one, but you can always change that "
	   "later on.</p>"
	   "<p>If you have clicked this button accidentally, you can undo this "
	   "by clicking on the <em>Delete</em> button.</p></qt>" );
const char * _wt_filterlist_copy =
I18N_NOOP( "<qt><p>Click this button to copy a filter.</p>"
	   "<p>If you have clicked this button accidentally, you can undo this "
	   "by clicking on the <em>Delete</em> button.</p></qt>" );
const char * _wt_filterlist_delete =
I18N_NOOP( "<qt><p>Click this button to <em>delete</em> the currently-"
	   "selected filter from the list above.</p>"
	   "<p>There is no way to get the filter back once "
	   "it is deleted, but you can always leave the "
	   "dialog by clicking <em>Cancel</em> to discard the "
	   "changes made.</p></qt>" );
const char * _wt_filterlist_up =
I18N_NOOP( "<qt><p>Click this button to move the currently-"
	   "selected filter <em>up</em> one in the list above.</p>"
	   "<p>This is useful since the order of the filters in the list "
	   "determines the order in which they are tried on messages: "
	   "The topmost filter gets tried first.</p>"
	   "<p>If you have clicked this button accidentally, you can undo this "
	   "by clicking on the <em>Down</em> button.</p></qt>" );
const char * _wt_filterlist_down =
I18N_NOOP( "<qt><p>Click this button to move the currently-"
	   "selected filter <em>down</em> one in the list above.</p>"
	   "<p>This is useful since the order of the filters in the list "
	   "determines the order in which they are tried on messages: "
	   "The topmost filter gets tried first.</p>"
	   "<p>If you have clicked this button accidentally, you can undo this "
	   "by clicking on the <em>Up</em> button.</p></qt>" );
const char * _wt_filterlist_rename =
I18N_NOOP( "<qt><p>Click this button to rename the currently-selected filter.</p>"
	   "<p>Filters are named automatically, as long as they start with "
	   "\"&lt;\".</p>"
	   "<p>If you have renamed a filter accidentally and want automatic "
	   "naming back, click this button and select <em>Clear</em> followed "
	   "by <em>OK</em> in the appearing dialog.</p></qt>" );
const char * _wt_filterdlg_showLater =
I18N_NOOP( "<qt><p>Check this button to force the confirmation dialog to be "
	   "displayed.</p><p>This is useful if you have defined a ruleset that tags "
           "messages to be downloaded later. Without the possibility to force "
           "the dialog popup, these messages could never be downloaded if no "
           "other large messages were waiting on the server, or if you wanted to "
           "change the ruleset to tag the messages differently.</p></qt>" );

// The anchor of the filter dialog's help.
const char * KMFilterDlgHelpAnchor =  "filters-id" ;
const char * KMPopFilterDlgHelpAnchor =  "popfilters-id" ;

//=============================================================================
//
// class KMFilterDlg (the filter dialog)
//
//=============================================================================

KMFilterDlg::KMFilterDlg(QWidget* parent, bool popFilter, bool createDummyFilter )
  : KDialog( parent ),
  bPopFilter(popFilter)
{
  if ( popFilter )
    setCaption( i18n("POP3 Filter Rules") );
  else
    setCaption( i18n("Filter Rules") );
  setButtons( Help|Ok|Apply|Cancel );
  setModal( false );
#ifdef Q_OS_UNIX  
  KWin::setIcons( winId(), qApp->windowIcon().pixmap(IconSize(K3Icon::Desktop),IconSize(K3Icon::Desktop)), qApp->windowIcon().pixmap(IconSize(K3Icon::Small),IconSize(K3Icon::Small)) );
#endif  
  setHelp( (bPopFilter)? KMPopFilterDlgHelpAnchor: KMFilterDlgHelpAnchor );

  QWidget *w = new QWidget( this );
  setMainWidget( w );
  QHBoxLayout *topLayout = new QHBoxLayout( w );
  topLayout->setObjectName( "topLayout" );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( 0 );
  QHBoxLayout *hbl = topLayout;
  QVBoxLayout *vbl2 = 0;
  QWidget *page1 = 0;
  QWidget *page2 = 0;

  mFilterList = new KMFilterListBox( i18n("Available Filters"), w, 0, bPopFilter);
  topLayout->addWidget( mFilterList, 1 /*stretch*/ );

  if(!bPopFilter) {
    QTabWidget *tabWidget = new QTabWidget( w );
    tabWidget->setObjectName( "kmfd_tab" );
    topLayout->addWidget( tabWidget );

    page1 = new QWidget( tabWidget );
    tabWidget->addTab( page1, i18n("&General") );
    hbl = new QHBoxLayout( page1 );
    hbl->setObjectName( "kmfd_hbl" );
    hbl->setSpacing( spacingHint() );
    hbl->setMargin( 0 );

    page2 = new QWidget( tabWidget );
    tabWidget->addTab( page2, i18n("A&dvanced") );
    vbl2 = new QVBoxLayout( page2 );
    vbl2->setObjectName( "kmfd_vbl2" );
    vbl2->setSpacing( spacingHint() );
    vbl2->setMargin( 0 );
  }

  QVBoxLayout *vbl = new QVBoxLayout();
  hbl->addLayout( vbl );
  vbl->setObjectName( "kmfd_vbl" );
  vbl->setSpacing( spacingHint() );
  hbl->setStretchFactor( vbl, 2 );

  mPatternEdit = new KMSearchPatternEdit( i18n("Filter Criteria"), bPopFilter ? w : page1 , "spe", bPopFilter);
  vbl->addWidget( mPatternEdit, 0, Qt::AlignTop );

  if(bPopFilter){
    mActionGroup = new KMPopFilterActionWidget( i18n("Filter Action"), w );
    vbl->addWidget( mActionGroup, 0, Qt::AlignTop );

    mGlobalsBox = new QGroupBox( i18n("Global Options"), w);
    QHBoxLayout *layout = new QHBoxLayout;
    mShowLaterBtn = new QCheckBox( i18n("Always &show matched 'Download Later' messages in confirmation dialog"), mGlobalsBox);
    mShowLaterBtn->setWhatsThis( i18n(_wt_filterdlg_showLater) );
    layout->addWidget( mShowLaterBtn );
    mGlobalsBox->setLayout( layout );
    vbl->addWidget( mGlobalsBox, 0, Qt::AlignTop );
  }
  else {
    QGroupBox *agb = new QGroupBox( i18n("Filter Actions"), page1 );
    QHBoxLayout *layout = new QHBoxLayout;
    mActionLister = new KMFilterActionWidgetLister( agb );
    layout->addWidget( mActionLister );
    agb->setLayout( layout );
    vbl->addWidget( agb, 0, Qt::AlignTop );

    mAdvOptsGroup = new QGroupBox (i18n("Advanced Options"), page2);
    {
      QGridLayout *gl = new QGridLayout();
      QVBoxLayout *vbl3 = new QVBoxLayout();
      gl->addLayout( vbl3, 0, 0 );
      vbl3->setObjectName( "vbl3" );
      vbl3->setSpacing( spacingHint() );
      vbl3->addStretch( 1 );
      mApplyOnIn = new QCheckBox( i18n("Apply this filter to incoming messages:"), mAdvOptsGroup );
      vbl3->addWidget( mApplyOnIn );
      QButtonGroup *bg = new QButtonGroup( mAdvOptsGroup );
      bg->setObjectName( "bg" );
      mApplyOnForAll = new QRadioButton( i18n("from all accounts"), mAdvOptsGroup );
      bg->addButton( mApplyOnForAll );
      vbl3->addWidget( mApplyOnForAll );
      mApplyOnForTraditional = new QRadioButton( i18n("from all but online IMAP accounts"), mAdvOptsGroup );
      bg->addButton( mApplyOnForTraditional );
      vbl3->addWidget( mApplyOnForTraditional );
      mApplyOnForChecked = new QRadioButton( i18n("from checked accounts only"), mAdvOptsGroup );
      bg->addButton( mApplyOnForChecked );
      vbl3->addWidget( mApplyOnForChecked );
      vbl3->addStretch( 2 );

      mAccountList = new K3ListView( mAdvOptsGroup );
      mAccountList->setObjectName( "accountList" );
      mAccountList->addColumn( i18n("Account Name") );
      mAccountList->addColumn( i18n("Type") );
      mAccountList->setAllColumnsShowFocus( true );
      mAccountList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
      mAccountList->setSorting( -1 );
      gl->addWidget( mAccountList, 0, 1, 4, 3 );

      mApplyOnOut = new QCheckBox( i18n("Apply this filter to &sent messages"), mAdvOptsGroup );
      gl->addWidget( mApplyOnOut, 4, 0, 1, 4 );

      mApplyOnCtrlJ = new QCheckBox( i18n("Apply this filter on manual &filtering"), mAdvOptsGroup );
      gl->addWidget( mApplyOnCtrlJ, 5, 0, 1, 4 );

      mStopProcessingHere = new QCheckBox( i18n("If this filter &matches, stop processing here"), mAdvOptsGroup );
      gl->addWidget( mStopProcessingHere, 6, 0, 1, 4 );
      mConfigureShortcut = new QCheckBox( i18n("Add this filter to the Apply Filter menu"), mAdvOptsGroup );
      gl->addWidget( mConfigureShortcut, 7, 0, 1, 2 );
      QLabel *keyButtonLabel = new QLabel( i18n( "Shortcut:" ), mAdvOptsGroup );
      keyButtonLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
      gl->addWidget( keyButtonLabel, 7, 2, 1, 1);
      mKeyButton = new KKeyButton( mAdvOptsGroup );
      mKeyButton->setObjectName( "FilterShortcutSelector" );
      gl->addWidget( mKeyButton, 7, 3, 1, 1);
      mKeyButton->setEnabled( false );
      mConfigureToolbar = new QCheckBox( i18n("Additionally add this filter to the toolbar"), mAdvOptsGroup );
      gl->addWidget( mConfigureToolbar, 8, 0, 1, 4 );
      mConfigureToolbar->setEnabled( false );

      KHBox *hbox = new KHBox( mAdvOptsGroup );
      mFilterActionLabel = new QLabel( i18n( "Icon for this filter:" ),
                                       hbox );
      mFilterActionLabel->setEnabled( false );

      mFilterActionIconButton = new KIconButton( hbox );
      mFilterActionLabel->setBuddy( mFilterActionIconButton );
      mFilterActionIconButton->setIconType( K3Icon::NoGroup, K3Icon::Any, true );
      mFilterActionIconButton->setIconSize( 16 );
      mFilterActionIconButton->setIcon( "gear" );
      mFilterActionIconButton->setEnabled( false );

      gl->addWidget( hbox, 9, 0, 1, 4 );

      mAdvOptsGroup->setLayout( gl );
    }
    vbl2->addWidget( mAdvOptsGroup, 0, Qt::AlignTop );
  }
  // spacer:
  vbl->addStretch( 1 );

  // load the filter parts into the edit widgets
  connect( mFilterList, SIGNAL(filterSelected(KMFilter*)),
	   this, SLOT(slotFilterSelected(KMFilter*)) );

  if (bPopFilter){
    // set the state of the global setting 'show later msgs'
    connect( mShowLaterBtn, SIGNAL(toggled(bool)),
             mFilterList, SLOT(slotShowLaterToggled(bool)));

    // set the action in the filter when changed
    connect( mActionGroup, SIGNAL(actionChanged(const KMPopFilterAction)),
	     this, SLOT(slotActionChanged(const KMPopFilterAction)) );
  } else {
    // transfer changes from the 'Apply this filter on...'
    // combo box to the filter
    connect( mApplyOnIn, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnForAll, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnForTraditional, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnForChecked, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnOut, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnCtrlJ, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mAccountList, SIGNAL(clicked(Q3ListViewItem*)),
  	     this, SLOT(slotApplicableAccountsChanged()) );
    connect( mAccountList, SIGNAL(spacePressed(Q3ListViewItem*)),
  	     this, SLOT(slotApplicableAccountsChanged()) );

    // transfer changes from the 'stop processing here'
    // check box to the filter
    connect( mStopProcessingHere, SIGNAL(toggled(bool)),
	     this, SLOT(slotStopProcessingButtonToggled(bool)) );

    connect( mConfigureShortcut, SIGNAL(toggled(bool)),
	     this, SLOT(slotConfigureShortcutButtonToggled(bool)) );

    connect( mKeyButton, SIGNAL( capturedKeySequence(const QKeySequence &)  ),
             this, SLOT( slotCapturedShortcutChanged( const QKeySequence& ) ) );

    connect( mConfigureToolbar, SIGNAL(toggled(bool)),
	     this, SLOT(slotConfigureToolbarButtonToggled(bool)) );

    connect( mFilterActionIconButton, SIGNAL( iconChanged( QString ) ),
             this, SLOT( slotFilterActionIconChanged( QString ) ) );
  }

  // reset all widgets here
  connect( mFilterList, SIGNAL(resetWidgets()),
	   this, SLOT(slotReset()) );

  connect( mFilterList, SIGNAL( applyWidgets() ),
           this, SLOT( slotUpdateFilter() ) );

  // support auto-naming the filter
  connect( mPatternEdit, SIGNAL(maybeNameChanged()),
	   mFilterList, SLOT(slotUpdateFilterName()) );

  // apply changes on 'Apply'
  connect( this, SIGNAL(applyClicked()),
	   mFilterList, SLOT(slotApplyFilterChanges()) );

  // apply changes on 'OK'
  connect( this, SIGNAL(okClicked()),
	   mFilterList, SLOT(slotApplyFilterChanges()) );

  // save dialog size on 'OK'
  connect( this, SIGNAL(okClicked()),
	   this, SLOT(slotSaveSize()) );

  // destruct the dialog on OK, close and Cancel
  connect( this, SIGNAL(finished()),
	   this, SLOT(slotFinished()) );

  KConfigGroup geometry( KMKernel::config(), "Geometry");
  const char * configKey
    = bPopFilter ? "popFilterDialogSize" : "filterDialogSize";
  if ( geometry.hasKey( configKey ) )
    resize( geometry.readEntry( configKey, QSize() ));
  else
    adjustSize();

  // load the filter list (emits filterSelected())
  mFilterList->loadFilterList( createDummyFilter );
}

void KMFilterDlg::slotFinished() {
	deleteLater();
}

void KMFilterDlg::slotSaveSize() {
  KConfigGroup geometry( KMKernel::config(), "Geometry" );
  geometry.writeEntry( bPopFilter ? "popFilterDialogSize" : "filterDialogSize", size() );
}

/** Set action of popFilter */
void KMFilterDlg::slotActionChanged(const KMPopFilterAction aAction)
{
  mFilter->setAction(aAction);
}

void KMFilterDlg::slotFilterSelected( KMFilter* aFilter )
{
  assert( aFilter );

  if (bPopFilter){
    mActionGroup->setAction( aFilter->action() );
    mGlobalsBox->setEnabled(true);
    mShowLaterBtn->setChecked(mFilterList->showLaterMsgs());
  } else {
    mActionLister->setActionList( aFilter->actions() );

    mAdvOptsGroup->setEnabled( true );
  }

  mPatternEdit->setSearchPattern( aFilter->pattern() );
  mFilter = aFilter;

  if (!bPopFilter) {
    kDebug(5006) << "apply on inbound == "
		  << aFilter->applyOnInbound() << endl;
    kDebug(5006) << "apply on outbound == "
		  << aFilter->applyOnOutbound() << endl;
    kDebug(5006) << "apply on explicit == "
		  << aFilter->applyOnExplicit() << endl;

    // NOTE: setting these values activates the slot that sets them in
    // the filter! So make sure we have the correct values _before_ we
    // set the first one:
    const bool applyOnIn = aFilter->applyOnInbound();
    const bool applyOnForAll = aFilter->applicability() == KMFilter::All;
    const bool applyOnTraditional = aFilter->applicability() == KMFilter::ButImap;
    const bool applyOnOut = aFilter->applyOnOutbound();
    const bool applyOnExplicit = aFilter->applyOnExplicit();
    const bool stopHere = aFilter->stopProcessingHere();
    const bool configureShortcut = aFilter->configureShortcut();
    const bool configureToolbar = aFilter->configureToolbar();
    const QString icon = aFilter->icon();
    const KShortcut shortcut( aFilter->shortcut() );

    mApplyOnIn->setChecked( applyOnIn );
    mApplyOnForAll->setEnabled( applyOnIn );
    mApplyOnForTraditional->setEnabled( applyOnIn );
    mApplyOnForChecked->setEnabled( applyOnIn );
    mApplyOnForAll->setChecked( applyOnForAll );
    mApplyOnForTraditional->setChecked( applyOnTraditional );
    mApplyOnForChecked->setChecked( !applyOnForAll && !applyOnTraditional );
    mAccountList->setEnabled( mApplyOnForChecked->isEnabled() && mApplyOnForChecked->isChecked() );
    slotUpdateAccountList();
    mApplyOnOut->setChecked( applyOnOut );
    mApplyOnCtrlJ->setChecked( applyOnExplicit );
    mStopProcessingHere->setChecked( stopHere );
    mConfigureShortcut->setChecked( configureShortcut );
#ifdef __GNUC__
#warning Port me!
#endif
//    mKeyButton->setShortcut( shortcut );
    mConfigureToolbar->setChecked( configureToolbar );
    mFilterActionIconButton->setIcon( icon );
  }
}

void KMFilterDlg::slotReset()
{
  mFilter = 0;
  mPatternEdit->reset();

  if(bPopFilter) {
    mActionGroup->reset();
    mGlobalsBox->setEnabled( false );
  } else {
    mActionLister->reset();
    mAdvOptsGroup->setEnabled( false );
    slotUpdateAccountList();
  }
}

void KMFilterDlg::slotUpdateFilter()
{
  mPatternEdit->updateSearchPattern();
  if ( !bPopFilter ) {
    mActionLister->updateActionList();
  }
}

void KMFilterDlg::slotApplicabilityChanged()
{
  if ( mFilter ) {
    mFilter->setApplyOnInbound( mApplyOnIn->isChecked() );
    mFilter->setApplyOnOutbound( mApplyOnOut->isChecked() );
    mFilter->setApplyOnExplicit( mApplyOnCtrlJ->isChecked() );
    if ( mApplyOnForAll->isChecked() )
      mFilter->setApplicability( KMFilter::All );
    else if ( mApplyOnForTraditional->isChecked() )
      mFilter->setApplicability( KMFilter::ButImap );
    else if ( mApplyOnForChecked->isChecked() )
      mFilter->setApplicability( KMFilter::Checked );

    mApplyOnForAll->setEnabled( mApplyOnIn->isChecked() );
    mApplyOnForTraditional->setEnabled(  mApplyOnIn->isChecked() );
    mApplyOnForChecked->setEnabled( mApplyOnIn->isChecked() );
    mAccountList->setEnabled( mApplyOnForChecked->isEnabled() && mApplyOnForChecked->isChecked() );

    // Advanced tab functionality - Update list of accounts this filter applies to
    Q3ListViewItemIterator it( mAccountList );
    while ( it.current() ) {
      Q3CheckListItem *item = dynamic_cast<Q3CheckListItem*>( it.current() );
      if (item) {
	int id = item->text( 2 ).toInt();
	  item->setOn( mFilter->applyOnAccount( id ) );
      }
      ++it;
    }

    kDebug(5006) << "KMFilterDlg: setting filter to be applied at "
                  << ( mFilter->applyOnInbound() ? "incoming " : "" )
                  << ( mFilter->applyOnOutbound() ? "outgoing " : "" )
                  << ( mFilter->applyOnExplicit() ? "explicit CTRL-J" : "" )
                  << endl;
  }
}

void KMFilterDlg::slotApplicableAccountsChanged()
{
  if ( mFilter && mApplyOnForChecked->isEnabled() && mApplyOnForChecked->isChecked() ) {
    // Advanced tab functionality - Update list of accounts this filter applies to
    Q3ListViewItemIterator it( mAccountList );
    while ( it.current() ) {
      Q3CheckListItem *item = dynamic_cast<Q3CheckListItem*>( it.current() );
      if (item) {
	int id = item->text( 2 ).toInt();
	mFilter->setApplyOnAccount( id, item->isOn() );
      }
      ++it;
    }
  }
}

void KMFilterDlg::slotStopProcessingButtonToggled( bool aChecked )
{
  if ( mFilter )
    mFilter->setStopProcessingHere( aChecked );
}

void KMFilterDlg::slotConfigureShortcutButtonToggled( bool aChecked )
{
  if ( mFilter ) {
    mFilter->setConfigureShortcut( aChecked );
    mKeyButton->setEnabled( aChecked );
    mConfigureToolbar->setEnabled( aChecked );
    mFilterActionIconButton->setEnabled( aChecked );
    mFilterActionLabel->setEnabled( aChecked );
  }
}

void KMFilterDlg::slotCapturedShortcutChanged( const QKeySequence& sc )
{
  KShortcut mySc(sc);
#ifdef __GNUC__
#warning Port me!
#endif
//  if ( mySc == mKeyButton->shortcut() ) return;
  // FIXME work around a problem when reseting the shortcut via the shortcut dialog
  // somehow the returned shortcut does not evaluate to true in KShortcut::isNull(),
  // so we additionally have to check for an empty string
  if ( mySc.isEmpty() || mySc.toString().isEmpty() )
    mySc.clear();
  if ( !mySc.isEmpty() && !( kmkernel->getKMMainWidget()->shortcutIsValid( sc ) ) ) {
    QString msg( i18n( "The selected shortcut is already used, "
          "please select a different one." ) );
    KMessageBox::sorry( this, msg );
  } else {
#ifdef __GNUC__
#warning Port me!
#endif
//    mKeyButton->setShortcut( mySc );
//    if ( mFilter )
//      mFilter->setShortcut( mKeyButton->shortcut() );
  }
}

void KMFilterDlg::slotConfigureToolbarButtonToggled( bool aChecked )
{
  if ( mFilter )
    mFilter->setConfigureToolbar( aChecked );
}

void KMFilterDlg::slotFilterActionIconChanged( QString icon )
{
  if ( mFilter )
    mFilter->setIcon( icon );
}

void KMFilterDlg::slotUpdateAccountList()
{
  mAccountList->clear();
  Q3ListViewItem *top = 0;
  for( KMAccount *a = kmkernel->acctMgr()->first(); a!=0;
       a = kmkernel->acctMgr()->next() ) {
    Q3CheckListItem *listItem =
      new Q3CheckListItem( mAccountList, top, a->name(), Q3CheckListItem::CheckBox );
    listItem->setText( 1, a->type() );
    listItem->setText( 2, QString( "%1" ).arg( a->id() ) );
    if ( mFilter )
      listItem->setOn( mFilter->applyOnAccount( a->id() ) );
    top = listItem;
  }

  Q3ListViewItem *listItem = mAccountList->firstChild();
  if ( listItem ) {
    mAccountList->setCurrentItem( listItem );
    mAccountList->setSelected( listItem, true );
  }
}

//=============================================================================
//
// class KMFilterListBox (the filter list manipulator)
//
//=============================================================================

KMFilterListBox::KMFilterListBox( const QString & title, QWidget *parent,
                                  const char* name, bool popFilter )
  : QGroupBox( title, parent ),
    bPopFilter(popFilter)
{
  setObjectName( name );
  QVBoxLayout *layout = new QVBoxLayout();

  mIdxSelItem = -1;

  //----------- the list box
  mListWidget = new QListWidget(this);
  mListWidget->setMinimumWidth(150);
  mListWidget->setWhatsThis( i18n(_wt_filterlist) );

  layout->addWidget( mListWidget );

  //----------- the first row of buttons
  KHBox *hb = new KHBox(this);
  hb->setSpacing(4);
  mBtnUp = new KPushButton( QString(), hb );
  mBtnUp->setAutoRepeat( true );
  mBtnUp->setIcon( BarIconSet( "go-up", K3Icon::SizeSmall ) );
  mBtnUp->setMinimumSize( mBtnUp->sizeHint() * 1.2 );
  mBtnDown = new KPushButton( QString(), hb );
  mBtnDown->setAutoRepeat( true );
  mBtnDown->setIcon( BarIconSet( "go-down", K3Icon::SizeSmall ) );
  mBtnDown->setMinimumSize( mBtnDown->sizeHint() * 1.2 );
  mBtnUp->setToolTip( i18n("Up") );
  mBtnDown->setToolTip( i18n("Down") );
  mBtnUp->setWhatsThis( i18n(_wt_filterlist_up) );
  mBtnDown->setWhatsThis( i18n(_wt_filterlist_down) );

  layout->addWidget( hb );

  //----------- the second row of buttons
  hb = new KHBox(this);
  hb->setSpacing(4);
  mBtnNew = new QPushButton( QString(), hb );
  mBtnNew->setIcon( BarIconSet( "document-new", K3Icon::SizeSmall ) );
  mBtnNew->setMinimumSize( mBtnNew->sizeHint() * 1.2 );
  mBtnCopy = new QPushButton( QString(), hb );
  mBtnCopy->setIcon( BarIconSet( "edit-copy", K3Icon::SizeSmall ) );
  mBtnCopy->setMinimumSize( mBtnCopy->sizeHint() * 1.2 );
  mBtnDelete = new QPushButton( QString(), hb );
  mBtnDelete->setIcon( BarIconSet( "edit-delete", K3Icon::SizeSmall ) );
  mBtnDelete->setMinimumSize( mBtnDelete->sizeHint() * 1.2 );
  mBtnRename = new QPushButton( i18n("Rename..."), hb );
  mBtnNew->setToolTip( i18n("New") );
  mBtnCopy->setToolTip( i18n("Copy") );
  mBtnDelete->setToolTip( i18n("Delete"));
  mBtnNew->setWhatsThis( i18n(_wt_filterlist_new) );
  mBtnCopy->setWhatsThis( i18n(_wt_filterlist_copy) );
  mBtnDelete->setWhatsThis( i18n(_wt_filterlist_delete) );
  mBtnRename->setWhatsThis( i18n(_wt_filterlist_rename) );

  layout->addWidget( hb );
  setLayout( layout );

  //----------- now connect everything
  connect( mListWidget, SIGNAL(currentRowChanged(int)),
	   this, SLOT(slotSelected(int)) );
  connect( mListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
           this, SLOT( slotRename()) );
  connect( mBtnUp, SIGNAL(clicked()),
	   this, SLOT(slotUp()) );
  connect( mBtnDown, SIGNAL(clicked()),
	   this, SLOT(slotDown()) );
  connect( mBtnNew, SIGNAL(clicked()),
	   this, SLOT(slotNew()) );
  connect( mBtnCopy, SIGNAL(clicked()),
	   this, SLOT(slotCopy()) );
  connect( mBtnDelete, SIGNAL(clicked()),
	   this, SLOT(slotDelete()) );
  connect( mBtnRename, SIGNAL(clicked()),
	   this, SLOT(slotRename()) );

  // the dialog should call loadFilterList()
  // when all signals are connected.
  enableControls();
}


KMFilterListBox::~KMFilterListBox()
{
  qDeleteAll( mFilterList );
}


void KMFilterListBox::createFilter( const QByteArray & field,
				    const QString & value )
{
  KMSearchRule *newRule = KMSearchRule::createInstance( field, KMSearchRule::FuncContains, value );

  KMFilter *newFilter = new KMFilter( bPopFilter );
  newFilter->pattern()->append( newRule );
  newFilter->pattern()->setName( QString("<%1>:%2").arg( QString::fromLatin1( field ) ).arg( value) );

  KMFilterActionDesc *desc = kmkernel->filterActionDict()->value( "transfer" );
  if ( desc )
    newFilter->actions()->append( desc->create() );

  insertFilter( newFilter );
  enableControls();
}

bool KMFilterListBox::showLaterMsgs()
{
  return mShowLater;
}

void KMFilterListBox::slotUpdateFilterName()
{
  if ( mIdxSelItem < 0 ) {
    kDebug(5006) << "KMFilterListBox::slotUpdateFilterName called while no filter is selected, ignoring. idx=" << mIdxSelItem << endl;
    return;
  }

  KMSearchPattern *p = mFilterList.at(mIdxSelItem)->pattern();
  if ( !p ) return;

  QString shouldBeName = p->name();
  QString displayedName = mListWidget->item( mIdxSelItem )->text();

  if ( shouldBeName.trimmed().isEmpty() ) {
    mFilterList.at(mIdxSelItem)->setAutoNaming( true );
  }

  if ( mFilterList.at(mIdxSelItem)->isAutoNaming() ) {
    // auto-naming of patterns
    if ( p->first() && !p->first()->field().trimmed().isEmpty() )
      shouldBeName = QString( "<%1>: %2" ).arg( QString::fromLatin1( p->first()->field() ) ).arg( p->first()->contents() );
    else
      shouldBeName = '<' + i18n("unnamed") + '>';
    p->setName( shouldBeName );
  }

  if ( displayedName == shouldBeName ) return;

  mListWidget->blockSignals(true);
  mListWidget->item( mIdxSelItem )->setText( shouldBeName );
  mListWidget->blockSignals(false);
}

void KMFilterListBox::slotShowLaterToggled(bool aOn)
{
  mShowLater = aOn;
}

void KMFilterListBox::slotApplyFilterChanges()
{
  if ( mIdxSelItem >= 0 )
    slotSelected( mListWidget->currentRow() );

  // by now all edit widgets should have written back
  // their widget's data into our filter list.

  KMFilterMgr *fm;
  if (bPopFilter)
    fm = kmkernel->popFilterMgr();
  else
    fm = kmkernel->filterMgr();

  QList<KMFilter*> newFilters;
  QStringList emptyFilters;
  QList<KMFilter*>::const_iterator it;
  for ( it = mFilterList.begin() ; it != mFilterList.end() ; ++it ) {
    KMFilter *f = new KMFilter( **it ); // deep copy
    f->purify();
    if ( !f->isEmpty() )
      // the filter is valid:
      newFilters.append( f );
    else {
      // the filter is invalid:
      emptyFilters << f->name();
      delete f;
    }
  }
  if (bPopFilter)
    fm->setShowLaterMsgs(mShowLater);

  // block attemts to use filters (currently a no-op)
  fm->beginUpdate();
  fm->setFilters( newFilters );
  if (fm->atLeastOneOnlineImapFolderTarget()) {
    QString str = i18n("At least one filter targets a folder on an online "
		       "IMAP account. Such filters will only be applied "
		       "when manually filtering and when filtering "
		       "incoming online IMAP mail.");
    KMessageBox::information( this, str, QString(),
			      "filterDlgOnlineImapCheck" );
  }
  // allow usage of the filters again.
  fm->endUpdate();
  fm->writeConfig();

  // report on invalid filters:
  if ( !emptyFilters.empty() ) {
    QString msg = i18n("The following filters have not been saved because they "
		       "were invalid (e.g. containing no actions or no search "
		       "rules).");
    KMessageBox::informationList( 0, msg, emptyFilters, QString(),
				  "ShowInvalidFilterWarning" );
  }
}

void KMFilterListBox::slotSelected( int aIdx )
{
  kDebug(5006) << "KMFilterListBox::slotSelected called. idx=" << aIdx << endl;
  mIdxSelItem = aIdx;

  if ( mIdxSelItem >= 0 && mIdxSelItem < mFilterList.count() ) {
    KMFilter *f = mFilterList.at(aIdx);
    if ( f )
      emit filterSelected( f );
    else
      emit resetWidgets();
  } else {
    emit resetWidgets();
  }
  enableControls();
}

void KMFilterListBox::slotNew()
{
  // just insert a new filter.
  insertFilter( new KMFilter( bPopFilter ) );
  enableControls();
}

void KMFilterListBox::slotCopy()
{
  if ( mIdxSelItem < 0 ) {
    kDebug(5006) << "KMFilterListBox::slotCopy called while no filter is selected, ignoring." << endl;
    return;
  }

  // make sure that all changes are written to the filter before we copy it
  emit applyWidgets();

  KMFilter *filter = mFilterList.at( mIdxSelItem );

  // enableControls should make sure this method is
  // never called when no filter is selected.
  assert( filter );

  // inserts a copy of the current filter.
  insertFilter( new KMFilter( *filter ) );
  enableControls();
}

void KMFilterListBox::slotDelete()
{
  if ( mIdxSelItem < 0 ) {
    kDebug(5006) << "KMFilterListBox::slotDelete called while no filter is selected, ignoring." << endl;
    return;
  }

  int oIdxSelItem = mIdxSelItem;
  mIdxSelItem = -1;
  // unselect all
  // TODO remove this line: mListWidget->clearSelection();
  // broadcast that all widgets let go
  // of the filter
  emit resetWidgets();

  // remove the filter from both the filter list...
  mFilterList.takeAt( oIdxSelItem );
  // and the listbox
  mListWidget->takeItem( oIdxSelItem );

  int count = mListWidget->count();
  // and set the new current item.
  if ( count > oIdxSelItem )
    // oIdxItem is still a valid index
    mListWidget->setCurrentRow( oIdxSelItem );
  else if ( count )
    // oIdxSelIdx is no longer valid, but the
    // list box isn't empty
    mListWidget->setCurrentRow( count - 1 );

  // work around a problem when deleting the first item in a QListWidget:
  // after takeItem, slotSelectionChanged is emitted with 1, but the row 0
  // remains selected and another selectCurrentRow(0) does not trigger the
  // selectionChanged signal
  // (qt-copy as of 2006-12-22 / gungl)
  if ( oIdxSelItem == 0 )
    slotSelected( 0 );

  mIdxSelItem = mListWidget->currentRow();
  enableControls();
}

void KMFilterListBox::slotUp()
{
  if ( mIdxSelItem < 0 ) {
    kDebug(5006) << "KMFilterListBox::slotUp called while no filter is selected, ignoring." << endl;
    return;
  }
  if ( mIdxSelItem == 0 ) {
    kDebug(5006) << "KMFilterListBox::slotUp called while the _topmost_ filter is selected, ignoring." << endl;
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem - 1 );
  enableControls();
}

void KMFilterListBox::slotDown()
{
  if ( mIdxSelItem < 0 ) {
    kDebug(5006) << "KMFilterListBox::slotDown called while no filter is selected, ignoring." << endl;
    return;
  }
  if ( mIdxSelItem == (int)mListWidget->count() - 1 ) {
    kDebug(5006) << "KMFilterListBox::slotDown called while the _last_ filter is selected, ignoring." << endl;
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem + 1);
  enableControls();
}

void KMFilterListBox::slotRename()
{
  if ( mIdxSelItem < 0 ) {
    kDebug(5006) << "KMFilterListBox::slotRename called while no filter is selected, ignoring." << endl;
    return;
  }

  bool okPressed = false;
  KMFilter *filter = mFilterList.at( mIdxSelItem );

  // enableControls should make sure this method is
  // never called when no filter is selected.
  assert( filter );

  // allow empty names - those will turn auto-naming on again
  QValidator *validator = new QRegExpValidator( QRegExp( ".*" ), 0 );
  QString newName = KInputDialog::getText
    (
     i18n("Rename Filter"),
     i18n("Rename filter \"%1\" to:\n(leave the field empty for automatic naming)",
          filter->pattern()->name() ) /*label*/,
     filter->pattern()->name() /* initial value */,
     &okPressed, topLevelWidget(), validator
     );
  delete validator;

  if ( !okPressed ) return;

  if ( newName.isEmpty() ) {
    // bait for slotUpdateFilterName to
    // use automatic naming again.
    filter->pattern()->setName( "<>" );
    filter->setAutoNaming( true );
  } else {
    filter->pattern()->setName( newName );
    filter->setAutoNaming( false );
  }

  slotUpdateFilterName();
}

void KMFilterListBox::enableControls()
{
  bool theFirst = ( mIdxSelItem == 0 );
  bool theLast = ( mIdxSelItem >= (int)mFilterList.count() - 1 );
  bool aFilterIsSelected = ( mIdxSelItem >= 0 );

  mBtnUp->setEnabled( aFilterIsSelected && !theFirst );
  mBtnDown->setEnabled( aFilterIsSelected && !theLast );
  mBtnCopy->setEnabled( aFilterIsSelected );
  mBtnDelete->setEnabled( aFilterIsSelected );
  mBtnRename->setEnabled( aFilterIsSelected );

  if ( aFilterIsSelected )
    mListWidget->scrollToItem( mListWidget->currentItem() );
}

void KMFilterListBox::loadFilterList( bool createDummyFilter )
{
  assert(mListWidget);
  setEnabled(false);
  // we don't want the insertion to
  // cause flicker in the edit widgets.
  blockSignals(true);

  // clear both lists
  mFilterList.clear();
  mListWidget->clear();

  const KMFilterMgr *manager = 0;
  if(bPopFilter)
  {
    mShowLater = kmkernel->popFilterMgr()->showLaterMsgs();
    manager = kmkernel->popFilterMgr();
  }
  else
  {
    manager = kmkernel->filterMgr();
  }
  Q_ASSERT( manager );

  QList<KMFilter*>::const_iterator it;
  for ( it = manager->filters().begin() ; it != manager->filters().end() ; ++it ) {
    mFilterList.append( new KMFilter( **it ) ); // deep copy
    mListWidget->addItem( (*it)->pattern()->name() );
  }

  blockSignals(false);
  setEnabled(true);

  // create an empty filter when there's none, to avoid a completely
  // disabled dialog (usability tests indicated that the new-filter
  // button is too hard to find that way):
  if ( !mListWidget->count() && createDummyFilter )
    slotNew();

  if ( mListWidget->count() > 0 )
    mListWidget->setCurrentRow( 0 );

  enableControls();
}

void KMFilterListBox::insertFilter( KMFilter* aFilter )
{
  // must be really a filter...
  assert( aFilter );

  // if mIdxSelItem < 0, QListBox::insertItem will append.
  mListWidget->insertItem( mIdxSelItem, aFilter->pattern()->name() );
  if ( mIdxSelItem < 0 ) {
    // none selected -> append
    mFilterList.append( aFilter );
    mListWidget->setCurrentRow( mListWidget->count() - 1 );
  } else {
    // insert just before selected
    mFilterList.insert( mIdxSelItem, aFilter );
    mListWidget->setCurrentRow( mIdxSelItem );
  }

}

void KMFilterListBox::swapNeighbouringFilters( int untouchedOne, int movedOne )
{
  // must be neighbours...
  assert( untouchedOne - movedOne == 1 || movedOne - untouchedOne == 1 );

  // untouchedOne is at idx. to move it down(up),
  // remove item at idx+(-)1 w/o deleting it.
  QListWidgetItem *item = mListWidget->item( movedOne );
  mListWidget->takeItem( movedOne );
  // now selected item is at idx(idx-1), so
  // insert the other item at idx, ie. above(below).
  mListWidget->insertItem( untouchedOne, item );

  KMFilter* filter = mFilterList.takeAt( movedOne );
  mFilterList.insert( untouchedOne, filter );

  mIdxSelItem += movedOne - untouchedOne;
}


//=============================================================================
//
// class KMFilterActionWidget
//
//=============================================================================

KMFilterActionWidget::KMFilterActionWidget( QWidget *parent, const char* name )
  : KHBox( parent )
{
  setObjectName( name );

  int i;

  mComboBox = new QComboBox( this );
  mComboBox->setEditable( false );
  assert( mComboBox );
  mWidgetStack = new QStackedWidget(this);
  assert( mWidgetStack );

  setSpacing( 4 );

  QList<KMFilterActionDesc*> list = kmkernel->filterActionDict()->list();
  QList<KMFilterActionDesc*>::const_iterator it;
  for ( i=0, it = list.begin() ; it != list.end() ; ++it, ++i ) {
    //create an instance:
    KMFilterAction *a = (*it)->create();
    // append to the list of actions:
    mActionList.append( a );
    // add parameter widget to widget stack:
    mWidgetStack->insertWidget( i, a->createParamWidget( mWidgetStack ) );
    // add (i18n-ized) name to combo box
    mComboBox->addItem( (*it)->label );
  }
  // widget for the case where no action is selected.
  mWidgetStack->insertWidget( i,new QLabel( i18n("Please select an action."), mWidgetStack ) );
  mWidgetStack->setCurrentIndex(i);
  mComboBox->addItem( " " );
  mComboBox->setCurrentIndex(i);

  // don't show scroll bars.
  mComboBox->setMaxCount( mComboBox->count() );
  // layout management:
  // o the combo box is not to be made larger than it's sizeHint(),
  //   the parameter widget should grow instead.
  // o the whole widget takes all space horizontally, but is fixed vertically.
  mComboBox->adjustSize();
  mComboBox->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );
  updateGeometry();

  // redirect focus to the filter action combo box
  setFocusProxy( mComboBox );

  // now connect the combo box and the widget stack
  connect( mComboBox, SIGNAL(activated(int)),
	   mWidgetStack, SLOT(setCurrentIndex (int)) );
}

KMFilterActionWidget::~KMFilterActionWidget()
{
  qDeleteAll( mActionList );
}

void KMFilterActionWidget::setAction( const KMFilterAction* aAction )
{
  int i=0;
  bool found = false;
  int count = mComboBox->count() - 1 ; // last entry is the empty one
  QString label = ( aAction ) ? aAction->label() : QString() ;

  // find the index of typeOf(aAction) in mComboBox
  // and clear the other widgets on the way.
  for ( ; i < count ; i++ )
    if ( aAction && mComboBox->itemText(i) == label ) {
      //...set the parameter widget to the settings
      // of aAction...
      aAction->setParamWidgetValue( mWidgetStack->widget(i) );
      //...and show the correct entry of
      // the combo box
      mComboBox->setCurrentIndex(i); // (mm) also raise the widget, but doesn't
      mWidgetStack->setCurrentIndex(i);
      found = true;
    } else // clear the parameter widget
      mActionList.at(i)->clearParamWidget( mWidgetStack->widget(i) );
  if ( found ) return;

  // not found, so set the empty widget
  mComboBox->setCurrentIndex( count ); // last item
  mWidgetStack->setCurrentIndex( count) ;
}

KMFilterAction * KMFilterActionWidget::action()
{
  // look up the action description via the label
  // returned by QComboBox::currentText()...
  KMFilterActionDesc *desc = kmkernel->filterActionDict()->value( mComboBox->currentText() );
  if ( desc ) {
    // ...create an instance...
    KMFilterAction *fa = desc->create();
    if ( fa ) {
      // ...and apply the setting of the parameter widget.
      fa->applyParamWidgetValue( mWidgetStack->currentWidget() );
      return fa;
    }
  }

  return 0;
}

//=============================================================================
//
// class KMFilterActionWidgetLister (the filter action editor)
//
//=============================================================================

KMFilterActionWidgetLister::KMFilterActionWidgetLister( QWidget *parent, const char* name )
  : KWidgetLister( 1, FILTER_MAX_ACTIONS, parent, name )
{
  mActionList = 0;
}

KMFilterActionWidgetLister::~KMFilterActionWidgetLister()
{
}

void KMFilterActionWidgetLister::setActionList( QList<KMFilterAction*> *aList )
{
  assert ( aList );

  if ( mActionList )
    regenerateActionListFromWidgets();

  mActionList = aList;

  ((QWidget*)parent())->setEnabled( true );

  if ( aList->count() == 0 ) {
    slotClear();
    return;
  }

  int superfluousItems = (int)mActionList->count() - mMaxWidgets ;
  if ( superfluousItems > 0 ) {
    kDebug(5006) << "KMFilterActionWidgetLister: Clipping action list to "
	      << mMaxWidgets << " items!" << endl;

    for ( ; superfluousItems ; superfluousItems-- )
      mActionList->removeLast();
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo( mActionList->count() );

  // load the actions into the widgets
  QList<KMFilterAction*>::const_iterator aIt;
  Q3PtrListIterator<QWidget> wIt( mWidgetList );
  for ( aIt = mActionList->begin(), wIt.toFirst() ;
        (aIt != mActionList->end()) && wIt.current() ; ++aIt, ++wIt )
    ((KMFilterActionWidget*)(*wIt))->setAction( (*aIt) );
}

void KMFilterActionWidgetLister::reset()
{
  if ( mActionList )
    regenerateActionListFromWidgets();

  mActionList = 0;
  slotClear();
  ((QWidget*)parent())->setEnabled( false );
}

QWidget* KMFilterActionWidgetLister::createWidget( QWidget *parent )
{
  return new KMFilterActionWidget(parent);
}

void KMFilterActionWidgetLister::clearWidget( QWidget *aWidget )
{
  if ( aWidget )
    ((KMFilterActionWidget*)aWidget)->setAction(0);
}

void KMFilterActionWidgetLister::regenerateActionListFromWidgets()
{
  if ( !mActionList ) return;

  mActionList->clear();

  Q3PtrListIterator<QWidget> it( mWidgetList );
  for ( it.toFirst() ; it.current() ; ++it ) {
    KMFilterAction *a = ((KMFilterActionWidget*)(*it))->action();
    if ( a )
      mActionList->append( a );
  }

}

//=============================================================================
//
// class KMPopFilterActionWidget
//
//=============================================================================

KMPopFilterActionWidget::KMPopFilterActionWidget( const QString& title, QWidget *parent, const char* name )
  : QGroupBox( title, parent )
{
  setObjectName( name );
  QVBoxLayout *layout = new QVBoxLayout( this );
  QButtonGroup *bg = new QButtonGroup( this );

  QRadioButton *downBtn = new QRadioButton( i18n("&Download mail"), this );
  QRadioButton *laterBtn = new QRadioButton( i18n("Download mail la&ter"), this );
  QRadioButton *deleteBtn = new QRadioButton( i18n("D&elete mail from server"), this );

  layout->addWidget( downBtn );
  layout->addWidget( laterBtn );
  layout->addWidget( deleteBtn );
  bg->addButton( downBtn );
  bg->addButton( laterBtn );
  bg->addButton( deleteBtn );

  mActionMap.insert( Down, downBtn );
  mActionMap.insert( Later, laterBtn );
  mActionMap.insert( Delete, deleteBtn );

  mButtonMap.insert( downBtn, Down );
  mButtonMap.insert( laterBtn, Later );
  mButtonMap.insert( deleteBtn, Delete );

  connect( bg, SIGNAL(buttonClicked(QAbstractButton*)),
	   this, SLOT(slotActionClicked(QAbstractButton*)) );
}

void KMPopFilterActionWidget::setAction( KMPopFilterAction aAction )
{
  if( aAction == NoAction)
  {
    aAction = Later;
  }

  mAction = aAction;

  blockSignals( true );
  if(!mActionMap[aAction]->isChecked())
  {
    mActionMap[aAction]->setChecked(true);
  }
  blockSignals( false );

  setEnabled(true);
}

KMPopFilterAction  KMPopFilterActionWidget::action()
{
  return mAction;
}

void KMPopFilterActionWidget::slotActionClicked( QAbstractButton *btn )
{
  emit actionChanged( mButtonMap[btn] );
  setAction( mButtonMap[btn] );
}

void KMPopFilterActionWidget::reset()
{
  blockSignals(true);
  mActionMap[Down]->setChecked( true );
  blockSignals(false);

  setEnabled( false );
}

#include "kmfilterdlg.moc"
