// kmsearchpatternedit.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL

#include <config.h>
#include "kmsearchpatternedit.h"

#include <klocale.h>
#include <kdebug.h>

#include <qradiobutton.h>
#include <klineedit.h>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qstringlist.h>

#include <assert.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>
#include <qpushbutton.h>
#include <qdialog.h>

static QStringList sFilterFieldList, sFilterFuncList;

//=============================================================================
//
// class KMSearchRuleWidget
//
//=============================================================================

KMSearchRuleWidget::KMSearchRuleWidget(QWidget *parent, KMSearchRule *aRule, const char *name, bool headersOnly)
  : QHBox(parent,name),
    mRuleEditBut(0),
    mRegExpEditDialog(0)
{
  initLists( headersOnly ); // sFilter{Func,Field}List are local to KMSearchRuleWidget
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
  mRuleValue = new KLineEdit( this, "mRuleValue" );

  if( !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty() ) {
    mRuleEditBut = new QPushButton( i18n("Edit..."), this, "mRuleEditBut" );
    connect( mRuleEditBut, SIGNAL( clicked() ), this, SLOT( editRegExp()));
    connect( mRuleFunc, SIGNAL( activated(int) ), this, SLOT( functionChanged(int) ) );
    functionChanged( mRuleFunc->currentItem() );
  }

  mRuleFunc->insertStringList(sFilterFuncList);
  mRuleFunc->adjustSize();

  mRuleField->insertStringList(sFilterFieldList);
  // don't show sliders when popping up this menu
  mRuleField->setSizeLimit( mRuleField->count() );
  mRuleField->adjustSize();

  connect( mRuleField, SIGNAL(textChanged(const QString &)),
	   this, SIGNAL(fieldChanged(const QString &)) );
  connect( mRuleValue, SIGNAL(textChanged(const QString &)),
	   this, SIGNAL(contentsChanged(const QString &)) );
}

void KMSearchRuleWidget::editRegExp()
{
  if ( mRegExpEditDialog == 0 )
    mRegExpEditDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString::null, this );

  KRegExpEditorInterface *iface = static_cast<KRegExpEditorInterface *>( mRegExpEditDialog->qt_cast( "KRegExpEditorInterface" ) );
  if( iface ) {
    iface->setRegExp( mRuleValue->text() );
    if( mRegExpEditDialog->exec() == QDialog::Accepted )
      mRuleValue->setText( iface->regExp() );
  }
}

void KMSearchRuleWidget::functionChanged( int which )
{
  mRuleEditBut->setEnabled( which == 4 || which == 5 );
}


void KMSearchRuleWidget::setRule(KMSearchRule *aRule)
{
  assert ( aRule );

  blockSignals(TRUE);
  //--------------set the field
  int i = indexOfRuleField( aRule->field() );

  if ( i<0 ) { // not found -> user defined field
    mRuleField->changeItem( QString(aRule->field()), 0 );
    i=0;
  } else // found in the list of predefined fields
    mRuleField->changeItem( "", 0 );

  mRuleField->setCurrentItem( i );

  //--------------set function and contents
  mRuleFunc->setCurrentItem( (int)aRule->function() );
  mRuleValue->setText( aRule->contents() );

  if (mRuleEditBut)
    functionChanged( (int)aRule->function() );

  blockSignals(FALSE);
}

KMSearchRule* KMSearchRuleWidget::rule() const
{
  KMSearchRule *r = new KMSearchRule;

  r->init( ruleFieldToEnglish( mRuleField->currentText() ).latin1(),
	   (KMSearchRule::Function)mRuleFunc->currentItem(),
	   mRuleValue->text() );

  return r;
}

void KMSearchRuleWidget::reset()
{
  blockSignals(TRUE);

  mRuleField->changeItem( "", 0 );
  mRuleField->setCurrentItem( 0 );

  mRuleFunc->setCurrentItem( 0 );

  mRuleValue->clear();

  blockSignals(FALSE);
}

QString KMSearchRuleWidget::ruleFieldToEnglish(const QString & i18nVal) const
{
  if (i18nVal == i18n("<recipients>")) return QString::fromLatin1("<recipients>");
  if (i18nVal == i18n("<body>")) return QString::fromLatin1("<body>");
  if (i18nVal == i18n("<message>")) return QString::fromLatin1("<message>");
  if (i18nVal == i18n("<any header>")) return QString::fromLatin1("<any header>");
  if (i18nVal == i18n("<size>")) return QString::fromLatin1("<size>");
  if (i18nVal == i18n("<age in days>")) return QString::fromLatin1("<age in days>");
  return i18nVal;
}

int KMSearchRuleWidget::indexOfRuleField(const QString aName) const
{
  int i;

  if ( aName.isEmpty() ) return -1;

  QString i18n_aName = i18n( aName.latin1() );

  for (i=sFilterFieldList.count()-1; i>=0; i--) {
    if (*(sFilterFieldList.at(i))==i18n_aName) break;
  }
  return i;
}

