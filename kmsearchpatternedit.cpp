// -*- mode: C++; c-file-style: "gnu" -*-
// kmsearchpatternedit.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL

#include <config.h>
#include "kmsearchpatternedit.h"

#include "kmsearchpattern.h"
#include "rulewidgethandlermanager.h"
using KMail::RuleWidgetHandlerManager;

#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>

#include <qradiobutton.h>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qwidgetstack.h>
#include <qlayout.h>

#include <assert.h>

// Definition of special rule field strings
// Note: Also see KMSearchRule::matches() and ruleFieldToEnglish() if
//       you change the following i18n-ized strings!
// Note: The index of the values in the following array has to correspond to
//       the value of the entries in the enum in KMSearchRuleWidget.
static const struct {
  const char *internalName;
  const char *displayName;
} SpecialRuleFields[] = {
  { "<message>",     I18N_NOOP( "<message>" )       },
  { "<body>",        I18N_NOOP( "<body>" )          },
  { "<any header>",  I18N_NOOP( "<any header>" )    },
  { "<recipients>",  I18N_NOOP( "<recipients>" )    },
  { "<size>",        I18N_NOOP( "<size in bytes>" ) },
  { "<age in days>", I18N_NOOP( "<age in days>" )   },
  { "<status>",      I18N_NOOP( "<status>" )        }
};
static const int SpecialRuleFieldsCount =
  sizeof( SpecialRuleFields ) / sizeof( *SpecialRuleFields );

//=============================================================================
//
// class KMSearchRuleWidget
//
//=============================================================================

KMSearchRuleWidget::KMSearchRuleWidget( QWidget *parent, KMSearchRule *aRule,
                                        const char *name, bool headersOnly,
                                        bool absoluteDates )
  : QWidget( parent, name ),
    mRuleField( 0 ),
    mFunctionStack( 0 ),
    mValueStack( 0 ),
    mAbsoluteDates( absoluteDates )
{
  initFieldList( headersOnly, absoluteDates );
  initWidget();

  if ( aRule )
    setRule( aRule );
  else
    reset();
}

void KMSearchRuleWidget::setHeadersOnly( bool headersOnly )
{
  QCString currentText = rule()->field();
  initFieldList( headersOnly, mAbsoluteDates );
  
  mRuleField->clear();
  mRuleField->insertStringList( mFilterFieldList );
  mRuleField->setSizeLimit( mRuleField->count() );
  mRuleField->adjustSize();

  if ((currentText != "<message>") &&
      (currentText != "<body>"))
    mRuleField->changeItem( QString::fromAscii( currentText ), 0 );
  else
    mRuleField->changeItem( QString::null, 0 );
}

void KMSearchRuleWidget::initWidget()
{
  QHBoxLayout * hlay = new QHBoxLayout( this, 0, KDialog::spacingHint() );

  // initialize the header field combo box
  mRuleField = new QComboBox( true, this, "mRuleField" );
  mRuleField->insertStringList( mFilterFieldList );
  // don't show sliders when popping up this menu
  mRuleField->setSizeLimit( mRuleField->count() );
  mRuleField->adjustSize();
  hlay->addWidget( mRuleField );

  // initialize the function/value widget stack
  mFunctionStack = new QWidgetStack( this, "mFunctionStack" );
	//Don't expand the widget in vertical direction
	mFunctionStack->setSizePolicy( QSizePolicy::Preferred,QSizePolicy::Fixed );

  hlay->addWidget( mFunctionStack );

  mValueStack = new QWidgetStack( this, "mValueStack" );
	mValueStack->setSizePolicy( QSizePolicy::Preferred,QSizePolicy::Fixed );
  hlay->addWidget( mValueStack );
  hlay->setStretchFactor( mValueStack, 10 );

  RuleWidgetHandlerManager::instance()->createWidgets( mFunctionStack,
                                                       mValueStack,
                                                       this );

  // redirect focus to the header field combo box
  setFocusProxy( mRuleField );

  connect( mRuleField, SIGNAL( activated( const QString & ) ),
	   this, SLOT( slotRuleFieldChanged( const QString & ) ) );
  connect( mRuleField, SIGNAL( textChanged( const QString & ) ),
	   this, SLOT( slotRuleFieldChanged( const QString & ) ) );
  connect( mRuleField, SIGNAL( textChanged( const QString & ) ),
           this, SIGNAL( fieldChanged( const QString & ) ) );
}

