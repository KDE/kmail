// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include <config.h>

#include "kmatmlistview.h"

#include "kmmainwin.h"
#include "kmreadermainwin.h"
#include "messagesender.h"
#include "kmmsgpartdlg.h"
#include <kpgpblock.h>
#include <kaddrbook.h>
#include "kmaddrbook.h"
#include "kmmsgdict.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldercombobox.h"
#include "kmtransport.h"
#include "kmcommands.h"
#include "kcursorsaver.h"
#include "partNode.h"
#include "attachmentlistview.h"
#include "transportmanager.h"
using KMail::AttachmentListView;
#include "dictionarycombobox.h"
using KMail::DictionaryComboBox;
#include "addressesdialog.h"
using KPIM::AddressesDialog;
#include "addresseeemailselection.h"
using KPIM::AddresseeEmailSelection;
using KPIM::AddresseeSelectorDialog;
#include <maillistdrag.h>
using KPIM::MailListDrag;
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;
#include "kleo_util.h"
#include "stl_util.h"
#include "recipientseditor.h"

#include "attachmentcollector.h"
#include "objecttreeparser.h"

#include "kmfoldermaildir.h"

#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identitycombo.h>
#include <libkpimidentities/identity.h>
#include <libkdepim/kfileio.h>
#include <libemailfunctions/email.h>
#include <kleo/cryptobackendfactory.h>
#include <kleo/exportjob.h>
#include <ui/progressdialog.h>
#include <ui/keyselectiondialog.h>

#include <gpgmepp/context.h>
#include <gpgmepp/key.h>

#include <kabc/vcardconverter.h>
#include <libkdepim/kvcarddrag.h>
#include <kio/netaccess.h>


#include "klistboxdialog.h"

#include "messagecomposer.h"

#include <kcharsets.h>
#include <kcompletionbox.h>
#include <kcursor.h>
#include <kcombobox.h>
#include <kstdaccel.h>
#include <kmenu.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kwin.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kio/scheduler.h>
#include <ktempfile.h>
#include <klocale.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kdirwatch.h>
#include <kstdguiitem.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kuserprofile.h>
#include <krun.h>
#include <ktempdir.h>
//#include <keditlistbox.h>
#include "globalsettings.h"
#include "replyphrases.h"

#include <kspell.h>
#include <kspelldlg.h>
#include <spellingfilter.h>
#include <k3syntaxhighlighter.h>
#include <kcolordialog.h>
#include <kzip.h>
#include <ksavefile.h>

#include <q3tabdialog.h>
#include <qregexp.h>
#include <qbuffer.h>
#include <qtooltip.h>
#include <qtextcodec.h>
#include <q3header.h>
#include <q3whatsthis.h>
#include <qfontdatabase.h>

#include <mimelib/mimepp.h>

#include <algorithm>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

KMAtmListViewItem::KMAtmListViewItem(Q3ListView *parent)
  : QObject(),
    Q3ListViewItem( parent ),
    mListview( parent ),
    mCBSignEnabled( false ),
    mCBEncryptEnabled( false )
{
  mCBEncrypt = new QCheckBox( mListview->viewport() );
  mCBSign = new QCheckBox( mListview->viewport() );
  mCBCompress = new QCheckBox( mListview->viewport() );
  connect( mCBCompress, SIGNAL( clicked() ), this, SLOT( slotCompress() ) );

  mCBEncrypt->hide();
  mCBSign->hide();
}

KMAtmListViewItem::~KMAtmListViewItem()
{
  delete mCBEncrypt;
  mCBEncrypt = 0;
  delete mCBSign;
  mCBSign = 0;
  delete mCBCompress;
  mCBCompress = 0;
}

void KMAtmListViewItem::paintCell( QPainter * p, const QColorGroup & cg,
                                  int column, int width, int align )
{
  // this is also called for the encrypt/sign columns to assure that the
  // background is cleared
  Q3ListViewItem::paintCell( p, cg, column, width, align );
  if ( 4 == column ) {
    QRect r = mListview->itemRect( this );
    if ( !r.size().isValid() ) {
        mListview->ensureItemVisible( this );
        mListview->repaintContents( FALSE );
        r = mListview->itemRect( this );
    }
    int colWidth = mListview->header()->sectionSize( column );
    r.setX( mListview->header()->sectionPos( column )
            - mListview->header()->offset()
            + colWidth / 2
            - r.height() / 2
            - 1 );
    r.setY( r.y() + 1 );
    r.setWidth(  r.height() - 2 );
    r.setHeight( r.height() - 2 );
    r = QRect( mListview->viewportToContents( r.topLeft() ), r.size() );

    mCBCompress->resize( r.size() );
    mListview->moveChild( mCBCompress, r.x(), r.y() );

    QColor bg;
    if (isSelected())
      bg = cg.highlight();
    else
      bg = cg.base();

    mCBCompress->setPaletteBackgroundColor(bg);
    mCBCompress->show();
  }
  if( 5 == column || 6 == column ) {
    QRect r = mListview->itemRect( this );
    if ( !r.size().isValid() ) {
        mListview->ensureItemVisible( this );
        mListview->repaintContents( FALSE );
        r = mListview->itemRect( this );
    }
    int colWidth = mListview->header()->sectionSize( column );
    r.setX( mListview->header()->sectionPos( column )
            + colWidth / 2
            - r.height() / 2
            - 1 );
    r.setY( r.y() + 1 );
    r.setWidth(  r.height() - 2 );
    r.setHeight( r.height() - 2 );
    r = QRect( mListview->viewportToContents( r.topLeft() ), r.size() );

    QCheckBox* cb = (5 == column) ? mCBEncrypt : mCBSign;
    cb->resize( r.size() );
    mListview->moveChild( cb, r.x(), r.y() );

    QColor bg;
    if (isSelected())
      bg = cg.highlight();
    else
      bg = cg.base();

    bool enabled = (5 == column) ? mCBEncryptEnabled : mCBSignEnabled;
    cb->setPaletteBackgroundColor(bg);
    if (enabled) cb->show();
  }
}

void KMAtmListViewItem::enableCryptoCBs(bool on)
{
  if( mCBEncrypt ) {
    mCBEncryptEnabled = on;
    mCBEncrypt->setEnabled( on );
    mCBEncrypt->setVisible( on );
  }
  if( mCBSign ) {
    mCBSignEnabled = on;
    mCBSign->setEnabled( on );
    mCBSign->setVisible( on );
  }
}

void KMAtmListViewItem::setEncrypt(bool on)
{
  if( mCBEncrypt )
    mCBEncrypt->setChecked( on );
}

bool KMAtmListViewItem::isEncrypt()
{
  if( mCBEncrypt )
    return mCBEncrypt->isChecked();
  else
    return false;
}

void KMAtmListViewItem::setSign(bool on)
{
  if( mCBSign )
    mCBSign->setChecked( on );
}

bool KMAtmListViewItem::isSign()
{
  if( mCBSign )
    return mCBSign->isChecked();
  else
    return false;
}

void KMAtmListViewItem::setCompress(bool on)
{
  mCBCompress->setChecked( on );
}

bool KMAtmListViewItem::isCompress()
{
  return mCBCompress->isChecked();
}

void KMAtmListViewItem::slotCompress()
{
    if ( mCBCompress->isChecked() )
        emit compress( itemPos() );
    else
        emit uncompress( itemPos() );
}

#include "kmatmlistview.moc"
