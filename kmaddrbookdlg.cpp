// kmaddrbookdlg.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kmkernel.h"
#include "kmrecentaddr.h"

#include <assert.h>

#include <qcheckbox.h>
#include <qlistview.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qheader.h>

#include <kapplication.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kconfig.h>


//-----------------------------------------------------------------------------
class KMAddrListViewItem : public KListViewItem
{
public:
  KMAddrListViewItem( KListView * parent )
    : KListViewItem( parent ) {};
  KMAddrListViewItem( KListView * parent, KListViewItem * after )
    : KListViewItem( parent, after ) {};
  KMAddrListViewItem( KListView * parent, QString str )
    : KListViewItem( parent, str ) {};
  KMAddrListViewItem( KListView * parent, KListViewItem * after, QString str )
    : KListViewItem( parent, after, str ) {};

  virtual QString key( int, bool ) const;
};

QString KMAddrListViewItem::key( int column, bool ) const
{
  // case insensitive sorting
  QString s = text( column ).lower();
  // ignore a leading " for sorting
  if( s[0] == '"' )
    s = s.mid(1);
  return s;
}

//-----------------------------------------------------------------------------
KMAddrBookSelDlg::KMAddrBookSelDlg(QWidget *parent, const QString& aCap):
  KDialogBase(parent, "addressbook", true,
              aCap.isEmpty() ? i18n("Addressbook") : aCap,
              Ok|Cancel, Ok, true)
{
  QVBox *page = makeVBoxMainWidget();

  mAddrListView = new KListView(page);
  mCheckBox = new QCheckBox(i18n("Show &recent addresses"), page);

  QString addr;
  mAddress  = QString::null;

  mAddrListView->addColumn( "" );
  mAddrListView->setSelectionMode(QListView::Multi);
  mAddrListView->setMinimumWidth(fontMetrics().maxWidth()*20);
  mAddrListView->setMinimumHeight(fontMetrics().lineSpacing()*15);
  mAddrListView->header()->hide();

  readConfig();

  connect(mAddrListView, SIGNAL(selected(int)), SLOT(slotOk()));
  connect(mCheckBox, SIGNAL(toggled(bool)), SLOT(toggleShowRecent(bool)));

  showAddresses(AddressBookAddresses |
                (mCheckBox->isChecked() ? RecentAddresses : 0));
}


//-----------------------------------------------------------------------------
KMAddrBookSelDlg::~KMAddrBookSelDlg()
{
  saveConfig();
}

void KMAddrBookSelDlg::readConfig()
{
  KConfigGroup general( kapp->config(), "General" );
  mCheckBox->setChecked( general.readBoolEntry("Show recent addresses in Addresses-dialog", false) );
  KConfigGroup geometry( kapp->config(), "Geometry" );
  if ( geometry.hasKey( "AddressBook dialog size" ) )
    resize( geometry.readSizeEntry( "AddressBook dialog size" ) );
}

void KMAddrBookSelDlg::saveConfig()
{
  KConfigGroup general( kapp->config(), "General" );
  general.writeEntry( "Show recent addresses in Addresses-dialog",
                      mCheckBox->isChecked() );
  KConfigGroup geometry( kapp->config(), "Geometry" );
  geometry.writeEntry( "AddressBook dialog size", size() );
}

void KMAddrBookSelDlg::toggleShowRecent( bool on )
{
  showAddresses(AddressBookAddresses | (on ? RecentAddresses : 0));
}

void KMAddrBookSelDlg::showAddresses( int addressTypes )
{
  mAddrListView->clear();
  mAddrListView->setSorting( 0 );

  if ( addressTypes & AddressBookAddresses ) {
    QStringList addresses;
    KabcBridge::addresses(&addresses);
    for( QStringList::Iterator it = addresses.begin();
         it != addresses.end(); ++it )
      new KMAddrListViewItem( mAddrListView, *it );
  }
  mAddrListView->sort();
  mAddrListView->setSorting( -1 );

  if ( addressTypes & RecentAddresses )
  {
    KMAddrListViewItem* prevlvi = 0;
    QStringList addresses = KMRecentAddresses::self()->addresses();
    for( QStringList::Iterator it = addresses.begin();
         it != addresses.end(); ++it )
    {
      if( prevlvi == 0 )
        prevlvi = new KMAddrListViewItem( mAddrListView, *it );
      else
        prevlvi = new KMAddrListViewItem( mAddrListView, prevlvi, *it );
    }
  }
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotOk()
{
  mAddress = QString::null;
  unsigned int count = 0;
  QListViewItemIterator it( mAddrListView );
  for ( ; it.current(); ++it )
  {
    if( it.current()->isSelected() )
    {
      if( count > 0 )
        mAddress += ", ";
      mAddress += it.current()->text( 0 );
      count++;
    }
  }
  KDialogBase::slotOk();
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotCancel()
{
  mAddress = QString::null;
  KDialogBase::slotCancel();
}


//-----------------------------------------------------------------------------
#include "kmaddrbookdlg.moc"
