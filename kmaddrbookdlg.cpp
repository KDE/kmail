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

#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>

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
}

void KMAddrBookSelDlg::readConfig()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, "General" );
  mCheckBox->setChecked( config->readBoolEntry("Show recent addresses in Addresses-dialog", false) );
}

void KMAddrBookSelDlg::saveConfig()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, "General" );
  config->writeEntry( "Show recent addresses in Addresses-dialog",
                      mCheckBox->isChecked() );
}

void KMAddrBookSelDlg::toggleShowRecent( bool on )
{
  showAddresses(AddressBookAddresses | (on ? RecentAddresses : 0));
  saveConfig();
}

void KMAddrBookSelDlg::showAddresses( int addressTypes )
{
  mListBox->clear();

  if ( addressTypes & AddressBookAddresses ) {
    if (KMAddrBookExternal::useKABC()) {
      QStringList addresses;
      KabcBridge::addresses(&addresses);
      mListBox->insertStringList(addresses);
    }
    else if (KMAddrBookExternal::useKAB()) {
      QStringList addresses;
      KabBridge::addresses(&addresses);
      mListBox->insertStringList(addresses);
    }
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