void KMSearchRuleWidget::setRule( KMSearchRule *aRule )
{
  assert ( aRule );

  //kdDebug(5006) << "KMSearchRuleWidget::setRule( "
  //              << aRule->asString() << " )" << endl;

  //--------------set the field
  int i = indexOfRuleField( aRule->field() );

  mRuleField->blockSignals( true );

  if ( i < 0 ) { // not found -> user defined field
    mRuleField->changeItem( QString::fromLatin1( aRule->field() ), 0 );
    i = 0;
  } else { // found in the list of predefined fields
    mRuleField->changeItem( QString::null, 0 );
  }

  mRuleField->setCurrentItem( i );
  mRuleField->blockSignals( false );

  RuleWidgetHandlerManager::instance()->setRule( mFunctionStack, mValueStack,
                                                 aRule );
}

KMSearchRule* KMSearchRuleWidget::rule() const {
  const QCString ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  const KMSearchRule::Function function =
    RuleWidgetHandlerManager::instance()->function( ruleField,
                                                    mFunctionStack );
  const QString value =
    RuleWidgetHandlerManager::instance()->value( ruleField, mFunctionStack,
                                                 mValueStack );

  return KMSearchRule::createInstance( ruleField, function, value );
}

void KMSearchRuleWidget::reset()
{
  mRuleField->blockSignals( true );
  mRuleField->changeItem( "", 0 );
  mRuleField->setCurrentItem( 0 );
  mRuleField->blockSignals( false );

  RuleWidgetHandlerManager::instance()->reset( mFunctionStack, mValueStack );
}

void KMSearchRuleWidget::slotFunctionChanged()
{
  const QCString ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  RuleWidgetHandlerManager::instance()->update( ruleField,
                                                mFunctionStack,
                                                mValueStack );
}

void KMSearchRuleWidget::slotValueChanged()
{
  const QCString ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  const QString prettyValue =
    RuleWidgetHandlerManager::instance()->prettyValue( ruleField,
                                                       mFunctionStack,
                                                       mValueStack );
  emit contentsChanged( prettyValue );
}

QCString KMSearchRuleWidget::ruleFieldToEnglish( const QString & i18nVal )
{
  for ( int i = 0; i < SpecialRuleFieldsCount; ++i ) {
    if ( i18nVal == i18n( SpecialRuleFields[i].displayName ) )
      return SpecialRuleFields[i].internalName;
  }
  return i18nVal.latin1();
}

int KMSearchRuleWidget::ruleFieldToId( const QString & i18nVal )
{
  for ( int i = 0; i < SpecialRuleFieldsCount; ++i ) {
    if ( i18nVal == i18n( SpecialRuleFields[i].displayName ) )
      return i;
  }
  return -1; // no pseudo header
}

int KMSearchRuleWidget::indexOfRuleField( const QCString & aName ) const
{
  if ( aName.isEmpty() )
    return -1;

  QString i18n_aName = i18n( aName );

  for ( int i = 1; i < mRuleField->count(); ++i ) {
    if ( mRuleField->text( i ) == i18n_aName )
      return i;
  }

  return -1;
}

void KMSearchRuleWidget::initFieldList( bool headersOnly, bool absoluteDates )
{
  mFilterFieldList.clear();
  mFilterFieldList.append(""); // empty entry for user input
  if( !headersOnly ) {
    mFilterFieldList.append( i18n( SpecialRuleFields[Message].displayName ) );
    mFilterFieldList.append( i18n( SpecialRuleFields[Body].displayName ) );
  }
  mFilterFieldList.append( i18n( SpecialRuleFields[AnyHeader].displayName ) );
  mFilterFieldList.append( i18n( SpecialRuleFields[Recipients].displayName ) );
  mFilterFieldList.append( i18n( SpecialRuleFields[Size].displayName ) );
  if ( !absoluteDates )
    mFilterFieldList.append( i18n( SpecialRuleFields[AgeInDays].displayName ) );
  mFilterFieldList.append( i18n( SpecialRuleFields[Status].displayName ) );
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

void KMSearchRuleWidget::slotRuleFieldChanged( const QString & field )
{
  RuleWidgetHandlerManager::instance()->update( ruleFieldToEnglish( field ),
                                                mFunctionStack,
                                                mValueStack );
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
    (static_cast<KMSearchRuleWidget*>(*wIt))->setRule( (*rIt) );
  }
  for ( ; wIt.current() ; ++wIt )
    ((KMSearchRuleWidget*)(*wIt))->reset();

  assert( mWidgetList.first() );
  mWidgetList.first()->blockSignals(FALSE);
}

void KMSearchRuleWidgetLister::setHeadersOnly( bool headersOnly )
{
  QPtrListIterator<QWidget> wIt( mWidgetList );
  for ( wIt.toFirst() ; wIt.current() ; ++wIt ) {
    (static_cast<KMSearchRuleWidget*>(*wIt))->setHeadersOnly( headersOnly );
  }
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

void KMSearchPatternEdit::setHeadersOnly( bool headersOnly )
{
  mRuleLister->setHeadersOnly( headersOnly );
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
