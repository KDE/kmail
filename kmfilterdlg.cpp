// kmfilterdlg.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// based on work by Stefan Taferner <taferner@kde.org>
// This code is under the GPL

#include "kmfilterdlg.h"
#include "kmsearchpatternedit.h"
#include "kmsearchpattern.h"
#include "kmfilter.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmglobal.h"

#include <kdebug.h>
#include <klocale.h>
#include <klineeditdlg.h>
#include <kiconloader.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qwidgetstack.h>

#include <assert.h>

// The anchor of the filter dialog's help.
const QString KMFilterDlgHelpAnchor( "FILTERS_ID" );

//=============================================================================
//
// class KMFilterDlg (the filter dialog)
//
//=============================================================================

KMFilterDlg::KMFilterDlg(QWidget* parent, const char* name)
  : KDialogBase( parent, name, FALSE /* modality */,
		 i18n("Filter Rules") /* caption*/,
		 Help|Ok|Apply|Cancel /* button mask */,
		 Ok /* default btn */, FALSE /* separator */)
{
  setHelp( KMFilterDlgHelpAnchor );

  QWidget *w = new QWidget(this);
  setMainWidget(w);
  QHBoxLayout *hbl = new QHBoxLayout( w, 0, spacingHint(), "kmfd_hbl" );

  mFilterList = new KMFilterListBox( i18n("Available Filters"), w);
  hbl->addWidget( mFilterList, 1 /*stretch*/ );

  QVBoxLayout *vbl = new QVBoxLayout( hbl, spacingHint(), "kmfd_vbl" );

  mPatternEdit = new KMSearchPatternEdit( i18n("Filter Criteria"), w );
  vbl->addWidget( mPatternEdit, 0, Qt::AlignTop );

  QGroupBox *agb = new QGroupBox( 1 /*column*/, Vertical, i18n("Filter Actions"), w );
  mActionLister = new KMFilterActionWidgetLister( agb );
  vbl->addWidget( agb, 0, Qt::AlignTop );
  // spacer:
  vbl->addStretch( 1 );
  
  // load the filter parts into the edit widgets
  connect( mFilterList, SIGNAL(filterSelected(KMFilter*)),
	   this, SLOT(slotFilterSelected(KMFilter*)) );
  
  // reset the edit widgets
  connect( mFilterList, SIGNAL(resetWidgets()),
	   mPatternEdit, SLOT(reset()) );
  connect( mFilterList, SIGNAL(resetWidgets()),
	   mActionLister, SLOT(reset()) );
  
  // support auto-naming the filter
  connect( mPatternEdit, SIGNAL(maybeNameChanged()),
	   mFilterList, SLOT(slotUpdateFilterName()) );

  // apply changes on 'Apply'
  connect( this, SIGNAL(applyClicked()),
	   mFilterList, SLOT(slotApplyFilterChanges()) );

  // apply changes on 'OK'
  connect( this, SIGNAL(okClicked()),
	   mFilterList, SLOT(slotApplyFilterChanges()) );
  
  // destruct the dialog on OK, close and Cancel
  connect( this, SIGNAL(finished()),
	   this, SLOT(slotDelayedDestruct()) );

  adjustSize();

  // load the filter list (emits filterSelected())
  mFilterList->loadFilterList();
}

void KMFilterDlg::slotFilterSelected( KMFilter* aFilter )
{
  assert( aFilter );
  mPatternEdit->setSearchPattern( aFilter->pattern() );
  mActionLister->setActionList( aFilter->actions() );
}


//=============================================================================
//
// class KMFilterListBox (the filter list manipulator)
//
//=============================================================================

