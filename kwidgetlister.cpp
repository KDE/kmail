// kwidgetliaster.cpp
// A class to show multiple widgets in rows
// and allowing to add and remove them at/from the end.
// Author: Marc Mutz <Marc@Mutz.com>

#include "kwidgetlister.h"

#include <klocale.h>
#include <kdebug.h>

#include <qpushbutton.h>
#include <qlayout.h>
#include <qhbox.h>

#include <assert.h>

KWidgetLister::KWidgetLister( int minWidgets, int maxWidgets, QWidget *parent, const char* name )
  : QWidget( parent, name )
{
  mWidgetList.setAutoDelete(TRUE);

  mMinWidgets = QMAX( minWidgets, 1 );
  mMaxWidgets = QMAX( maxWidgets, mMinWidgets + 1 );

  //--------- the button box
  mLayout = new QVBoxLayout(this, 0, 4);
  mButtonBox = new QHBox(this);
  mLayout->addWidget( mButtonBox );

  mBtnMore = new QPushButton( i18n("more widgets","More"), mButtonBox );
  mButtonBox->setStretchFactor( mBtnMore, 0 );

  mBtnFewer = new QPushButton( i18n("fewer widgets","Fewer"), mButtonBox );
  mButtonBox->setStretchFactor( mBtnFewer, 0 );

  QWidget *spacer = new QWidget( mButtonBox );
  mButtonBox->setStretchFactor( spacer, 1 );

  mBtnClear = new QPushButton( i18n("clear widgets","Clear"), mButtonBox );
  mButtonBox->setStretchFactor( mBtnClear, 0 );

  //---------- connect everything
  connect( mBtnMore, SIGNAL(clicked()),
	   this, SLOT(slotMore()) );
  connect( mBtnFewer, SIGNAL(clicked()),
	   this, SLOT(slotFewer()) );
  connect( mBtnClear, SIGNAL(clicked()),
	   this, SLOT(slotClear()) );

  enableControls();
}

KWidgetLister::~KWidgetLister()
{
}

void KWidgetLister::slotMore()
{
  // the class should make certain that slotMore can't
  // be called when mMaxWidgets are on screen.
  assert( (int)mWidgetList.count() < mMaxWidgets );

  addWidgetAtEnd();
  //  adjustSize();
  enableControls();
}

void KWidgetLister::slotFewer()
{
  // the class should make certain that slotFewer can't
  // be called when mMinWidgets are on screen.
  assert( (int)mWidgetList.count() > mMinWidgets );

  removeLastWidget();
  //  adjustSize();
  enableControls();
}

void KWidgetLister::slotClear()
{
  setNumberOfShownWidgetsTo( mMinWidgets );

  // clear remaining widgets
  QListIterator<QWidget> it( mWidgetList );
  for ( it.toFirst() ; it.current() ; ++it )
    clearWidget( (*it) );

  //  adjustSize();
  enableControls();
}

void KWidgetLister::addWidgetAtEnd()
{
  QWidget *w = this->createWidget(this);

  mLayout->insertWidget( mLayout->findWidget( mButtonBox ), w );
  mWidgetList.append( w );
  w->show();
}

void KWidgetLister::removeLastWidget()
{
  // The layout will take care that the
  // widget is removed from screen, too.
  mWidgetList.removeLast();
}

void KWidgetLister::clearWidget( QWidget* /*aWidget*/ )
{
}

QWidget* KWidgetLister::createWidget( QWidget* parent )
{
  return new QWidget( parent );
}

void KWidgetLister::setNumberOfShownWidgetsTo( int aNum )
{
  int superfluousWidgets = QMAX( (int)mWidgetList.count() - aNum, 0 );
  int missingWidgets     = QMAX( aNum - (int)mWidgetList.count(), 0 );

  // remove superfluous widgets
  for ( ; superfluousWidgets ; superfluousWidgets-- )
    removeLastWidget();

  // add missing widgets
  for ( ; missingWidgets ; missingWidgets-- )
    addWidgetAtEnd();
}

void KWidgetLister::enableControls()
{
  int count = mWidgetList.count();
  bool isMaxWidgets = ( count >= mMaxWidgets );
  bool isMinWidgets = ( count <= mMinWidgets );

  mBtnMore->setEnabled( !isMaxWidgets );
  mBtnFewer->setEnabled( !isMinWidgets );
}

#include "kwidgetlister.moc"
