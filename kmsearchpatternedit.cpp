// kmsearchpatternedit.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL

#include <config.h>
#include "kmsearchpatternedit.h"

#include "kmsearchpattern.h"

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>

#include <qpushbutton.h>
#include <qdialog.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qwidgetstack.h>

#include <assert.h>

//=============================================================================
//
// class KMSearchRuleWidget
//
//=============================================================================

KMSearchRuleWidget::KMSearchRuleWidget(QWidget *parent, KMSearchRule *aRule, const char *name, bool headersOnly, bool absoluteDates)
  : QHBox(parent,name),
    mRuleEditBut(0),
    mRegExpEditDialog(0)
{
  initLists( headersOnly, absoluteDates ); // sFilter{Func,Field}List are local to KMSearchRuleWidget
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
  mValueWidgetStack = new QWidgetStack( this, "mValueWidgetStack" );
  mRuleValue = new KLineEdit( this, "mRuleValue" );
  mStati = new QComboBox( false, this, "mStati" );
  mValueWidgetStack->addWidget( mRuleValue );
  mValueWidgetStack->addWidget( mStati );
  mValueWidgetStack->raiseWidget( mRuleValue );

  if( !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty() ) {
    mRuleEditBut = new QPushButton( i18n("Edit..."), this, "mRuleEditBut" );
    connect( mRuleEditBut, SIGNAL( clicked() ), this, SLOT( editRegExp()));
    connect( mRuleFunc, SIGNAL( activated(int) ), this, SLOT( functionChanged(int) ) );
    functionChanged( mRuleFunc->currentItem() );
  }

  mRuleFunc->insertStringList(mFilterFuncList);
  mRuleFunc->adjustSize();

  mRuleField->insertStringList(mFilterFieldList);
  // don't show sliders when popping up this menu
  mRuleField->setSizeLimit( mRuleField->count() );
  mRuleField->adjustSize();

  mStati->insertStringList( mStatiList );
  mStati->adjustSize();

  connect( mRuleField, SIGNAL(activated(int)),
	   this, SLOT(slotRuleChanged(int)) );
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
  //--------------special case status search
  if ( i == indexOfRuleField( "<status>" ) ) {
    mValueWidgetStack->raiseWidget( mStati );
    mStati->setCurrentItem( indexOfStatus( aRule->contents() ) );
  } else {
    mValueWidgetStack->raiseWidget( mRuleValue );
    mRuleValue->setText( aRule->contents() );
  }
  if (mRuleEditBut)
    functionChanged( (int)aRule->function() );

  blockSignals(FALSE);
}

KMSearchRule* KMSearchRuleWidget::rule() const {
  QCString ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  QString text = 0;
  if ( ruleField == "<status>" )
    text = statusToEnglish( mStati->currentText() );
  else
    text = mRuleValue->text();

  return KMSearchRule::createInstance( ruleField,
                     (KMSearchRule::Function)mRuleFunc->currentItem(), text );
}

void KMSearchRuleWidget::reset()
{
  blockSignals(TRUE);

  mRuleField->changeItem( "", 0 );
  mRuleField->setCurrentItem( 0 );

  mRuleFunc->setCurrentItem( 0 );
  if (mRuleEditBut)
      mRuleEditBut->setEnabled( false );
  mValueWidgetStack->raiseWidget( mRuleValue );
  mRuleValue->clear();
  mStati->setCurrentItem( 0 );

  blockSignals(FALSE);
}

QCString KMSearchRuleWidget::ruleFieldToEnglish(const QString & i18nVal) {
  if (i18nVal == i18n("<recipients>")) return "<recipients>";
  if (i18nVal == i18n("<body>")) return "<body>";
  if (i18nVal == i18n("<message>")) return "<message>";
  if (i18nVal == i18n("<any header>")) return "<any header>";
  if (i18nVal == i18n("<size in bytes>")) return "<size>";
  if (i18nVal == i18n("<age in days>")) return "<age in days>";
  if (i18nVal == i18n("<status>")) return "<status>";
  return i18nVal.latin1();
}

QCString KMSearchRuleWidget::statusToEnglish(const QString & i18nVal) {
  if (i18nVal == i18n("new")) return "new";
  if (i18nVal == i18n("unread")) return "unread";
  if (i18nVal == i18n("read")) return "read";
  if (i18nVal == i18n("old")) return "old";
  if (i18nVal == i18n("deleted")) return "deleted";
  if (i18nVal == i18n("replied")) return "replied";
  if (i18nVal == i18n("forwarded")) return "forwarded";
  if (i18nVal == i18n("queued")) return "queued";
  if (i18nVal == i18n("sent")) return "sent";
  if (i18nVal == i18n("important")) return "important";
  if (i18nVal == i18n("watched")) return "watched";
  if (i18nVal == i18n("ignored")) return "ignored";
  //if (i18nVal == i18n("todo")) return "todo";
  if (i18nVal == i18n("spam")) return "spam";
  return i18nVal.latin1();
}


int KMSearchRuleWidget::indexOfRuleField( const QString & aName ) const {
  int i;

  if ( aName.isEmpty() ) return -1;

  QString i18n_aName = i18n( aName.latin1() );

  for (i=mFilterFieldList.count()-1; i>=0; i--) {
    if (*(mFilterFieldList.at(i))==i18n_aName) break;
  }
  return i;
}