void KMSearchRuleWidget::initLists(bool headersOnly) const
{
  //---------- initialize list of filter operators
  if ( sFilterFuncList.isEmpty() )
  {
    // also see KMSearchRule::matches() and KMSearchRule::Function
    // if you change the following strings!
    sFilterFuncList.append(i18n("contains"));
    sFilterFuncList.append(i18n("doesn't contain"));
    sFilterFuncList.append(i18n("equals"));
    sFilterFuncList.append(i18n("doesn't equal"));
    sFilterFuncList.append(i18n("matches regular expr."));
    sFilterFuncList.append(i18n("doesn't match reg. expr."));
    sFilterFuncList.append(i18n("is greater than"));
    sFilterFuncList.append(i18n("is less than or equal to"));
    sFilterFuncList.append(i18n("is less than"));
    sFilterFuncList.append(i18n("is greater than or equal to"));
  }

  //---------- initialize list of filter operators
  if ( sFilterFieldList.isEmpty() )
  {
    sFilterFieldList.append("");
    // also see KMSearchRule::matches() and ruleFieldToEnglish() if
    // you change the following i18n-ized strings!
    if( !headersOnly )  sFilterFieldList.append(i18n("<message>"));
    if( !headersOnly )  sFilterFieldList.append(i18n("<body>"));
    sFilterFieldList.append(i18n("<any header>"));
    sFilterFieldList.append(i18n("<recipients>"));
    sFilterFieldList.append(i18n("<size>"));
    sFilterFieldList.append(i18n("<age in days>"));
    // these others only represent message headers and you can add to
    // them as you like
    sFilterFieldList.append("Subject");
    sFilterFieldList.append("From");
    sFilterFieldList.append("To");
    sFilterFieldList.append("CC");
    sFilterFieldList.append("Reply-To");
    sFilterFieldList.append("List-Id");
    sFilterFieldList.append("Organization");
    sFilterFieldList.append("Resent-From");
    sFilterFieldList.append("X-Loop");
    sFilterFieldList.append("X-Mailing-List");
  }
}

//=============================================================================
//
// class KMFilterActionWidgetLister (the filter action editor)
//
//=============================================================================

KMSearchRuleWidgetLister::KMSearchRuleWidgetLister( QWidget *parent, const char* name, bool headersOnly )
  : KWidgetLister( 2, FILTER_MAX_RULES, parent, name )
{
  mRuleList = 0;
  mHeadersOnly = headersOnly;
}

KMSearchRuleWidgetLister::~KMSearchRuleWidgetLister()
{
}

void KMSearchRuleWidgetLister::setRuleList( QPtrList<KMSearchRule> *aList )
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
    kdDebug(5006) << "KMSearchRuleWidgetLister: Clipping rule list to "
		  << mMaxWidgets << " items!" << endl;

    for ( ; superfluousItems ; superfluousItems-- )
      mRuleList->removeLast();
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo( QMAX((int)mRuleList->count(),mMinWidgets) );

  // load the actions into the widgets
  QPtrListIterator<KMSearchRule> rIt( *mRuleList );
  QPtrListIterator<QWidget> wIt( mWidgetList );
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
  return new KMSearchRuleWidget(parent, 0, 0, mHeadersOnly);
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

  QPtrListIterator<QWidget> it( mWidgetList );
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

KMSearchPatternEdit::KMSearchPatternEdit(QWidget *parent, const char *name, bool headersOnly )
  : QGroupBox( 1/*columns*/, Horizontal, parent, name )
{
  setTitle( i18n("Search Criteria") );
  initLayout( headersOnly );
}

KMSearchPatternEdit::KMSearchPatternEdit(const QString & title, QWidget *parent, const char *name, bool headersOnly )
  : QGroupBox( 1/*column*/, Horizontal, title, parent, name )
{
  initLayout( headersOnly );
}

KMSearchPatternEdit::~KMSearchPatternEdit()
{
}

void KMSearchPatternEdit::initLayout(bool headersOnly)
{
  //------------the radio buttons
  mAllRBtn = new QRadioButton( i18n("Match a&ll of the following"), this, "mAllRBtn" );
  mAnyRBtn = new QRadioButton( i18n("Match an&y of the following"), this, "mAnyRBtn" );

  mAllRBtn->setChecked(TRUE);
  mAnyRBtn->setChecked(FALSE);

  QButtonGroup *bg = new QButtonGroup( this );
  bg->hide();
  bg->insert( mAllRBtn, (int)KMSearchPattern::OpAnd );
  bg->insert( mAnyRBtn, (int)KMSearchPattern::OpOr );

  //------------the list of KMSearchRuleWidget's
  mRuleLister = new KMSearchRuleWidgetLister( this, "swl", headersOnly );
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
    kdDebug(5006) << "KMSearchPatternEdit: no first KMSearchRuleWidget, though slotClear() has been called!" << endl;
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
