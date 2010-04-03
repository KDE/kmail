/*
  Filter Dialog
  Author: Marc Mutz <Marc@Mutz.com>
  based upon work by Stefan Taferner <taferner@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kmfilterdlg.h"

// other KMail headers:
#include "kmsearchpatternedit.h"
#include "kmfiltermgr.h"
#include "kmmainwidget.h"
#include "filterimporterexporter.h"
#include "util.h"
using KMail::FilterImporterExporter;

// KDEPIMLIBS headers
#include <Akonadi/AgentType>
#include <Akonadi/AgentInstance>

// other KDE headers:
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kwindowsystem.h>
#include <kconfig.h>
#include <kicondialog.h>
#include <kkeysequencewidget.h>
#include <kpushbutton.h>
#include <kconfiggroup.h>
#include <ktabwidget.h>
#include <kvbox.h>
#include <klistwidgetsearchline.h>

// Qt headers:
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QTreeWidget>

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
  bPopFilter(popFilter),
  mDoNotClose( false )
{
  if ( popFilter )
    setCaption( i18n("POP3 Filter Rules") );
  else
    setCaption( i18n("Filter Rules") );
  setButtons( Help|Ok|Apply|Cancel|User1|User2 );
  setModal( false );
  setButtonFocus( Ok );
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap(IconSize(KIconLoader::Desktop),IconSize(KIconLoader::Desktop)), qApp->windowIcon().pixmap(IconSize(KIconLoader::Small),IconSize(KIconLoader::Small)) );
  setHelp( (bPopFilter)? KMPopFilterDlgHelpAnchor: KMFilterDlgHelpAnchor );
  setButtonText( User1, i18n("Import...") );
  setButtonText( User2, i18n("Export...") );
  connect( this, SIGNAL(user1Clicked()),
           this, SLOT( slotImportFilters()) );
  connect( this, SIGNAL(user2Clicked()),
           this, SLOT( slotExportFilters()) );

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
    KTabWidget *tabWidget = new KTabWidget( w );
    tabWidget->setObjectName( "kmfd_tab" );
    topLayout->addWidget( tabWidget );

    page1 = new QWidget( tabWidget );
    tabWidget->addTab( page1, i18nc("General mail filter settings.", "General") );
    hbl = new QHBoxLayout( page1 );
    hbl->setObjectName( "kmfd_hbl" );
    hbl->setSpacing( spacingHint() );
    hbl->setMargin( marginHint() );

    page2 = new QWidget( tabWidget );
    tabWidget->addTab( page2, i18nc("Advanced mail filter settings.","Advanced") );
    vbl2 = new QVBoxLayout( page2 );
    vbl2->setObjectName( "kmfd_vbl2" );
    vbl2->setSpacing( spacingHint() );
    vbl2->setMargin( marginHint() );
  }

  QVBoxLayout *vbl = new QVBoxLayout();
  hbl->addLayout( vbl );
  vbl->setObjectName( "kmfd_vbl" );
  vbl->setSpacing( spacingHint() );
  hbl->setStretchFactor( vbl, 2 );

  mPatternEdit = new KMSearchPatternEdit( i18n("Filter Criteria"),
                                          bPopFilter ? w : page1, bPopFilter );
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

      mAccountList = new QTreeWidget( mAdvOptsGroup );
      mAccountList->setObjectName( "accountList" );
      mAccountList->setColumnCount( 2 );
      QStringList headerNames;
      headerNames << i18n("Account Name") << i18n("Type");
      mAccountList->setHeaderItem( new QTreeWidgetItem( headerNames ) );
      mAccountList->setAllColumnsShowFocus( true );
      mAccountList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
      mAccountList->setSortingEnabled( false );
      mAccountList->setRootIsDecorated( false );
      gl->addWidget( mAccountList, 0, 1, 4, 3 );

      mApplyBeforeOut = new QCheckBox( i18n("Apply this filter &before sending messages"), mAdvOptsGroup );
      mApplyBeforeOut->setToolTip( i18n( "<p>The filter will be triggered <b>before</b> the message is sent and it will affect both the local copy and the sent copy of the message.</p>"
            "<p>This is required if the recipient's copy also needs to be modified.</p>" ) );
      gl->addWidget( mApplyBeforeOut, 5, 0, 1, 4 );

      mApplyOnOut = new QCheckBox( i18n("Apply this filter to &sent messages"), mAdvOptsGroup );
      mApplyOnOut->setToolTip( i18n( "<p>The filter will be triggered <b>after</b> the message is sent and it will only affect the local copy of the message.</p>"
            "<p>If the recipient's copy also needs to be modified, please use \"Apply this filter <b>before</b> sending messages\".</p>" ) );
      gl->addWidget( mApplyOnOut, 4, 0, 1, 4 );

      mApplyOnCtrlJ = new QCheckBox( i18n("Apply this filter on manual &filtering"), mAdvOptsGroup );
      gl->addWidget( mApplyOnCtrlJ, 6, 0, 1, 4 );

      mStopProcessingHere = new QCheckBox( i18n("If this filter &matches, stop processing here"), mAdvOptsGroup );
      gl->addWidget( mStopProcessingHere, 7, 0, 1, 4 );
      mConfigureShortcut = new QCheckBox( i18n("Add this filter to the Apply Filter menu"), mAdvOptsGroup );
      gl->addWidget( mConfigureShortcut, 8, 0, 1, 2 );
      QLabel *keyButtonLabel = new QLabel( i18n( "Shortcut:" ), mAdvOptsGroup );
      keyButtonLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
      gl->addWidget( keyButtonLabel, 8, 2, 1, 1);
      mKeySeqWidget = new KKeySequenceWidget( mAdvOptsGroup );
      mKeySeqWidget->setObjectName( "FilterShortcutSelector" );
      gl->addWidget( mKeySeqWidget, 8, 3, 1, 1);
      mKeySeqWidget->setEnabled( false );
      mKeySeqWidget->setModifierlessAllowed( true );
      mKeySeqWidget->setCheckActionCollections(
                             kmkernel->getKMMainWidget()->actionCollections() );
      mConfigureToolbar = new QCheckBox( i18n("Additionally add this filter to the toolbar"), mAdvOptsGroup );
      gl->addWidget( mConfigureToolbar, 9, 0, 1, 4 );
      mConfigureToolbar->setEnabled( false );

      KHBox *hbox = new KHBox( mAdvOptsGroup );
      mFilterActionLabel = new QLabel( i18n( "Icon for this filter:" ),
                                       hbox );
      mFilterActionLabel->setEnabled( false );

      mFilterActionIconButton = new KIconButton( hbox );
      mFilterActionLabel->setBuddy( mFilterActionIconButton );
      mFilterActionIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Action, false );
      mFilterActionIconButton->setIconSize( 16 );
      mFilterActionIconButton->setIcon( "system-run" );
      mFilterActionIconButton->setEnabled( false );

      gl->addWidget( hbox, 10, 0, 1, 4 );

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
    connect( mApplyBeforeOut, SIGNAL(clicked()),
             this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnOut, SIGNAL(clicked()),
             this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnCtrlJ, SIGNAL(clicked()),
             this, SLOT(slotApplicabilityChanged()) );
    connect( mAccountList, SIGNAL( itemChanged(QTreeWidgetItem *,int) ),
             this, SLOT( slotApplicableAccountsChanged() ) );

    // transfer changes from the 'stop processing here'
    // check box to the filter
    connect( mStopProcessingHere, SIGNAL(toggled(bool)),
             this, SLOT(slotStopProcessingButtonToggled(bool)) );

    connect( mConfigureShortcut, SIGNAL(toggled(bool)),
             this, SLOT(slotConfigureShortcutButtonToggled(bool)) );

    connect( mKeySeqWidget, SIGNAL( keySequenceChanged( const QKeySequence& ) ),
             this, SLOT( slotShortcutChanged( const QKeySequence& ) ) );

    connect( mConfigureToolbar, SIGNAL(toggled(bool)),
             this, SLOT(slotConfigureToolbarButtonToggled(bool)) );

    connect( mFilterActionIconButton, SIGNAL( iconChanged( const QString& ) ),
             this, SLOT( slotFilterActionIconChanged( const QString& ) ) );
  }

  // reset all widgets here
  connect( mFilterList, SIGNAL(resetWidgets()),
           this, SLOT(slotReset()) );

  connect( mFilterList, SIGNAL( applyWidgets() ),
           this, SLOT( slotUpdateFilter() ) );

  // support auto-naming the filter
  connect( mPatternEdit, SIGNAL(maybeNameChanged()),
           mFilterList, SLOT(slotUpdateFilterName()) );

  // save filters on 'Apply' or 'OK'
  connect( this, SIGNAL( buttonClicked( KDialog::ButtonCode ) ),
           mFilterList, SLOT( slotApplyFilterChanges( KDialog::ButtonCode ) ) );

  // save dialog size on 'OK'
  connect( this, SIGNAL(okClicked()),
           this, SLOT(slotSaveSize()) );

  // destruct the dialog on close and Cancel
  connect( this, SIGNAL( closeClicked() ),
           this, SLOT( slotFinished() ) );
  connect( this, SIGNAL( cancelClicked() ),
           this, SLOT( slotFinished() ) );

  // disable closing when user wants to continue editing
  connect( mFilterList, SIGNAL( abortClosing() ),
           this, SLOT( slotDisableAccept() ) );

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

void KMFilterDlg::accept()
{
  if ( mDoNotClose ) {
    mDoNotClose = false; // only abort current close attempt
  } else {
    KDialog::accept();
    slotFinished();
  }
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
    kDebug() << "apply on inbound ==" << aFilter->applyOnInbound();
    kDebug() << "apply on outbound ==" << aFilter->applyOnOutbound();
    kDebug() << "apply before outbound == " << aFilter->applyBeforeOutbound();
    kDebug() << "apply on explicit ==" << aFilter->applyOnExplicit();

    // NOTE: setting these values activates the slot that sets them in
    // the filter! So make sure we have the correct values _before_ we
    // set the first one:
    const bool applyOnIn = aFilter->applyOnInbound();
    const bool applyOnForAll = aFilter->applicability() == KMFilter::All;
    const bool applyOnTraditional = aFilter->applicability() == KMFilter::ButImap;
    const bool applyBeforeOut = aFilter->applyBeforeOutbound();
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
    mApplyBeforeOut->setChecked( applyBeforeOut );
    mApplyOnOut->setChecked( applyOnOut );
    mApplyOnCtrlJ->setChecked( applyOnExplicit );
    mStopProcessingHere->setChecked( stopHere );
    mConfigureShortcut->setChecked( configureShortcut );
    mKeySeqWidget->setKeySequence( shortcut.primary(),
                                   KKeySequenceWidget::NoValidate );
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
    mFilter->setApplyBeforeOutbound( mApplyBeforeOut->isChecked() );
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
    QTreeWidgetItemIterator it( mAccountList );
    while( QTreeWidgetItem * item = *it ) {
      QString id = item->text( 2 );
      item->setCheckState( 0, mFilter->applyOnAccount( id ) ? Qt::Checked :
                                                              Qt::Unchecked );
      ++it;
    }

    kDebug() << "Setting filter to be applied at"
                 << ( mFilter->applyOnInbound() ? "incoming " : "" )
                 << ( mFilter->applyOnOutbound() ? "outgoing " : "" )
                 << ( mFilter->applyBeforeOutbound() ? "before_outgoing " : "" )
                 << ( mFilter->applyOnExplicit() ? "explicit CTRL-J" : "" );
  }
}

void KMFilterDlg::slotApplicableAccountsChanged()
{
  // Advanced tab functionality - Update list of accounts this filter applies to
  if ( mFilter && mApplyOnForChecked->isEnabled() && mApplyOnForChecked->isChecked() ) {

    QTreeWidgetItemIterator it( mAccountList );

    while( QTreeWidgetItem *item = *it ) {
      QString id = item->text( 2 );
      mFilter->setApplyOnAccount( id, item->checkState( 0 ) == Qt::Checked );
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
    mKeySeqWidget->setEnabled( aChecked );
    mConfigureToolbar->setEnabled( aChecked );
    mFilterActionIconButton->setEnabled( aChecked );
    mFilterActionLabel->setEnabled( aChecked );
  }
}

void KMFilterDlg::slotShortcutChanged( const QKeySequence &newSeq )
{
  if ( mFilter ) {
    mKeySeqWidget->applyStealShortcut();
    mFilter->setShortcut( KShortcut( newSeq ) );
  }
}

void KMFilterDlg::slotConfigureToolbarButtonToggled( bool aChecked )
{
  if ( mFilter )
    mFilter->setConfigureToolbar( aChecked );
}

void KMFilterDlg::slotFilterActionIconChanged( const QString &icon )
{
  if ( mFilter )
    mFilter->setIcon( icon );
}

void KMFilterDlg::slotUpdateAccountList()
{
  mAccountList->clear();

  QTreeWidgetItem *top = 0;
  // Block the signals here, otherwise we end up calling
  // slotApplicableAccountsChanged(), which will read the incomplete item
  // state and write that back to the filter
  mAccountList->blockSignals( true );
  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  for ( int i = 0; i <lst.count(); ++i ) {
    QTreeWidgetItem *listItem = new QTreeWidgetItem( mAccountList, top );
    listItem->setText( 0, lst.at( i ).name() );
    listItem->setText( 1, lst.at( i ).type().name() );
    listItem->setText( 2, QString( "%1" ).arg( lst.at( i ).identifier() ) );
    if ( mFilter )
      listItem->setCheckState( 0, mFilter->applyOnAccount( lst.at( i ).identifier() ) ?
                                  Qt::Checked : Qt::Unchecked );
    top = listItem;
  }
  mAccountList->blockSignals( false );

  // make sure our hidden column is really hidden (Qt tends to re-show it)
  mAccountList->hideColumn( 2 );
  mAccountList->resizeColumnToContents( 0 );
  mAccountList->resizeColumnToContents( 1 );

  top = mAccountList->topLevelItem( 0 );
  if ( top ) {
    mAccountList->setCurrentItem( top );
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

  KListWidgetSearchLine* mSearchListWidget = new KListWidgetSearchLine( this, mListWidget );
  mSearchListWidget->setClickMessage( i18nc( "@info/plain Displayed grayed-out inside the "
                                             "textbox, verb to search", "Search" ) );

  layout->addWidget( mSearchListWidget );
  layout->addWidget( mListWidget );

  //----------- the first row of buttons
  KHBox *hb = new KHBox(this);
  hb->setSpacing(4);
  mBtnUp = new KPushButton( QString(), hb );
  mBtnUp->setAutoRepeat( true );
  mBtnUp->setIcon( KIcon( "go-up" ) );
  mBtnUp->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtnUp->setMinimumSize( mBtnUp->sizeHint() * 1.2 );
  mBtnDown = new KPushButton( QString(), hb );
  mBtnDown->setAutoRepeat( true );
  mBtnDown->setIcon( KIcon( "go-down" ) );
  mBtnDown->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtnDown->setMinimumSize( mBtnDown->sizeHint() * 1.2 );
  mBtnUp->setToolTip( i18nc("Move selected filter up.", "Up") );
  mBtnDown->setToolTip( i18nc("Move selected filter down.", "Down") );
  mBtnUp->setWhatsThis( i18n(_wt_filterlist_up) );
  mBtnDown->setWhatsThis( i18n(_wt_filterlist_down) );

  layout->addWidget( hb );

  //----------- the second row of buttons
  hb = new KHBox(this);
  hb->setSpacing(4);
  mBtnNew = new QPushButton( QString(), hb );
  mBtnNew->setIcon( KIcon( "document-new" ) );
  mBtnNew->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtnNew->setMinimumSize( mBtnNew->sizeHint() * 1.2 );
  mBtnCopy = new QPushButton( QString(), hb );
  mBtnCopy->setIcon( KIcon( "edit-copy" ) );
  mBtnCopy->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtnCopy->setMinimumSize( mBtnCopy->sizeHint() * 1.2 );
  mBtnDelete = new QPushButton( QString(), hb );
  mBtnDelete->setIcon( KIcon( "edit-delete" ) );
  mBtnDelete->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtnDelete->setMinimumSize( mBtnDelete->sizeHint() * 1.2 );
  mBtnRename = new QPushButton( i18n("Rename..."), hb );
  mBtnNew->setToolTip( i18nc("@action:button in filter list manipulator", "New") );
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
    kDebug() << "Called while no filter is selected, ignoring. idx=" << mIdxSelItem;
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
    if ( !p->isEmpty() && p->first() && !p->first()->field().trimmed().isEmpty() )
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

void KMFilterListBox::slotApplyFilterChanges( KDialog::ButtonCode button )
{
  bool closeAfterSaving;
  if ( button == KDialog::Ok )
    closeAfterSaving = true;
  else if ( button == KDialog::Apply )
    closeAfterSaving = false;
  else
    return; // ignore close and cancel

  if ( mIdxSelItem >= 0 ) {
    emit applyWidgets();
    slotSelected( mListWidget->currentRow() );
  }

  // by now all edit widgets should have written back
  // their widget's data into our filter list.

  KMFilterMgr *fm;
  if ( bPopFilter )
    fm = kmkernel->popFilterMgr();
  else
    fm = kmkernel->filterMgr();

  QList<KMFilter *> newFilters = filtersForSaving( closeAfterSaving );

  if ( bPopFilter )
    fm->setShowLaterMsgs( mShowLater );

  fm->setFilters( newFilters );
  if ( fm->atLeastOneOnlineImapFolderTarget() ) {
    QString str = i18n("At least one filter targets a folder on an online "
                       "IMAP account. Such filters will only be applied "
                       "when manually filtering and when filtering "
                       "incoming online IMAP mail.");
    KMessageBox::information( this, str, QString(), "filterDlgOnlineImapCheck" );
  }
}

QList<KMFilter *> KMFilterListBox::filtersForSaving( bool closeAfterSaving ) const
{
  const_cast<KMFilterListBox*>( this )->applyWidgets(); // signals aren't const
  QList<KMFilter *> filters;
  QStringList emptyFilters;
  foreach ( KMFilter *const it, mFilterList ) {
    KMFilter *f = new KMFilter( *it ); // deep copy
    f->purify();
    if ( !f->isEmpty() )
      // the filter is valid:
      filters.append( f );
    else {
      // the filter is invalid:
      emptyFilters << f->name();
      delete f;
    }
  }

  // report on invalid filters:
  if ( !emptyFilters.empty() ) {
    if ( closeAfterSaving ) {
      // Ok clicked. Give option to continue editing
      int response = KMessageBox::warningContinueCancelList(
        0,
        i18n( "The following filters are invalid (e.g. containing no actions "
              "or no search rules). Discard or edit invalid filters?" ),
        emptyFilters,
        QString(),
        KGuiItem( i18n( "Discard" ) ),
        KStandardGuiItem::cancel(),
        "ShowInvalidFilterWarning" );
      if ( response == KMessageBox::Cancel )
        emit abortClosing();
    } else {
      // Apply clicked. Just warn.
      KMessageBox::informationList(
        0,
        i18n( "The following filters have not been saved because they were invalid "
              "(e.g. containing no actions or no search rules)." ),
        emptyFilters,
        QString(),
        "ShowInvalidFilterWarning" );
    }
  }
  return filters;
}

void KMFilterListBox::slotSelected( int aIdx )
{
  kDebug() << "idx=" << aIdx;
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
    kDebug() << "Called while no filter is selected, ignoring.";
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
    kDebug() << "Called while no filter is selected, ignoring.";
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
    kDebug() << "Called while no filter is selected, ignoring.";
    return;
  }
  if ( mIdxSelItem == 0 ) {
    kDebug() << "Called while the _topmost_ filter is selected, ignoring.";
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem - 1 );
  enableControls();
}

void KMFilterListBox::slotDown()
{
  if ( mIdxSelItem < 0 ) {
    kDebug() << "Called while no filter is selected, ignoring.";
    return;
  }
  if ( mIdxSelItem == (int)mListWidget->count() - 1 ) {
    kDebug() << "Called while the _last_ filter is selected, ignoring.";
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem + 1);
  enableControls();
}

void KMFilterListBox::slotRename()
{
  if ( mIdxSelItem < 0 ) {
    kDebug() << "Called while no filter is selected, ignoring.";
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
  emit resetWidgets();
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
  for ( it = manager->filters().constBegin() ;
        it != manager->filters().constEnd();
        ++it ) {
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

void KMFilterListBox::appendFilter( KMFilter* aFilter )
{
  mFilterList.append( aFilter );
  mListWidget->addItems( QStringList( aFilter->pattern()->name() ) );
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

  QWidget *w = new QWidget( this );
  gl = new QGridLayout( w );
  gl->setContentsMargins( 0, 0, 0, 0 );
  mComboBox = new KComboBox( gl );
  mComboBox->setEditable( false );
  assert( mComboBox );
  gl->addWidget( mComboBox, 1, 1 );

  setSpacing( 4 );

  QList<KMFilterActionDesc*> list = kmkernel->filterActionDict()->list();
  QList<KMFilterActionDesc*>::const_iterator it;
  for ( i=0, it = list.constBegin() ; it != list.constEnd() ; ++it, ++i ) {
    //create an instance:
    KMFilterAction *a = (*it)->create();
    // append to the list of actions:
    mActionList.append( a );
    // add (i18n-ized) name to combo box
    mComboBox->addItem( (*it)->label );
  }
  // widget for the case where no action is selected.
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
  connect( mComboBox, SIGNAL( activated( int ) ),
          this, SLOT( slotFilterTypeChanged( int ) ) );

  setFilterAction();
}

void KMFilterActionWidget::setFilterAction( QWidget* w )
{
  if ( gl->itemAtPosition( 1, 2 ) )
    delete gl->itemAtPosition( 1, 2 )->widget();

  if ( w )
    gl->addWidget( w, 1, 2/*, Qt::AlignTop*/ );
  else
    gl->addWidget( new QLabel( i18n( "Please select an action." ), this ), 1, 2 );
}

