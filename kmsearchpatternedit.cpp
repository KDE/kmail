// kmfilterrulesedit.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL

#include "kmsearchpatternedit.h"

#include <klocale.h>
#include <kdebug.h>
#include <kbuttonbox.h>

#include <qstring.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qbuttongroup.h>

#include <assert.h>

static QStringList sFilterFieldList, sFilterFuncList;

//=============================================================================
//
// class KMSearchRuleWidget
//
//=============================================================================

KMSearchRuleWidget::KMSearchRuleWidget(QWidget *parent, KMSearchRule *aRule, const char *name)
  : QHBox(parent,name)
{
  initLists(); // sFilter{Func,Field}List are local to KMSearchRuleWidget
  initWidget();
  
  if ( aRule )
    setRule(aRule);
  else
    reset();
}

void KMSearchRuleWidget::initWidget()
{
  setSpacing(4);

  mRuleField = new QComboBox( true, this, "mRuleField" );
  mRuleFunc = new QComboBox( false, this, "mRuleFunc" );
  mRuleValue = new QLineEdit( this, "mRuleValue" );
  
  mRuleFunc->insertStringList(sFilterFuncList);
  mRuleFunc->adjustSize();

  mRuleField->insertStringList(sFilterFieldList);
  mRuleField->adjustSize();
  
  connect( mRuleField, SIGNAL(textChanged(const QString &)),
	   this, SIGNAL(fieldChanged(const QString &)) );
  connect( mRuleValue, SIGNAL(textChanged(const QString &)),
	   this, SIGNAL(contentsChanged(const QString &)) );
}

void KMSearchRuleWidget::setRule(KMSearchRule *aRule)
{
  assert ( aRule );

  blockSignals(TRUE);
  //--------------set the field
  int i = indexOfRuleField( aRule->field() );
  
  if ( i<0 ) { // not found -> user defined field
    mRuleField->changeItem( aRule->field(), 0 );
    i=0;
  } else // found in the list of predefined fields
    mRuleField->changeItem( " ", 0 );
  
  mRuleField->setCurrentItem( i );

  //--------------set function and contents
  mRuleFunc->setCurrentItem( (int)aRule->function() );
  mRuleValue->setText( aRule->contents() );

  blockSignals(FALSE);
}

KMSearchRule* KMSearchRuleWidget::rule() const
{
  KMSearchRule *r = new KMSearchRule;

  r->init( ruleFieldToEnglish( mRuleField->currentText() ),
	   (KMSearchRule::Function)mRuleFunc->currentItem(),
	   mRuleValue->text() );

  return r;
}

void KMSearchRuleWidget::reset()
{
  blockSignals(TRUE);

  mRuleField->changeItem( " ", 0 );
  mRuleField->setCurrentItem( 0 );

  mRuleFunc->setCurrentItem( 0 );

  mRuleValue->clear();

  blockSignals(FALSE);
}

QString KMSearchRuleWidget::ruleFieldToEnglish(const QString & i18nVal) const
{
  if (i18nVal == i18n("<To or Cc>")) return QString("<To or Cc>");
  if (i18nVal == i18n("<body>")) return QString("<body>");
  if (i18nVal == i18n("<message>")) return QString("<message>");
  if (i18nVal == i18n("<any header>")) return QString("<any header>");
  return i18nVal;
}

int KMSearchRuleWidget::indexOfRuleField(const QString aName) const
{
  int i;

  if ( aName.isEmpty() ) return -1;

  for (i=sFilterFieldList.count()-1; i>=0; i--) {
    if (*(sFilterFieldList.at(i))==i18n(aName)) break;
  }
  return i;
}

void KMSearchRuleWidget::initLists() const
{
  //---------- initialize list of filter operators
  if ( sFilterFuncList.isEmpty() )
  {
    // also see KMSearchRule::matches() and KMSearchRule::Function
    // you change the following strings!
    sFilterFuncList.append(i18n("equals"));
    sFilterFuncList.append(i18n("doesn't equal"));
    sFilterFuncList.append(i18n("contains"));
    sFilterFuncList.append(i18n("doesn't contain"));
    sFilterFuncList.append(i18n("matches regular expr."));
    sFilterFuncList.append(i18n("doesn't match reg. expr."));
  }

  //---------- initialize list of filter operators
  if ( sFilterFieldList.isEmpty() )
  {
    sFilterFieldList.append(" ");
    // also see KMSearchRule::matches() and ruleFieldToEnglish() if
    // you change the following i18n-ized strings!
    sFilterFieldList.append(i18n("<message>"));
    sFilterFieldList.append(i18n("<body>"));
    sFilterFieldList.append(i18n("<any header>"));
    sFilterFieldList.append(i18n("<To or Cc>"));
    // these others only represent meassage headers and you can add to
    // them as you like
    sFilterFieldList.append("Subject");
    sFilterFieldList.append("From");
    sFilterFieldList.append("To");
    sFilterFieldList.append("Cc");
    sFilterFieldList.append("Reply-To");
    sFilterFieldList.append("Organization");
    sFilterFieldList.append("Resent-From");
    sFilterFieldList.append("X-Loop");
  }
}

//=============================================================================
//
// class KMFilterActionWidgetLister (the filter action editor)
//
//=============================================================================

KMSearchRuleWidgetLister::KMSearchRuleWidgetLister( QWidget *parent, const char* name )
  : KWidgetLister( 2, FILTER_MAX_RULES, parent, name )
{
  mRuleList = 0;
}

KMSearchRuleWidgetLister::~KMSearchRuleWidgetLister()
{
}

