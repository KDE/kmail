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

// other KDE headers:
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kwin.h>
#include <kconfig.h>
#include <kicondialog.h>

// other Qt headers:
#include <qlayout.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qwidgetstack.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qhbox.h>
#include <qvalidator.h>

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
	   "\"<<\".</p>"
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

KMFilterDlg::KMFilterDlg(QWidget* parent, const char* name, bool popFilter)
  : KDialogBase( parent, name, FALSE /* modality */,
		 (popFilter)? i18n("POP3 Filter Rules"): i18n("Filter Rules") /* caption*/,
		 Help|Ok|Apply|Cancel /* button mask */,
		 Ok /* default btn */, FALSE /* separator */),
  bPopFilter(popFilter)
{
  KWin::setIcons( winId(), kapp->icon(), kapp->miniIcon() );
  setHelp( (bPopFilter)? KMPopFilterDlgHelpAnchor: KMFilterDlgHelpAnchor );

  QWidget *w = new QWidget(this);
  setMainWidget(w);
  QHBoxLayout *hbl = new QHBoxLayout( w, 0, spacingHint(), "kmfd_hbl" );

  mFilterList = new KMFilterListBox( i18n("Available Filters"), w, 0, bPopFilter);
  hbl->addWidget( mFilterList, 1 /*stretch*/ );

  QVBoxLayout *vbl = new QVBoxLayout( hbl, spacingHint(), "kmfd_vbl" );
  hbl->setStretchFactor( vbl, 2 );

  mPatternEdit = new KMSearchPatternEdit( i18n("Filter Criteria"), w , "spe", bPopFilter);
  vbl->addWidget( mPatternEdit, 0, Qt::AlignTop );

  if(bPopFilter){
    mActionGroup = new KMPopFilterActionWidget( i18n("Filter Action"), w );
    vbl->addWidget( mActionGroup, 0, Qt::AlignTop );

    mGlobalsBox = new QVGroupBox(i18n("Global Options"), w);
    mShowLaterBtn = new QCheckBox(i18n("Always &show matched 'Download Later' messages in confirmation dialog"), mGlobalsBox);
    QWhatsThis::add( mShowLaterBtn, i18n(_wt_filterdlg_showLater) );
    vbl->addWidget( mGlobalsBox, 0, Qt::AlignTop );
  }
  else {
    QGroupBox *agb = new QGroupBox( 1 /*column*/, Vertical, i18n("Filter Actions"), w );
    mActionLister = new KMFilterActionWidgetLister( agb );
    vbl->addWidget( agb, 0, Qt::AlignTop );

    mAdvOptsGroup = new QGroupBox ( 1 /*columns*/, Vertical,
				    i18n("Advanced Options"), w);
    {
      QWidget *adv_w = new QWidget( mAdvOptsGroup );
      QGridLayout *gl = new QGridLayout( adv_w, 4 /*rows*/, 4 /*cols*/,
				         0 /*border*/, spacingHint() );
      gl->setColStretch( 0, 1 );
      QLabel *l = new QLabel( i18n("Apply this filter"), adv_w );
      gl->addWidget( l, 0, 0, AlignLeft );
      mApplyOnIn = new QCheckBox( i18n("to &incoming messages"), adv_w );
      gl->addWidget( mApplyOnIn, 0, 1 );
      mApplyOnOut = new QCheckBox( i18n("to &sent messages"), adv_w );
      gl->addWidget( mApplyOnOut, 0, 2 );
      mApplyOnCtrlJ = new QCheckBox( i18n("on manual &filtering"), adv_w );
      gl->addWidget( mApplyOnCtrlJ, 0, 3 );
      mStopProcessingHere = new QCheckBox( i18n("If this filter &matches, stop processing here"), adv_w );
      gl->addMultiCellWidget( mStopProcessingHere, //1, 0, Qt::AlignLeft );
			      1, 1, /*from to row*/
  			      0, 3 /*from to col*/ );
      mConfigureShortcut = new QCheckBox( i18n("Add this filter to the Apply Filter menu"), adv_w );
      gl->addMultiCellWidget( mConfigureShortcut, 2, 2, 0, 3 );
      mConfigureToolbar = new QCheckBox( i18n("Additionally add this filter to the toolbar"), adv_w );
      gl->addMultiCellWidget( mConfigureToolbar, 3, 3, 1, 3 );

      QHBox *hbox = new QHBox( adv_w );
      mFilterActionLabel = new QLabel( i18n( "Icon for this filter:" ),
                                       hbox );
      mFilterActionLabel->setEnabled( false );

      mFilterActionIconButton = new KIconButton( hbox );
      mFilterActionLabel->setBuddy( mFilterActionIconButton );
      mFilterActionIconButton->setIconType( KIcon::NoGroup, KIcon::Any, true );
      mFilterActionIconButton->setIconSize( 16 );
      mFilterActionIconButton->setIcon( "gear" );
      mFilterActionIconButton->setEnabled( false );

      gl->addMultiCellWidget( hbox, 4, 4, 1, 3 );
    }
    vbl->addWidget( mAdvOptsGroup, 0, Qt::AlignTop );
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
    connect( mApplyOnOut, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );
    connect( mApplyOnCtrlJ, SIGNAL(clicked()),
  	     this, SLOT(slotApplicabilityChanged()) );

    // transfer changes from the 'stop processing here'
    // check box to the filter
    connect( mStopProcessingHere, SIGNAL(toggled(bool)),
	     this, SLOT(slotStopProcessingButtonToggled(bool)) );

    connect( mConfigureShortcut, SIGNAL(toggled(bool)),
	     this, SLOT(slotConfigureShortcutButtonToggled(bool)) );

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
    resize( geometry.readSizeEntry( configKey ) );
  else
    adjustSize();

  // load the filter list (emits filterSelected())
  mFilterList->loadFilterList();
}

void KMFilterDlg::slotFinished() {
	delayedDestruct();
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
    kdDebug(5006) << "apply on inbound == "
		  << aFilter->applyOnInbound() << endl;
    kdDebug(5006) << "apply on outbound == "
		  << aFilter->applyOnOutbound() << endl;
    kdDebug(5006) << "apply on explicit == "
		  << aFilter->applyOnExplicit() << endl;

    // NOTE: setting these values activates the slot that sets them in
    // the filter! So make sure we have the correct values _before_ we
    // set the first one:
    const bool applyOnIn = aFilter->applyOnInbound();
    const bool applyOnOut = aFilter->applyOnOutbound();
    const bool applyOnExplicit = aFilter->applyOnExplicit();
    const bool stopHere = aFilter->stopProcessingHere();
    const bool configureShortcut = aFilter->configureShortcut();
    const bool configureToolbar = aFilter->configureToolbar();
    const QString icon = aFilter->icon();

    mApplyOnIn->setChecked( applyOnIn );
    mApplyOnOut->setChecked( applyOnOut );
    mApplyOnCtrlJ->setChecked( applyOnExplicit );
    mStopProcessingHere->setChecked( stopHere );
    mConfigureShortcut->setChecked( configureShortcut );
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
  if ( !mFilter )
    return;

  mFilter->setApplyOnInbound( mApplyOnIn->isChecked() );
  mFilter->setApplyOnOutbound( mApplyOnOut->isChecked() );
  mFilter->setApplyOnExplicit( mApplyOnCtrlJ->isChecked() );
  kdDebug(5006) << "KMFilterDlg: setting filter to be applied at "
		<< ( mFilter->applyOnInbound() ? "incoming " : "" )
		<< ( mFilter->applyOnOutbound() ? "outgoing " : "" )
		<< ( mFilter->applyOnExplicit() ? "explicit CTRL-J" : "" )
		<< endl;
}

void KMFilterDlg::slotStopProcessingButtonToggled( bool aChecked )
{
  if ( !mFilter )
    return;

  mFilter->setStopProcessingHere( aChecked );
}

void KMFilterDlg::slotConfigureShortcutButtonToggled( bool aChecked )
{
  if ( !mFilter )
    return;

  mFilter->setConfigureShortcut( aChecked );
  mConfigureToolbar->setEnabled( aChecked );
  mFilterActionIconButton->setEnabled( aChecked );
  mFilterActionLabel->setEnabled( aChecked );
}

void KMFilterDlg::slotConfigureToolbarButtonToggled( bool aChecked )
{
  if ( !mFilter )
    return;

  mFilter->setConfigureToolbar( aChecked );
}

void KMFilterDlg::slotFilterActionIconChanged( QString icon )
{
  if ( !mFilter )
    return;

  mFilter->setIcon( icon );
}

//=============================================================================
//
// class KMFilterListBox (the filter list manipulator)
//
//=============================================================================

KMFilterListBox::KMFilterListBox( const QString & title, QWidget *parent, const char* name, bool popFilter )
  : QGroupBox( 1, Horizontal, title, parent, name ),
    bPopFilter(popFilter)
{
  mFilterList.setAutoDelete(TRUE);
  mIdxSelItem = -1;

  //----------- the list box
  mListBox = new QListBox(this);
  mListBox->setMinimumWidth(150);
  QWhatsThis::add( mListBox, i18n(_wt_filterlist) );

  //----------- the first row of buttons
  QHBox *hb = new QHBox(this);
  hb->setSpacing(4);
  mBtnUp = new QPushButton( QString::null, hb );
  mBtnUp->setAutoRepeat( true );
  mBtnUp->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
  mBtnUp->setMinimumSize( mBtnUp->sizeHint() * 1.2 );
  mBtnDown = new QPushButton( QString::null, hb );
  mBtnDown->setAutoRepeat( true );
  mBtnDown->setPixmap( BarIcon( "down", KIcon::SizeSmall ) );
  mBtnDown->setMinimumSize( mBtnDown->sizeHint() * 1.2 );
  QToolTip::add( mBtnUp, i18n("Up") );
  QToolTip::add( mBtnDown, i18n("Down") );
  QWhatsThis::add( mBtnUp, i18n(_wt_filterlist_up) );
  QWhatsThis::add( mBtnDown, i18n(_wt_filterlist_down) );

  //----------- the second row of buttons
  hb = new QHBox(this);
  hb->setSpacing(4);
  mBtnNew = new QPushButton( QString::null, hb );
  mBtnNew->setPixmap( BarIcon( "filenew", KIcon::SizeSmall ) );
  mBtnNew->setMinimumSize( mBtnNew->sizeHint() * 1.2 );
  mBtnCopy = new QPushButton( QString::null, hb );
  mBtnCopy->setPixmap( BarIcon( "editcopy", KIcon::SizeSmall ) );
  mBtnCopy->setMinimumSize( mBtnCopy->sizeHint() * 1.2 );
  mBtnDelete = new QPushButton( QString::null, hb );
  mBtnDelete->setPixmap( BarIcon( "editdelete", KIcon::SizeSmall ) );
  mBtnDelete->setMinimumSize( mBtnDelete->sizeHint() * 1.2 );
  mBtnRename = new QPushButton( i18n("Rename..."), hb );
  QToolTip::add( mBtnNew, i18n("New") );
  QToolTip::add( mBtnCopy, i18n("Copy") );
  QToolTip::add( mBtnDelete, i18n("Delete"));
  QWhatsThis::add( mBtnNew, i18n(_wt_filterlist_new) );
  QWhatsThis::add( mBtnCopy, i18n(_wt_filterlist_copy) );
  QWhatsThis::add( mBtnDelete, i18n(_wt_filterlist_delete) );
  QWhatsThis::add( mBtnRename, i18n(_wt_filterlist_rename) );


  //----------- now connect everything
  connect( mListBox, SIGNAL(highlighted(int)),
	   this, SLOT(slotSelected(int)) );
  connect( mListBox, SIGNAL( doubleClicked ( QListBoxItem * )),
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


void KMFilterListBox::createFilter( const QCString & field,
				    const QString & value )
{
  KMSearchRule *newRule = KMSearchRule::createInstance( field, KMSearchRule::FuncContains, value );

  KMFilter *newFilter = new KMFilter(0, bPopFilter);
  newFilter->pattern()->append( newRule );
  newFilter->pattern()->setName( QString("<%1>:%2").arg( field ).arg( value) );

  KMFilterActionDesc *desc = (*kmkernel->filterActionDict())["transfer"];
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
  KMSearchPattern *p = mFilterList.at(mIdxSelItem)->pattern();
  if ( !p ) return;

  QString shouldBeName = p->name();
  QString displayedName = mListBox->text( mIdxSelItem );

  if ( shouldBeName.stripWhiteSpace().isEmpty() ) {
    mFilterList.at(mIdxSelItem)->setAutoNaming( true );
  }
  
  if ( mFilterList.at(mIdxSelItem)->isAutoNaming() ) {
    // auto-naming of patterns
    if ( p->first() && !p->first()->field().stripWhiteSpace().isEmpty() )
      shouldBeName = QString( "<%1>: %2" ).arg( p->first()->field() ).arg( p->first()->contents() );
    else
      shouldBeName = "<" + i18n("unnamed") + ">";
    p->setName( shouldBeName );
  }

  if ( displayedName == shouldBeName ) return;

  mListBox->blockSignals(TRUE);
  mListBox->changeItem( shouldBeName, mIdxSelItem );
  mListBox->blockSignals(FALSE);
}

void KMFilterListBox::slotShowLaterToggled(bool aOn)
{
  mShowLater = aOn;
}

void KMFilterListBox::slotApplyFilterChanges()
{
  int oIdxSelItem = mIdxSelItem;
  // unselect all filters:
  mListBox->selectAll( FALSE );
  // maybe QListBox doesn't emit selected(-1) on unselect,
  // so we make sure the edit widgets receive an equivalent:
  emit resetWidgets();
  mIdxSelItem = -1;
  enableControls();

  // by now all edit widgets should have written back
  // their widget's data into our filter list.

  KMFilterMgr *fm;
  if (bPopFilter)
    fm = kmkernel->popFilterMgr();
  else
    fm = kmkernel->filterMgr();

  // block attemts to use filters (currently a no-op)
  fm->beginUpdate();
  fm->clear();

  QStringList emptyFilters;
  QPtrListIterator<KMFilter> it( mFilterList );
  for ( it.toFirst() ; it.current() ; ++it ) {
    KMFilter *f = new KMFilter( **it ); // deep copy
    f->purify();
    if ( !f->isEmpty() )
      // the filter is valid:
      fm->append( f );
    else {
      // the filter is invalid:
      emptyFilters << f->name();
      delete f;
    }
  }
  if (bPopFilter)
    fm->setShowLaterMsgs(mShowLater);

  // allow usage of the filters again.
  fm->endUpdate();
  fm->writeConfig();

  // report on invalid filters:
  if ( !emptyFilters.empty() ) {
    QString msg = i18n("The following filters have not been saved because they "
		       "were invalid (e.g. containing no actions or no search "
		       "rules).");
    KMessageBox::informationList( 0, msg, emptyFilters, QString::null,
				  "ShowInvalidFilterWarning" );
  }

  if ( oIdxSelItem >= 0 ) {
    mIdxSelItem = oIdxSelItem;
    mListBox->setSelected( oIdxSelItem, TRUE);
    slotSelected( mListBox->currentItem() );
  }
}

void KMFilterListBox::slotSelected( int aIdx )
{
  mIdxSelItem = aIdx;
  // QPtrList::at(i) will return 0 if i is out of range.
  KMFilter *f = mFilterList.at(aIdx);
  if ( f )
    emit filterSelected( f );
  else
    emit resetWidgets();
  enableControls();
}

void KMFilterListBox::slotNew()
{
  // just insert a new filter.
  insertFilter( new KMFilter(0, bPopFilter) );
  enableControls();
}

void KMFilterListBox::slotCopy()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug(5006) << "KMFilterListBox::slotCopy called while no filter is selected, ignoring." << endl;
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
    kdDebug(5006) << "KMFilterListBox::slotDelete called while no filter is selected, ignoring." << endl;
    return;
  }

  int oIdxSelItem = mIdxSelItem;
  mIdxSelItem = -1;
  // unselect all
  mListBox->selectAll(FALSE);
  // broadcast that all widgets let go
  // of the filter
  emit resetWidgets();

  // remove the filter from both the filter list...
  mFilterList.remove( oIdxSelItem );
  // and the listbox
  mListBox->removeItem( oIdxSelItem );

  int count = (int)mListBox->count();
  // and set the new current item.
  if ( count > oIdxSelItem )
    // oIdxItem is still a valid index
    mListBox->setSelected( oIdxSelItem, TRUE );
  else if ( (int)mListBox->count() )
    // oIdxSelIdx is no longer valid, but the
    // list box isn't empty
    mListBox->setSelected( count - 1, TRUE );
  // the list is empty - keep index -1

  enableControls();
}

void KMFilterListBox::slotUp()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug(5006) << "KMFilterListBox::slotUp called while no filter is selected, ignoring." << endl;
    return;
  }
  if ( mIdxSelItem == 0 ) {
    kdDebug(5006) << "KMFilterListBox::slotUp called while the _topmost_ filter is selected, ignoring." << endl;
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem - 1 );
  enableControls();
}

