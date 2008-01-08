/*
  Copyright (c) 2001 Heiko Hund <heiko@ist.eigentlich.net>
  Copyright (c) 2001 Thorsten Zachmann <T.Zachmann@zagge.de>
  Copyright (c) 2008 Thomas McGuire <Thomas.McGuire@gmx.net>

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

#include "kmpopfiltercnfrmdlg.h"

#include "kmmsgbase.h"
#include "kmmessage.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QSignalMapper>
#include <QTimer>
#include <QVBoxLayout>

#include <klocale.h>
#include <kio/global.h>

#include <kmime/kmime_dateformatter.h>

#include <assert.h>

KMPopHeadersView::KMPopHeadersView( QWidget *parent,
                                    KMPopFilterCnfrmDlg *dialog )
      : QTreeWidget( parent )
{
  assert( dialog );
  mDialog = dialog;

  QStringList headerNames;
  headerNames << "" << "" << "" << i18n("Subject") << i18n("Sender")
              << i18n("Receiver") << i18n("Date") << i18n("Size");
  QTreeWidgetItem *headerItem = new QTreeWidgetItem( headerNames );
  headerItem->setTextAlignment( 0, Qt::AlignHCenter );
  headerItem->setTextAlignment( 1, Qt::AlignHCenter );
  headerItem->setTextAlignment( 2, Qt::AlignHCenter );
  headerItem->setTextAlignment( 7, Qt::AlignRight );
  headerItem->setToolTip( 0, i18nc("@action:button", "Download all messages now") );
  headerItem->setToolTip( 1, i18nc("@action:button", "Download all messages later") );
  headerItem->setToolTip( 2, i18nc("@action:button", "Delete all messages") );
  headerItem->setIcon( 0, KIcon( "mail-download-now" ) );
  headerItem->setIcon( 1, KIcon( "mail-download-later" ) );
  headerItem->setIcon( 2, KIcon( "edit-delete" ) );
  setHeaderItem( headerItem );
  header()->setResizeMode( QHeaderView::Interactive );
  header()->setResizeMode( 0, QHeaderView::Fixed );
  header()->setResizeMode( 1, QHeaderView::Fixed );
  header()->setResizeMode( 2, QHeaderView::Fixed );
  //### Why is this so horribly broken?? Suddenly, a horizontal scrollbar
  //    is there, the sections can't be resized properly (other columns
  //    suddenly change size, resizing stops) etc.
  //    Disable it for now.
  //header()->setResizeMode( 3, QHeaderView::Stretch );
  header()->setStretchLastSection( false );
  setColumnWidth( 0, 22 );    // Download Now icon
  setColumnWidth( 1, 22 );    // Download Later icon
  setColumnWidth( 2, 22 );    // Delete icon
  setColumnWidth( 3, 180 );   // Subject
  setColumnWidth( 4, 140 );   // Sender
  setColumnWidth( 5, 140);    // Receiver
  setColumnWidth( 6, 130 );   // Date
  setColumnWidth( 7, 80 );    // Size
  setAllColumnsShowFocus( true );
  setIndentation( 0 );
  header()->setSortIndicator( 7, Qt::DescendingOrder );
  setSelectionMode( QAbstractItemView::ExtendedSelection );
  setAlternatingRowColors ( true );

  // We can not enable automatic sorting, because that would make the icon
  // columns sortable. We want to change the action of all items when an icon
  // column is clicked instead
  header()->setSortIndicatorShown( true );
  header()->setClickable( true );
  connect( header(), SIGNAL( sectionClicked(int) ),
           this, SLOT( slotSectionClicked(int) ) );

  // This code relies on the fact that radiobuttons are the first three
  // columns for easier Column-Action mapping it does not necessarily be
  // true - you could redefine mapToColumn and mapToAction to eg. shift
  // those numbers by 1
  assert( 0 == Down );
  assert( 1 == Later );
  assert( 2 == Delete );

  //we rely on fixed column order, so we forbid this
  header()->setMovable( false );
}

KMPopHeadersView::~KMPopHeadersView()
{
}

// Handle keystrokes - Left and Right key select previous/next
// action correspondingly
void KMPopHeadersView::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() != Qt::Key_Left && e->key() != Qt::Key_Right ) {
    QTreeWidget::keyPressEvent( e );
    return;
  }

  // Loop through all selected items change the selected action to the action
  // to the right/left if that key was pressed
  foreach ( QTreeWidgetItem *item, selectedItems() ) {

    KMPopHeadersViewItem *popItem
        = dynamic_cast<KMPopHeadersViewItem*>( item );

    assert( popItem );
    assert( mDialog );

    int newAction = popItem->action();
    if ( e->key() == Qt::Key_Left ) {
      // here we rely on the fact that the leftmost action is equal to 0!
      if ( popItem->action() )
        newAction = popItem->action() - 1;
    }
    else if ( e->key() == Qt::Key_Right ) {
      //here we rely on the fact that right most action is one less than NoAction!
      if ( popItem->action() < NoAction - 1 )
        newAction = popItem->action() + 1;
    }
    popItem->setAction( static_cast<KMPopFilterAction>( newAction ) );
    mDialog->setAction( item, popItem->action() );
    assert( popItem->action() >= 0 && popItem->action() < NoAction );
  }
}

void KMPopHeadersView::slotRadioButtonClicked( QTreeWidgetItem* item, int column ) {
  assert( item && column >= 0 && column < NoAction );
  mDialog->setAction( item, mapToAction( column ) );

  // If the user selected some items, but changed an unselected one, he probebly
  // doesn't want to change the actions of the selected ones
  if ( !item->isSelected() )
    return;

  // The user has selected at least one item and clicked on a radio button.
  // Toogle all radiobuttons of the other selected items as well. This way,
  // the user can quickly set many actions at once.
  foreach ( QTreeWidgetItem *selectedItem, selectedItems() ) {
    KMPopHeadersViewItem *popItem =
        dynamic_cast<KMPopHeadersViewItem*>( selectedItem );
    assert( popItem );
    popItem->setAction( mapToAction( column ) );
    mDialog->setAction( popItem, mapToAction( column ) );
  }
}

void KMPopHeadersView::slotSectionClicked( int column )
{
  // If a normal column was clicked, just sort that column
  if ( column > 2 ) {
    mLastSortColumn = header()->sortIndicatorSection();
    mLastSortOrder = header()->sortIndicatorOrder();
    sortByColumn( mLastSortColumn, mLastSortOrder );
  }

  // If one of the action columns was clicked, change the action of all items
  else {
    for ( int i = 0; i < topLevelItemCount(); i++ ) {
      KMPopHeadersViewItem *item =
          static_cast<KMPopHeadersViewItem*>( topLevelItem( i ) );
      item->setAction( mapToAction( column ) );
      mDialog->setAction( item, mapToAction( column ) );
    }

    // Reset the sort indicator Qt now incorrectly set to the icon column
    header()->setSortIndicator( mLastSortColumn, mLastSortOrder );
  }
}

KMPopHeadersViewItem::KMPopHeadersViewItem( KMPopHeadersView *parent,
                                            KMPopFilterAction action )
  : QTreeWidgetItem( parent )
{
  mParent = parent;
  mAction = NoAction;

  setToolTip( 0, i18nc("@info:tooltip", "Download Now") );
  setToolTip( 1, i18nc("@info:tooltip", "Download Later") );
  setToolTip( 2, i18nc("@info:tooltip", "Delete") );

  mActionGroup = new QButtonGroup( parent );
  mMapper = new QSignalMapper( parent );
  for( int column = 0; column <= 2; column++ ) {
    QRadioButton *button = addRadioButton( column );
    mActionGroup->addButton( button, column );
    // Don't accept focus, otherwise the right/left keys will incorrectly
    // change the action
    button->setFocusPolicy( Qt::NoFocus );
    connect( button, SIGNAL( clicked(bool) ), mMapper, SLOT( map() ) );
    mMapper->setMapping( button, column );
  }
  connect( mMapper, SIGNAL( mapped(int) ), this, SLOT( slotActionChanged(int) ) );

  setAction( action );
}

// Taken and adapted from QCheckBox* KMAtmListViewItem::addCheckBox.
QRadioButton* KMPopHeadersViewItem::addRadioButton( int column )
{
  // We can not call setItemWidget() on the radiobutton directly, because then
  // the checkbox would be left-aligned. Therefore we create a helper widget
  // with a layout to align the radiobutton to the center.
  QWidget *w = new QWidget( treeWidget() );
  QRadioButton *r = new QRadioButton();
  QHBoxLayout *l = new QHBoxLayout();
  l->insertWidget( 0, r, 0, Qt::AlignHCenter );
  w->setBackgroundRole( QPalette::Base );
  r->setBackgroundRole( QPalette::Base );
  w->setLayout( l );
  l->setMargin( 0 );
  l->setSpacing( 0 );
  w->show();
  treeWidget()->setItemWidget( this, column, w );
  return r;
}

KMPopHeadersViewItem::~KMPopHeadersViewItem()
{
}

void KMPopHeadersViewItem::slotActionChanged( int column )
{
  setAction( static_cast<KMPopFilterAction>( column ) );
  emit radioButtonClicked( this, column );
}

void KMPopHeadersViewItem::setAction( KMPopFilterAction action )
{
  if ( action != NoAction && action != mAction ) {
    if ( mAction != NoAction )
      mActionGroup->button( mParent->mapToColumn( mAction ) )->setChecked( false );

    mActionGroup->button( mParent->mapToColumn( action ) )->setChecked( true );
    mAction = action;
  }
}

bool KMPopHeadersViewItem::operator < ( const QTreeWidgetItem & other ) const
{
  switch( treeWidget()->sortColumn() ) {

    case 3: { // subject column
      const KMPopHeadersViewItem *otherItem =
          (static_cast<const KMPopHeadersViewItem*>( &other ));
      QString subject1 = KMMsgBase::skipKeyword( text( 3 ).toLower() );
      QString subject2 = KMMsgBase::skipKeyword( otherItem->text( 3 ).toLower() );
      return subject1 < subject2;
    }

    case 6: // date column
      return  mIsoDate <
          (static_cast<const KMPopHeadersViewItem*>( &other ))->mIsoDate;

    case 7: // size column
      return mSizeOfMessage <
          (static_cast<const KMPopHeadersViewItem*>( &other ))->mSizeOfMessage;

    default:
      return QTreeWidgetItem::operator < ( other );
  }
}

KMPopFilterCnfrmDlg::KMPopFilterCnfrmDlg( const QList<KMPopHeaders *> & headers,
                                          const QString & account,
                                          bool showLaterMsgs,
                                          QWidget * parent )
      : KDialog( parent )
{
  setUpdatesEnabled( false );
  setCaption( i18n("POP Filter") );
  setButtons( Ok | Help );
  setHelp( "popfilters" );
  unsigned int rulesetCount = 0;
  mShowLaterMsgs = showLaterMsgs;
  mLowerBoxVisible = false;

  QWidget *mainWidget = new QWidget( this );
  setMainWidget( mainWidget );

  QVBoxLayout *mainLayout = new QVBoxLayout( mainWidget );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( 0 );

  QLabel *infoLabel = new QLabel(
            i18n( "Messages to filter found on POP Account: <b>%1</b><p>"
                  "The messages shown exceed the maximum size limit you defined "
                  "for this account.<br />You can select what you want to do "
                  "with them by checking the appropriate button.</p>",
                  account ), mainWidget );
  mainLayout->addWidget( infoLabel );

  QGroupBox *upperBox = new QGroupBox( i18n("Messages Exceeding Size"), mainWidget );
  QVBoxLayout *upperBoxLayout = new QVBoxLayout( upperBox );
  upperBox->hide();
  KMPopHeadersView *upperHeadersView = new KMPopHeadersView( upperBox, this );
  upperBoxLayout->addWidget( upperHeadersView );
  mainLayout->addWidget( upperBox );

  QGroupBox *lowerBox = new QGroupBox( i18n("Ruleset Filtered Messages: none"),
                                       mainWidget );
  QVBoxLayout *lowerBoxLayout = new QVBoxLayout( lowerBox );
  QString checkBoxText(
      (showLaterMsgs) ?
      i18n("Show messages matched by a ruleset and tagged 'Download' or 'Delete'") :
      i18n("Show messages matched by a filter ruleset") );
  QCheckBox* cb = new QCheckBox( checkBoxText, lowerBox );
  cb->setEnabled( false );
  mFilteredHeaders = new KMPopHeadersView( lowerBox, this );
  mFilteredHeaders->hide();
  lowerBoxLayout->addWidget( cb );
  lowerBoxLayout->addWidget( mFilteredHeaders );
  mainLayout->addWidget( lowerBox );

  // fill the listviews with data from the headers
  for ( int i = 0; i < headers.count(); ++i ) {
    KMPopHeaders *header = headers[i];
    KMPopHeadersViewItem *lvi = 0;

    if ( header->ruleMatched() ) {
      if ( showLaterMsgs && header->action() == Later ) {
        // insert messages tagged 'later' only
        lvi = new KMPopHeadersViewItem( mFilteredHeaders, header->action() );
        mFilteredHeaders->addTopLevelItem( lvi );
        mFilteredHeaders->show();
        mLowerBoxVisible = true;
      }
      else if ( showLaterMsgs ) {
        // enable checkbox to show 'delete' and 'download' msgs
        // but don't insert them into the listview yet
        mDDLList.append( header );
        cb->setEnabled( true );
      }
      else if ( !showLaterMsgs ) {
        // insert all messaged tagged by a ruleset, enable
        // the checkbox, but don't show the listview yet
        lvi = new KMPopHeadersViewItem( mFilteredHeaders, header->action() );
        mFilteredHeaders->addTopLevelItem( lvi );
        cb->setEnabled( true );
      }
      rulesetCount++;
    }
    else {
      // insert all messages not tagged by a ruleset
      // into the upper listview
      lvi = new KMPopHeadersViewItem( upperHeadersView, header->action() );
      upperHeadersView->addTopLevelItem( lvi );
      upperBox->show();
    }

    if ( lvi ) {
      mItemMap[lvi] = header;
      setupLVI( lvi, header->header() );
    }
  }

  // Initally sort the columns of the treewidgets by size
  upperHeadersView->slotSectionClicked( 7 );
  mFilteredHeaders->slotSectionClicked( 7 );

  if ( rulesetCount )
    lowerBox->setTitle( i18n("Ruleset Filtered Messages: %1", rulesetCount ) );

  // connect signals and slots
  connect( cb, SIGNAL( toggled(bool) ),
           this, SLOT( slotToggled(bool) ) );

  resize( 800, 600 );
  setUpdatesEnabled( true );
}

KMPopFilterCnfrmDlg::~KMPopFilterCnfrmDlg()
{
}

void KMPopFilterCnfrmDlg::setupLVI( KMPopHeadersViewItem *lvi, KMMessage *msg )
{
  // set the subject
  QString tmp = msg->subject();
  if( tmp.isEmpty() )
    tmp = i18n("No Subject");
  lvi->setText( 3, tmp );
  lvi->setToolTip( 3, tmp );

  // set the sender
  tmp = msg->fromStrip();
  if( tmp.isEmpty() )
    tmp = i18n("Unknown");
  lvi->setText( 4, tmp );
  lvi->setToolTip( 4, tmp );

  // set the receiver
  tmp = msg->toStrip();
  if( tmp.isEmpty() )
    tmp = i18n("Unknown");
  lvi->setText( 5, tmp );
  lvi->setToolTip( 5, tmp );

  // set the date
  lvi->setText( 6, KMime::DateFormatter::formatDate(
                                   KMime::DateFormatter::Fancy, msg->date() ) );
  lvi->setIsoDate( msg->dateIsoStr() );

  // set the size
  lvi->setText( 7, KIO::convertSize( msg->msgLength() ) );
  lvi->setMessageSize( msg->msgLength() );

  connect( lvi, SIGNAL( radioButtonClicked(QTreeWidgetItem*, int) ),
           lvi->treeWidget(), SLOT( slotRadioButtonClicked(QTreeWidgetItem*,int) ) );
}

void KMPopFilterCnfrmDlg::setAction( QTreeWidgetItem *item,
                                     KMPopFilterAction action )
{
  mItemMap[item]->setAction( action );
}

void KMPopFilterCnfrmDlg::slotToggled( bool on )
{
  setUpdatesEnabled( false );
  if ( on ) {
    if ( mShowLaterMsgs ) {
      // show download and deletek msgs in the list view too
      for ( int i = 0; i < mDDLList.count(); ++i ) {
        KMPopHeaders *headers = mDDLList[i];
        KMPopHeadersViewItem *lvi =
            new KMPopHeadersViewItem( mFilteredHeaders, headers->action() );
        mFilteredHeaders->addTopLevelItem( lvi );
        mItemMap[lvi] = headers;
        mDelList.append( lvi );
        setupLVI( lvi, headers->header() );
      }
    }

    if ( !mLowerBoxVisible ) {
      mFilteredHeaders->show();
    }
  }
  else {
    if ( mShowLaterMsgs ) {
      // delete download and delete msgs from the lower listview
      for ( int i = 0; i < mDelList.count(); ++i ) {
        delete mDelList[i];
      }
      mDelList.clear();
    }

    if ( !mLowerBoxVisible ) {
      mFilteredHeaders->hide();
    }
  }
  setUpdatesEnabled( true );
}

#include "kmpopfiltercnfrmdlg.moc"