void KMSearchRuleWidgetLister::setRuleList( QList<KMSearchRule> *aList )
{
  assert ( aList );

  if ( mRuleList )
    regenerateRuleListFromWidgets();

  mRuleList = aList;

  if ( mWidgetList.first() ) // move this below next 'if'?
    mWidgetList.first()->blockSignals(TRUE);

  if ( aList->count() == 0 ) {
    slotClear();
    mWidgetList.first()->blockSignals(FALSE);
    return;
  }

  int superfluousItems = (int)mRuleList->count() - mMaxWidgets ;
  if ( superfluousItems > 0 ) {
    kdDebug() << "KMSearchRuleWidgetLister: Clipping rule list to "
	      << mMaxWidgets << " items!" << endl;

    for ( ; superfluousItems ; superfluousItems-- )
      mRuleList->removeLast();
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo( QMAX((int)mRuleList->count(),mMinWidgets) );

  // load the actions into the widgets
  QListIterator<KMSearchRule> rIt( *mRuleList );
  QListIterator<QWidget> wIt( mWidgetList );
  for ( rIt.toFirst(), wIt.toFirst() ;
	rIt.current() && wIt.current() ; ++rIt, ++wIt ) {
    ((KMSearchRuleWidget*)(*wIt))->setRule( (*rIt) );
  }
  for ( ; wIt.current() ; ++wIt )
    ((KMSearchRuleWidget*)(*wIt))->reset();

  assert( mWidgetList.first() );
  mWidgetList.first()->blockSignals(FALSE);
}

void KMSearchRuleWidgetLister::reset()
{
  if ( mRuleList )
    regenerateRuleListFromWidgets();

  mRuleList = 0;
  slotClear();
}

QWidget* KMSearchRuleWidgetLister::createWidget( QWidget *parent )
{
  return new KMSearchRuleWidget(parent);
}

void KMSearchRuleWidgetLister::clearWidget( QWidget *aWidget )
{
  if ( aWidget )
    ((KMSearchRuleWidget*)aWidget)->reset();
}

void KMSearchRuleWidgetLister::regenerateRuleListFromWidgets()
{
  if ( !mRuleList ) return;

  mRuleList->clear();

  QListIterator<QWidget> it( mWidgetList );
  for ( it.toFirst() ; it.current() ; ++it ) {
    KMSearchRule *r = ((KMSearchRuleWidget*)(*it))->rule();
    if ( r )
      mRuleList->append( r );
  }
}




//=============================================================================
//
// class KMSearchPatternEdit
//
//=============================================================================

KMSearchPatternEdit::KMSearchPatternEdit(QWidget *parent, const char *name )
  : QGroupBox( 1/*columns*/, Horizontal, parent, name )
{
  setTitle( i18n("Search Criteria") );
  initLayout();
}

KMSearchPatternEdit::KMSearchPatternEdit(const QString & title, QWidget *parent, const char *name )
  : QGroupBox( 1/*column*/, Horizontal, title, parent, name )
{
  initLayout();
}

KMSearchPatternEdit::~KMSearchPatternEdit()
{
}

void KMSearchPatternEdit::initLayout()
{
  //------------the radio buttons	
  mAllRBtn = new QRadioButton( i18n("Match all of the following"), this, "mAllRBtn" );
  mAnyRBtn = new QRadioButton( i18n("Match any of the following"), this, "mAnyRBtn" );
  
  mAllRBtn->setChecked(TRUE);
  mAnyRBtn->setChecked(FALSE);

  QButtonGroup *bg = new QButtonGroup( this );
  bg->hide();
  bg->insert( mAllRBtn, (int)KMSearchPattern::OpAnd );
  bg->insert( mAnyRBtn, (int)KMSearchPattern::OpOr );

  //------------the list of KMSearchRuleWidget's
  mRuleLister = new KMSearchRuleWidgetLister( this );
  mRuleLister->slotClear();

  //------------connect a few signals
  connect( bg, SIGNAL(clicked(int)),
	   this, SLOT(slotRadioClicked(int)) );

  KMSearchRuleWidget *srw = (KMSearchRuleWidget*)mRuleLister->mWidgetList.first();
  if ( srw ) {
    connect( srw, SIGNAL(fieldChanged(const QString &)),
	     this, SLOT(slotAutoNameHack()) );
    connect( srw, SIGNAL(contentsChanged(const QString &)),
	     this, SLOT(slotAutoNameHack()) );
  } else
    kdDebug() << "KMSearchPatternEdit: no first KMSearchRuleWidget, though slotClear() has been called!" << endl;
}

void KMSearchPatternEdit::setSearchPattern( KMSearchPattern *aPattern )
{
  assert( aPattern );

  mRuleLister->setRuleList( aPattern );
  
  mPattern = aPattern;

  blockSignals(TRUE);
  if ( mPattern->op() == KMSearchPattern::OpOr )
    mAnyRBtn->setChecked(TRUE);
  else
    mAllRBtn->setChecked(TRUE);
  blockSignals(FALSE);

  setEnabled( TRUE );
}

void KMSearchPatternEdit::reset()
{
  mRuleLister->reset();

  blockSignals(TRUE);
  mAllRBtn->setChecked( TRUE );
  blockSignals(FALSE);

  setEnabled( FALSE );
}

void KMSearchPatternEdit::slotRadioClicked(int aIdx)
{
  if ( mPattern ) 
    mPattern->setOp( (KMSearchPattern::Operator)aIdx );
}

void KMSearchPatternEdit::slotAutoNameHack()
{
  mRuleLister->regenerateRuleListFromWidgets();
  emit maybeNameChanged();
}
#include "kmsearchpatternedit.moc"