void KMFilterActionWidget::slotFilterTypeChanged( int newIdx )
{
  setFilterAction( newIdx < mActionList.count() ? mActionList.at( newIdx )->createParamWidget( this ) : 0 );
}

KMFilterActionWidget::~KMFilterActionWidget()
{
  qDeleteAll( mActionList );
}

void KMFilterActionWidget::setAction( const KMFilterAction* aAction )
{
  bool found = false;
  int count = mComboBox->count() - 1 ; // last entry is the empty one
  QString label = ( aAction ) ? aAction->label() : QString() ;

  // find the index of typeOf(aAction) in mComboBox
  // and clear the other widgets on the way.
  for ( int i = 0; i < count ; i++ ) {
    if ( aAction && mComboBox->itemText(i) == label ) {
      setFilterAction( mActionList.at( i )->createParamWidget( this ) );

      //...set the parameter widget to the settings
      // of aAction...
      aAction->setParamWidgetValue( gl->itemAtPosition( 1, 2 )->widget() );
      //...and show the correct entry of
      // the combo box
      mComboBox->setCurrentIndex(i); // (mm) also raise the widget, but doesn't
      found = true;
    }
  }
  if ( found ) return;

  // not found, so set the empty widget
  setFilterAction();

  mComboBox->setCurrentIndex( count ); // last item
}