void KMFilterListBox::slotDown()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug(5006) << "KMFilterListBox::slotDown called while no filter is selected, ignoring." << endl;
    return;
  }
  if ( mIdxSelItem == (int)mListBox->count() - 1 ) {
    kdDebug(5006) << "KMFilterListBox::slotDown called while the _last_ filter is selected, ignoring." << endl;
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem + 1);
  enableControls();
}

void KMFilterListBox::slotRename()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug(5006) << "KMFilterListBox::slotRename called while no filter is selected, ignoring." << endl;
    return;
  }

  bool okPressed = FALSE;
  KMFilter *filter = mFilterList.at( mIdxSelItem );

  // enableControls should make sure this method is
  // never called when no filter is selected.
  assert( filter );

  // allow empty names - those will turn auto-naming on again
  QValidator *validator = new QRegExpValidator( QRegExp( ".*" ), 0 );
  QString newName = KInputDialog::getText
    (
     i18n("Rename Filter"),
     i18n("Rename filter \"%1\" to:\n(leave the field empty for automatic naming)")
        .arg( filter->pattern()->name() ) /*label*/,
     filter->pattern()->name() /* initial value */,
     &okPressed, topLevelWidget(), 0, validator
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
    mListBox->ensureCurrentVisible();
}

void KMFilterListBox::loadFilterList()
{
  assert(mListBox);
  setEnabled(FALSE);
  // we don't want the insertion to
  // cause flicker in the edit widgets.
  blockSignals(TRUE);

  // clear both lists
  mFilterList.clear();
  mListBox->clear();

	QPtrList<KMFilter> *manager;
  if(bPopFilter)
	{
    mShowLater = kmkernel->popFilterMgr()->showLaterMsgs();
		manager = kmkernel->popFilterMgr();
	}
	else
	{
		manager = kmkernel->filterMgr();
	}

  QPtrListIterator<KMFilter> it( *manager );
  for ( it.toFirst() ; it.current() ; ++it ) {
    mFilterList.append( new KMFilter( **it ) ); // deep copy
    mListBox->insertItem( (*it)->pattern()->name() );
  }

  blockSignals(FALSE);
  setEnabled(TRUE);

  // create an empty filter when there's none, to avoid a completely
  // disabled dialog (usability tests indicated that the new-filter
  // button is too hard to find that way):
  if ( !mListBox->count() )
    slotNew();

  assert( mListBox->count() > 0 );
  mListBox->setSelected( 0, true );

  enableControls();
}