KMFilterListBox::KMFilterListBox( const QString & title, QWidget *parent, const char* name )
  : QGroupBox( 1, Horizontal, title, parent, name )
{
  mFilterList.setAutoDelete(TRUE);
  mIdxSelItem = -1;

  //----------- the list box
  mListBox = new QListBox(this);
  
  //----------- the first row of buttons
  QHBox *hb = new QHBox(this);
  mBtnUp = new QPushButton( QString::null, hb );
  mBtnUp->setPixmap( BarIcon( "up", KIcon::SizeSmall ) );
  mBtnUp->setMinimumSize( mBtnUp->sizeHint() * 1.2 );
  mBtnDown = new QPushButton( QString::null, hb );
  mBtnDown->setPixmap( BarIcon( "down", KIcon::SizeSmall ) );
  mBtnDown->setMinimumSize( mBtnDown->sizeHint() * 1.2 );
  
  //----------- the second row of buttons
  hb = new QHBox(this);
  mBtnNew = new QPushButton( QString::null, hb );
  mBtnNew->setPixmap( BarIcon( "filenew", KIcon::SizeSmall ) );
  mBtnNew->setMinimumSize( mBtnNew->sizeHint() * 1.2 );
  mBtnDelete = new QPushButton( QString::null, hb );
  mBtnDelete->setPixmap( BarIcon( "editdelete", KIcon::SizeSmall ) );
  mBtnDelete->setMinimumSize( mBtnDelete->sizeHint() * 1.2 );
  mBtnRename = new QPushButton( i18n("Rename"), hb );

  //----------- now connect everything
  connect( mListBox, SIGNAL(highlighted(int)),
	   this, SLOT(slotSelected(int)) );
  connect( mBtnUp, SIGNAL(clicked()),
	   this, SLOT(slotUp()) );
  connect( mBtnDown, SIGNAL(clicked()),
	   this, SLOT(slotDown()) );
  connect( mBtnNew, SIGNAL(clicked()),
	   this, SLOT(slotNew()) );
  connect( mBtnDelete, SIGNAL(clicked()),
	   this, SLOT(slotDelete()) );
  connect( mBtnRename, SIGNAL(clicked()),
	   this, SLOT(slotRename()) );

  // the dialog should call loadFilterList()
  // when all signals are connected.
  enableControls();
}


void KMFilterListBox::createFilter( const QString field, const QString value )
{
  KMSearchRule *newRule = new KMSearchRule();
  newRule->init( field, KMSearchRule::FuncEquals, value );
  
  KMFilter *newFilter = new KMFilter();
  newFilter->pattern()->append( newRule );
  newFilter->pattern()->setName( QString("<%1>:%2").arg( field ).arg( value) );
  
  KMFilterActionDesc *desc = (*kernel->filterActionDict())["move to folder"];
  if ( desc )
    newFilter->actions()->append( desc->create() );

  insertFilter( newFilter );
  enableControls();
}

void KMFilterListBox::slotUpdateFilterName()
{
  KMSearchPattern *p = mFilterList.at(mIdxSelItem)->pattern();
  if ( !p ) return;

  QString shouldBeName = p->name();
  QString displayedName = mListBox->text( mIdxSelItem );
  
  if ( shouldBeName.stripWhiteSpace().isEmpty() || shouldBeName[0] == '<' ) {
    // auto-naming of patterns
    if ( p->first() && !p->first()->field().stripWhiteSpace().isEmpty() )
      shouldBeName = QString( "<%1>:%2" ).arg( p->first()->field() ).arg( p->first()->contents() );
    else
      shouldBeName = "<" + i18n("unnamed") + ">";
    p->setName( shouldBeName );
  }

  if ( displayedName == shouldBeName ) return;

  mListBox->blockSignals(TRUE);
  mListBox->changeItem( shouldBeName, mIdxSelItem );
  mListBox->blockSignals(FALSE);
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

  KMFilterMgr *fm = kernel->filterMgr();

  // block attemts to use filters (currently a no-op)
  fm->beginUpdate();
  fm->clear();

  QListIterator<KMFilter> it( mFilterList );
  for ( it.toFirst() ; it.current() ; ++it ) {
    KMFilter *f = new KMFilter( (*it) ); // deep copy
    f->purify();
    if ( !f->isEmpty() )
      // the filter is valid:
      fm->append( f );
    else
      // the filter is invalid:
      delete f;
  }
  
  // allow usage of the filters again.
  fm->endUpdate();
  fm->writeConfig();

  if ( oIdxSelItem >= 0 ) {
    mIdxSelItem = oIdxSelItem;
    mListBox->setSelected( oIdxSelItem, TRUE);
    slotSelected( mListBox->currentItem() );
  }
}

void KMFilterListBox::slotSelected( int aIdx )
{
  mIdxSelItem = aIdx;
  // QList::at(i) will return NULL if i is out of range.
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
  insertFilter( new KMFilter() );
  enableControls();
}