int KMSearchRuleWidget::indexOfStatus( const QString & aStatus ) const {
  int i;

  if ( aStatus.isEmpty() ) return -1;

  QString i18n_aName = i18n( aStatus.latin1() );

  for (i=mStatiList.count()-1; i>=0; i--) {
    if (*(mStatiList.at(i))==i18n_aName) break;
  }
  return i;
}


void KMSearchRuleWidget::initLists(bool headersOnly, bool absoluteDates)
{
  //---------- initialize list of filter operators
  if ( mFilterFuncList.isEmpty() )
  {
    // also see KMSearchRule::matches() and KMSearchRule::Function
    // if you change the following strings!
    mFilterFuncList.append(i18n("contains"));
    mFilterFuncList.append(i18n("doesn't contain"));
    mFilterFuncList.append(i18n("equals"));
    mFilterFuncList.append(i18n("doesn't equal"));
    mFilterFuncList.append(i18n("matches regular expr."));
    mFilterFuncList.append(i18n("doesn't match reg. expr."));
    mFilterFuncList.append(i18n("is greater than"));
    mFilterFuncList.append(i18n("is less than or equal to"));
    mFilterFuncList.append(i18n("is less than"));
    mFilterFuncList.append(i18n("is greater than or equal to"));
  }

  //---------- initialize list of filter operators
  if ( mFilterFieldList.isEmpty() )
  {
    mFilterFieldList.append("");
    // also see KMSearchRule::matches() and ruleFieldToEnglish() if
    // you change the following i18n-ized strings!
    if( !headersOnly )  mFilterFieldList.append(i18n("<message>"));
    if( !headersOnly )  mFilterFieldList.append(i18n("<body>"));
    mFilterFieldList.append(i18n("<any header>"));
    mFilterFieldList.append(i18n("<recipients>"));
    mFilterFieldList.append(i18n("<size in bytes>"));
    if ( !absoluteDates )  mFilterFieldList.append(i18n("<age in days>"));
    mFilterFieldList.append(i18n("<status>"));
    // these others only represent message headers and you can add to
    // them as you like
    mFilterFieldList.append("Subject");
    mFilterFieldList.append("From");
    mFilterFieldList.append("To");
    mFilterFieldList.append("CC");
    mFilterFieldList.append("Reply-To");
    mFilterFieldList.append("List-Id");
    mFilterFieldList.append("Organization");
    mFilterFieldList.append("Resent-From");
    mFilterFieldList.append("X-Loop");
    mFilterFieldList.append("X-Mailing-List");
    mFilterFieldList.append("X-Spam-Flag");
  }

  //---------- initialize list of stati
  if ( mStatiList.isEmpty() )
  {
    mStatiList.append( "" );
    mStatiList.append( i18n( "new" ) );
    mStatiList.append( i18n( "unread" ) );
    mStatiList.append( i18n( "read" ) );
    mStatiList.append( i18n( "old" ) );
    mStatiList.append( i18n( "deleted" ) );
    mStatiList.append( i18n( "replied" ) );
    mStatiList.append( i18n( "forwarded" ) );
    mStatiList.append( i18n( "queued" ) );
    mStatiList.append( i18n( "sent" ) );
    mStatiList.append( i18n( "important" ) );
    mStatiList.append( i18n( "watched" ) );
    mStatiList.append( i18n( "ignored" ) );
    //mStatiList.append( i18n( "todo" ) );
    mStatiList.append( i18n( "spam" ) );
    mStatiList.append( i18n( "ham" ) );
  }
}

void KMSearchRuleWidget::slotRuleChanged( int ruleIndex )
{
  if ( ruleIndex == indexOfRuleField( "<status>" ) ) {
      mRuleValue->clear();
      mValueWidgetStack->raiseWidget( mStati );
  } else {
      mStati->setCurrentItem( 0 );
      mValueWidgetStack->raiseWidget( mRuleValue );
  }
}


//=============================================================================
//
// class KMFilterActionWidgetLister (the filter action editor)
//
//=============================================================================

KMSearchRuleWidgetLister::KMSearchRuleWidgetLister( QWidget *parent, const char* name, bool headersOnly, bool absoluteDates )
  : KWidgetLister( 2, FILTER_MAX_RULES, parent, name )
{
  mRuleList = 0;
  mHeadersOnly = headersOnly;
  mAbsoluteDates = absoluteDates;
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

  // HACK to workaround regression in Qt 3.1.3 and Qt 3.2.0 (fixes bug #63537)
  setNumberOfShownWidgetsTo( QMAX((int)mRuleList->count(),mMinWidgets)+1 );
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
  return new KMSearchRuleWidget(parent, 0, 0, mHeadersOnly, mAbsoluteDates);
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

KMSearchPatternEdit::KMSearchPatternEdit(QWidget *parent, const char *name, bool headersOnly, bool absoluteDates )
  : QGroupBox( 1/*columns*/, Horizontal, parent, name )
{
  setTitle( i18n("Search Criteria") );
  initLayout( headersOnly, absoluteDates );
}

KMSearchPatternEdit::KMSearchPatternEdit(const QString & title, QWidget *parent, const char *name, bool headersOnly, bool absoluteDates)
  : QGroupBox( 1/*column*/, Horizontal, title, parent, name )
{
  initLayout( headersOnly, absoluteDates );
}

KMSearchPatternEdit::~KMSearchPatternEdit()
{
}

void KMSearchPatternEdit::initLayout(bool headersOnly, bool absoluteDates)
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
  mRuleLister = new KMSearchRuleWidgetLister( this, "swl", headersOnly, absoluteDates );
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