void KMFilterListBox::insertFilter( KMFilter* aFilter )
{
  // must be really a filter...
  assert( aFilter );

  // if mIdxSelItem < 0, QListBox::insertItem will append.
  mListBox->insertItem( aFilter->pattern()->name(), mIdxSelItem );
  if ( mIdxSelItem < 0 ) {
    // none selected -> append
    mFilterList.append( aFilter );
    mListBox->setSelected( mListBox->count() - 1, TRUE );
    //    slotSelected( mListBox->count() - 1 );
  } else {
    // insert just before selected
    mFilterList.insert( mIdxSelItem, aFilter );
    mListBox->setSelected( mIdxSelItem, TRUE );
    //    slotSelected( mIdxSelItem );
  }

}

void KMFilterListBox::swapNeighbouringFilters( int untouchedOne, int movedOne )
{
  // must be neighbours...
  assert( untouchedOne - movedOne == 1 || movedOne - untouchedOne == 1 );

  // untouchedOne is at idx. to move it down(up),
  // remove item at idx+(-)1 w/o deleting it.
  QListBoxItem *item = mListBox->item( movedOne );
  mListBox->takeItem( item );
  // now selected item is at idx(idx-1), so
  // insert the other item at idx, ie. above(below).
  mListBox->insertItem( item, untouchedOne );

  KMFilter* filter = mFilterList.take( movedOne );
  mFilterList.insert( untouchedOne, filter );

  mIdxSelItem += movedOne - untouchedOne;
}