void KMFilterListBox::slotDelete()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug() << "KMFilterListBox::slotDelete called while no filter is selected, ignoring." << endl;
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
    kdDebug() << "KMFilterListBox::slotUp called while no filter is selected, ignoring." << endl;
    return;
  }
  if ( mIdxSelItem == 0 ) {
    kdDebug() << "KMFilterListBox::slotUp called while the _topmost_ filter is selected, ignoring." << endl;
    return;
  }

  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem - 1 );
  enableControls();
}

void KMFilterListBox::slotDown()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug() << "KMFilterListBox::slotDown called while no filter is selected, ignoring." << endl;
    return;
  }
  if ( mIdxSelItem == (int)mListBox->count() - 1 ) {
    kdDebug() << "KMFilterListBox::slotDown called while the _last_ filter is selected, ignoring." << endl;
    return;
  }
  
  swapNeighbouringFilters( mIdxSelItem, mIdxSelItem + 1);
  enableControls();
}

void KMFilterListBox::slotRename()
{
  if ( mIdxSelItem < 0 ) {
    kdDebug() << "KMFilterListBox::slotRename called while no filter is selected, ignoring." << endl;
    return;
  }

  bool okPressed = FALSE;
  KMFilter *filter = mFilterList.at( mIdxSelItem );

  // enableControls should make sure this method is
  // never called when no filter is selected.
  assert( filter );

  QString newName = KLineEditDlg::getText
    (
     i18n("Rename filter \"%1\" to:").arg( filter->pattern()->name() ) /*label*/,
     filter->pattern()->name() /* initial value */,
     &okPressed, 0 /* parent */
     );

  if ( !okPressed ) return;

  if ( newName.isEmpty() )
    // bait for slotUpdateFilterName to
    // use automatic naming again.
    filter->pattern()->setName( "<>" );
  else
    filter->pattern()->setName( newName );

  slotUpdateFilterName();
}

void KMFilterListBox::enableControls()
{
  bool theFirst = ( mIdxSelItem == 0 );
  bool theLast = ( mIdxSelItem >= (int)mFilterList.count() - 1 );
  bool aFilterIsSelected = ( mIdxSelItem >= 0 );

  mBtnUp->setEnabled( aFilterIsSelected && !theFirst );
  mBtnDown->setEnabled( aFilterIsSelected && !theLast );
  mBtnDelete->setEnabled( aFilterIsSelected );
  mBtnRename->setEnabled( aFilterIsSelected );
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

  QListIterator<KMFilter> it( *kernel->filterMgr() );
  for ( it.toFirst() ; it.current() ; ++it ) {
    mFilterList.append( new KMFilter( *it ) ); // deep copy
    mListBox->insertItem( (*it)->pattern()->name() );
  }

  blockSignals(FALSE);
  setEnabled(TRUE);

  // select topmost item
  if ( mListBox->count() )
    mListBox->setSelected( 0, TRUE );
  else {
    emit resetWidgets();
    mIdxSelItem = -1;
  }

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
  
  QListIterator<KMFilterActionDesc> it ( kernel->filterActionDict()->list() );
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

  // layout management:
  // o the combo box is not to be made larger than it's sizeHint(),
  //   the parameter widget should grow instead.
  // o the whole widget takes all space horizontally, but is fixed vertically.
  mComboBox->adjustSize();
  mComboBox->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed ) );
  setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );
  updateGeometry();

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
  KMFilterActionDesc *desc = (*kernel->filterActionDict())[ mComboBox->currentText() ];
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

void KMFilterActionWidgetLister::setActionList( QList<KMFilterAction> *aList )
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
    kdDebug() << "KMFilterActionWidgetLister: Clipping action list to "
	      << mMaxWidgets << " items!" << endl;

    for ( ; superfluousItems ; superfluousItems-- )
      mActionList->removeLast();
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo( mActionList->count() );

  // load the actions into the widgets
  QListIterator<KMFilterAction> aIt( *mActionList );
  QListIterator<QWidget> wIt( mWidgetList );
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

  QListIterator<QWidget> it( mWidgetList );
  for ( it.toFirst() ; it.current() ; ++it ) {
    KMFilterAction *a = ((KMFilterActionWidget*)(*it))->action();
    if ( a )
      mActionList->append( a );
  }
    
}

#include "kmfilterdlg.moc"
