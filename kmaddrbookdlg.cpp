// kmaddrbookdlg.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kmkernel.h"
#include "kmrecentaddr.h"

#include <assert.h>

#include <qcheckbox.h>
#include <qvbox.h>
#include <qpushbutton.h>

#include <kapplication.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>

//-----------------------------------------------------------------------------
KMAddrBookSelDlg::KMAddrBookSelDlg(QWidget *parent, const QString& aCap):
  KDialogBase(parent, "addressbook", true,
              aCap.isEmpty() ? i18n("Addressbook") : aCap,
              Ok|Cancel, Ok, true)
{
  QVBox *page = makeVBoxMainWidget();

  mListBox = new QListBox(page);
  mCheckBox = new QCheckBox(i18n("Show &recent addresses"), page);

  QString addr;
  mAddress  = QString::null;

  mListBox->setSelectionMode(QListBox::Multi);
  mListBox->setMinimumWidth(fontMetrics().maxWidth()*20);
  mListBox->setMinimumHeight(fontMetrics().lineSpacing()*15);

  readConfig();

  connect(mListBox, SIGNAL(selected(int)), SLOT(slotOk()));
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
  mListBox->clear();

  if ( addressTypes & AddressBookAddresses ) {
    QStringList addresses;
    KabcBridge::addresses(&addresses);
    mListBox->insertStringList(addresses);
  }
  mListBox->sort();

  if ( addressTypes & RecentAddresses )
    mListBox->insertStringList( KMRecentAddresses::self()->addresses(), 0 );
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotOk()
{
  mAddress = QString::null;
  unsigned int idx;
  unsigned int count = 0;
  for (idx = 0; idx < mListBox->count(); idx++)
  {
    if( mListBox->isSelected(idx) ) {
      if( count > 0 ) {
        mAddress += ", ";
      }
      mAddress += mListBox->text(idx);
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