//=============================================================================
//
// class KMFilterActionWidget
//
//=============================================================================

KMFilterActionWidget::KMFilterActionWidget( QWidget *parent, const char* name )
  : QHBox( parent, name )
{
  int i;
  mActionList.setAutoDelete(TRUE);

  mComboBox = new QComboBox( FALSE, this );
  assert( mComboBox );
  mWidgetStack = new QWidgetStack(this);
  assert( mWidgetStack );

  setSpacing( 4 );

  QPtrListIterator<KMFilterActionDesc> it ( kmkernel->filterActionDict()->list() );
  for ( i=0, it.toFirst() ; it.current() ; ++it, ++i ) {
    //create an instance:
    KMFilterAction *a = (*it)->create();
    // append to the list of actions:
    mActionList.append( a );
    // add parameter widget to widget stack:
    mWidgetStack->addWidget( a->createParamWidget( mWidgetStack ), i );
    // add (i18n-ized) name to combo box
    mComboBox->insertItem( (*it)->label );
  }
  // widget for the case where no action is selected.
  mWidgetStack->addWidget( new QLabel( i18n("Please select an action."), mWidgetStack ), i );
  mWidgetStack->raiseWidget(i);
  mComboBox->insertItem( " " );
  mComboBox->setCurrentItem(i);

  // don't show scroll bars.
  mComboBox->setSizeLimit( mComboBox->count() );
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
	   mWidgetStack, SLOT(raiseWidget(int)) );
}