KMFilterAction * KMFilterActionWidget::action() const
{
  // look up the action description via the label
  // returned by KComboBox::currentText()...
  KMFilterActionDesc *desc = kmkernel->filterActionDict()->value( mComboBox->currentText() );
  if ( desc ) {
    // ...create an instance...
    KMFilterAction *fa = desc->create();
    if ( fa ) {
      // ...and apply the setting of the parameter widget.
      fa->applyParamWidgetValue( gl->itemAtPosition( 1, 2 )->widget() );
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

KMFilterActionWidgetLister::KMFilterActionWidgetLister( QWidget *parent, const char* )
  : KWidgetLister( 1, FILTER_MAX_ACTIONS, parent )
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

  int superfluousItems = (int)mActionList->count() - widgetsMaximum();
  if ( superfluousItems > 0 ) {
    kDebug() << "KMFilterActionWidgetLister: Clipping action list to"
	      << widgetsMaximum() << "items!";

    for ( ; superfluousItems ; superfluousItems-- )
      mActionList->removeLast();
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo( mActionList->count() );

  // load the actions into the widgets
  QList<QWidget*> widgetList = widgets();
  QList<KMFilterAction*>::const_iterator aIt;
  QList<QWidget*>::ConstIterator wIt = widgetList.constBegin();
  for ( aIt = mActionList->constBegin();
        ( aIt != mActionList->constEnd() && wIt != widgetList.constEnd() );
        ++aIt, ++wIt )
    qobject_cast<KMFilterActionWidget*>( *wIt )->setAction( ( *aIt ) );
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

  foreach ( const QWidget* w, widgets() ) {
    KMFilterAction *a = qobject_cast<const KMFilterActionWidget*>( w )->action();
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

void KMFilterDlg::slotImportFilters()
{
  FilterImporterExporter importer( this, bPopFilter );
  QList<KMFilter *> filters = importer.importFilters();

  // FIXME message box how many were imported?
  if ( filters.isEmpty() ) return;

  QList<KMFilter*>::ConstIterator it;

  for ( it = filters.constBegin() ; it != filters.constEnd() ; ++it ) {
    mFilterList->appendFilter( *it ); // no need to deep copy, ownership passes to the list
  }
}

void KMFilterDlg::slotExportFilters()
{
  FilterImporterExporter exporter( this, bPopFilter );
  QList<KMFilter *> filters = mFilterList->filtersForSaving( false );
  exporter.exportFilters( filters );
  QList<KMFilter *>::ConstIterator it;
  for ( it = filters.constBegin(); it != filters.constEnd(); ++it )
    delete *it;
}

void KMFilterDlg::slotDisableAccept()
{
  mDoNotClose = true;
}

#include "kmfilterdlg.moc"