void KMFilterActionWidget::setAction( const KMFilterAction* aAction )
{
  int i=0;
  bool found = FALSE;
  int count = mComboBox->count() - 1 ; // last entry is the empty one
  QString label = ( aAction ) ? aAction->label() : QString::null ;

  // find the index of typeOf(aAction) in mComboBox
  // and clear the other widgets on the way.
  for ( ; i < count ; i++ )
    if ( aAction && mComboBox->text(i) == label ) {
      //...set the parameter widget to the settings
      // of aAction...
      aAction->setParamWidgetValue( mWidgetStack->widget(i) );
      //...and show the correct entry of
      // the combo box
      mComboBox->setCurrentItem(i); // (mm) also raise the widget, but doesn't
      mWidgetStack->raiseWidget(i);
      found = TRUE;
    } else // clear the parameter widget
      mActionList.at(i)->clearParamWidget( mWidgetStack->widget(i) );
  if ( found ) return;

  // not found, so set the empty widget
  mComboBox->setCurrentItem( count ); // last item
  mWidgetStack->raiseWidget( count) ;
}

KMFilterAction * KMFilterActionWidget::action()
{
  // look up the action description via the label
  // returned by QComboBox::currentText()...
  KMFilterActionDesc *desc = (*kmkernel->filterActionDict())[ mComboBox->currentText() ];
  if ( desc ) {
    // ...create an instance...
    KMFilterAction *fa = desc->create();
    if ( fa ) {
      // ...and apply the setting of the parameter widget.
      fa->applyParamWidgetValue( mWidgetStack->visibleWidget() );
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

void KMFilterActionWidgetLister::setActionList( QPtrList<KMFilterAction> *aList )
{
  assert ( aList );

  if ( mActionList )
    regenerateActionListFromWidgets();

  mActionList = aList;

  ((QWidget*)parent())->setEnabled( TRUE );

  if ( aList->count() == 0 ) {
    slotClear();
    return;
  }

  int superfluousItems = (int)mActionList->count() - mMaxWidgets ;
  if ( superfluousItems > 0 ) {
    kdDebug(5006) << "KMFilterActionWidgetLister: Clipping action list to "
	      << mMaxWidgets << " items!" << endl;

    for ( ; superfluousItems ; superfluousItems-- )
      mActionList->removeLast();
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo( mActionList->count() );

  // load the actions into the widgets
  QPtrListIterator<KMFilterAction> aIt( *mActionList );
  QPtrListIterator<QWidget> wIt( mWidgetList );
  for ( aIt.toFirst(), wIt.toFirst() ;
	aIt.current() && wIt.current() ; ++aIt, ++wIt )
    ((KMFilterActionWidget*)(*wIt))->setAction( (*aIt) );
}

void KMFilterActionWidgetLister::reset()
{
  if ( mActionList )
    regenerateActionListFromWidgets();

  mActionList = 0;
  slotClear();
  ((QWidget*)parent())->setEnabled( FALSE );
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

  QPtrListIterator<QWidget> it( mWidgetList );
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
  : QVButtonGroup( title, parent, name )
{
  mActionMap[Down] = new QRadioButton( i18n("&Download mail"), this );
  mActionMap[Later] = new QRadioButton( i18n("Download mail la&ter"), this );
  mActionMap[Delete] = new QRadioButton( i18n("D&elete mail from server"), this );
  mIdMap[id(mActionMap[Later])] = Later;
  mIdMap[id(mActionMap[Down])] = Down;
  mIdMap[id(mActionMap[Delete])] = Delete;

  connect( this, SIGNAL(clicked(int)),
	   this, SLOT( slotActionClicked(int)) );
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

void KMPopFilterActionWidget::slotActionClicked(int aId)
{
  emit actionChanged(mIdMap[aId]);
  setAction(mIdMap[aId]);
}

void KMPopFilterActionWidget::reset()
{
  blockSignals(TRUE);
  mActionMap[Down]->setChecked( TRUE );
  blockSignals(FALSE);

  setEnabled( FALSE );
}

#include "kmfilterdlg.moc"
