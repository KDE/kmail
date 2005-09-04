#include "managesievescriptsdialog.h"
#include "managesievescriptsdialog_p.h"

#include "sieveconfig.h"
#include "accountmanager.h"
#include "imapaccountbase.h"
#include "sievejob.h"
#include "kmkernel.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kwin.h>
#include <kapplication.h>
#include <kinputdialog.h>
#include <kglobalsettings.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <q3listview.h>
#include <q3textedit.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QVBoxLayout>

#include <cassert>

inline Q3CheckListItem * qcli_cast( Q3ListViewItem * lvi ) {
  return lvi && lvi->rtti() == 1 ? static_cast<Q3CheckListItem*>( lvi ) : 0 ;
}
inline const Q3CheckListItem * qcli_cast( const Q3ListViewItem * lvi ) {
  return lvi && lvi->rtti() == 1 ? static_cast<const Q3CheckListItem*>( lvi ) : 0 ;
}

KMail::ManageSieveScriptsDialog::ManageSieveScriptsDialog( QWidget * parent, const char * name )
  : KDialogBase( Plain, i18n( "Manage Sieve Scripts" ), Ok|Close, Ok, parent, name, false ),
    mSieveEditor( 0 ),
    mContextMenuItem( 0 ),
    mWasActive( false )
{
  setWFlags( WGroupLeader|WDestructiveClose );
  KWin::setIcons( winId(), kapp->icon(), kapp->miniIcon() );

  QVBoxLayout * vlay = new QVBoxLayout( plainPage(), 0, 0 );

  mListView = new Q3ListView( plainPage() );
  mListView->addColumn( i18n( "Available Scripts" ) );
  mListView->setResizeMode( Q3ListView::LastColumn );
  mListView->setRootIsDecorated( true );
  mListView->setSelectionMode( Q3ListView::Single );
  connect( mListView, SIGNAL(contextMenuRequested(Q3ListViewItem*,const QPoint&,int)),
           this, SLOT(slotContextMenuRequested(Q3ListViewItem*, const QPoint&)) );
  connect( mListView, SIGNAL(doubleClicked(Q3ListViewItem*,const QPoint&,int)),
           this, SLOT(slotDoubleClicked(Q3ListViewItem*)) );
  connect( mListView, SIGNAL(selectionChanged(Q3ListViewItem*)),
           this, SLOT(slotSelectionChanged(Q3ListViewItem*)) );
  vlay->addWidget( mListView );

  resize( 2 * sizeHint().width(), sizeHint().height() );

  slotRefresh();
}

KMail::ManageSieveScriptsDialog::~ManageSieveScriptsDialog() {
  killAllJobs();
}

void KMail::ManageSieveScriptsDialog::killAllJobs() {
  for ( QMap<SieveJob*,Q3CheckListItem*>::const_iterator it = mJobs.constBegin(), end = mJobs.constEnd() ; it != end ; ++it )
    it.key()->kill();
  mJobs.clear();
}

static KURL findUrlForAccount( const KMail::ImapAccountBase * a ) {
  assert( a );
  const KMail::SieveConfig sieve = a->sieveConfig();
  if ( !sieve.managesieveSupported() )
    return KURL();
  if ( sieve.reuseConfig() ) {
    // assemble Sieve url from the settings of the account:
    KURL u;
    u.setProtocol( "sieve" );
    u.setHost( a->host() );
    u.setUser( a->login() );
    u.setPass( a->passwd() );
    u.setPort( sieve.port() );
    return u;
  } else {
    return sieve.alternateURL();
  }
}

void KMail::ManageSieveScriptsDialog::slotRefresh() {
  killAllJobs();
  mUrls.clear();
  mListView->clear();

  KMail::AccountManager * am = kmkernel->acctMgr();
  assert( am );
  Q3CheckListItem * last = 0;
  for ( KMAccount * a = am->first() ; a ; a = am->next() ) {
    last = new Q3CheckListItem( mListView, last, a->name(), Q3CheckListItem::Controller );
    last->setPixmap( 0, SmallIcon( "server" ) );
    if ( ImapAccountBase * iab = dynamic_cast<ImapAccountBase*>( a ) ) {
      const KURL u = ::findUrlForAccount( iab );
      if ( u.isEmpty() )
        continue;
      SieveJob * job = SieveJob::list( u );
      connect( job, SIGNAL(item(KMail::SieveJob*,const QString&,bool)),
               this, SLOT(slotItem(KMail::SieveJob*,const QString&,bool)) );
      connect( job, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
               this, SLOT(slotResult(KMail::SieveJob*,bool,const QString&,bool)) );
      mJobs.insert( job, last );
      mUrls.insert( last, u );
    } else {
      Q3ListViewItem * item = new Q3ListViewItem( last, i18n( "No Sieve URL configured" ) );
      item->setEnabled( false );
      last->setOpen( true );
    }
  }
}

void KMail::ManageSieveScriptsDialog::slotResult( KMail::SieveJob * job, bool success, const QString &, bool ) {
  Q3CheckListItem * parent = mJobs[job];
  if ( !parent )
    return;

  mJobs.remove( job );

  parent->setOpen( true );

  if ( success )
    return;

  Q3ListViewItem * item = new Q3ListViewItem( parent, i18n( "Failed to fetch the list of scripts" ) );
  item->setEnabled( false );
}

void KMail::ManageSieveScriptsDialog::slotItem( KMail::SieveJob * job, const QString & filename, bool isActive ) {
  Q3CheckListItem * parent = mJobs[job];
  if ( !parent )
    return;
  Q3CheckListItem * item = new Q3CheckListItem( parent, filename, Q3CheckListItem::RadioButton );
  if ( isActive ) {
    item->setOn( true );
    mSelectedItems[parent] = item;
  }
}

void KMail::ManageSieveScriptsDialog::slotContextMenuRequested( Q3ListViewItem * i, const QPoint & p ) {
  Q3CheckListItem * item = qcli_cast( i );
  if ( !item )
    return;
  if ( !item->depth() && !mUrls.count( item ) )
    return;
  Q3PopupMenu menu;
  mContextMenuItem = item;
  if ( item->depth() ) {
    // script items:
    menu.insertItem( i18n( "Delete Script" ), this, SLOT(slotDeleteScript()) );
    menu.insertItem( i18n( "Edit Script..." ), this, SLOT(slotEditScript()) );
  } else {
    // top-levels:
    menu.insertItem( i18n( "New Script..." ), this, SLOT(slotNewScript()) );
  }
  menu.exec( p );
  mContextMenuItem = 0;
}

void KMail::ManageSieveScriptsDialog::slotSelectionChanged( Q3ListViewItem * i ) {
  Q3CheckListItem * item = qcli_cast( i );
  if ( !item )
    return;
  Q3CheckListItem * parent = qcli_cast( item->parent() );
  if ( !parent )
    return;
  if ( item->isOn() && mSelectedItems[parent] != item ) {
    mSelectedItems[parent] = item;
    changeActiveScript( parent );
  }
}

void KMail::ManageSieveScriptsDialog::changeActiveScript( Q3CheckListItem * item ) {
  if ( !item )
    return;
  if ( !mUrls.count( item ) )
    return;
  if ( !mSelectedItems.count( item ) )
    return;
  KURL u = mUrls[item];
  if ( u.isEmpty() )
    return;
  Q3CheckListItem * selected = mSelectedItems[item];
  if ( !selected )
    return;
  u.setFileName( selected->text( 0 ) );

  SieveJob * job = SieveJob::activate( u );
  connect( job, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotRefresh()) );
}

void KMail::ManageSieveScriptsDialog::slotDoubleClicked( Q3ListViewItem * i ) {
  Q3CheckListItem * item = qcli_cast( i );
  if ( !item )
    return;
  if ( !item->depth() )
    return;
  mContextMenuItem = item;
  slotEditScript();
  mContextMenuItem = 0;
}

void KMail::ManageSieveScriptsDialog::slotDeleteScript() {
  if ( !mContextMenuItem )
    return;
  if ( !mContextMenuItem->depth() )
    return;

  Q3CheckListItem * parent = qcli_cast( mContextMenuItem->parent() );
  if ( !parent )
    return;

  if ( !mUrls.count( parent ) )
    return;

  KURL u = mUrls[parent];
  if ( u.isEmpty() )
    return;

  u.setFileName( mContextMenuItem->text( 0 ) );

  if ( KMessageBox::questionYesNo( this, i18n( "Really delete script \"%1\" from the server?" ).arg( u.fileName() ),
                                   i18n( "Delete Sieve Script Confirmation" ),
                                   KStdGuiItem::del(), KStdGuiItem::cancel() )
       != KMessageBox::Yes )
    return;

  SieveJob * job = SieveJob::del( u );
  connect( job, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotRefresh()) );
}

void KMail::ManageSieveScriptsDialog::slotEditScript() {
  if ( !mContextMenuItem )
    return;
  if ( !mContextMenuItem->depth() )
    return;
  Q3CheckListItem * parent = qcli_cast( mContextMenuItem->parent() );
  if ( !mUrls.count( parent ) )
    return;
  KURL url = mUrls[parent];
  if ( url.isEmpty() )
    return;
  url.setFileName( mContextMenuItem->text( 0 ) );
  mCurrentURL = url;
  SieveJob * job = SieveJob::get( url );
  connect( job, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotGetResult(KMail::SieveJob*,bool,const QString&,bool)) );
}

void KMail::ManageSieveScriptsDialog::slotNewScript() {
  if ( !mContextMenuItem )
    return;
  if ( mContextMenuItem->depth() )
    mContextMenuItem = qcli_cast( mContextMenuItem->parent() );
  if ( !mContextMenuItem )
    return;

  if ( !mUrls.count( mContextMenuItem ) )
    return;

  KURL u = mUrls[mContextMenuItem];
  if ( u.isEmpty() )
    return;

  bool ok = false;
  const QString name = KInputDialog::getText( i18n( "New Sieve Script" ),
                                              i18n( "Please enter a name for the new Sieve script:" ),
                                              i18n( "unnamed" ), &ok, this );
  if ( !ok || name.isEmpty() )
    return;

  u.setFileName( name );

  (void) new Q3CheckListItem( mContextMenuItem, name, Q3CheckListItem::RadioButton );

  mCurrentURL = u;
  slotGetResult( 0, true, QString::null, false );
}

KMail::SieveEditor::SieveEditor( QWidget * parent, const char * name )
  : KDialogBase( Plain, i18n( "Edit Sieve Script" ), Ok|Cancel, Ok, parent, name )
{
  QVBoxLayout * vlay = new QVBoxLayout( plainPage(), 0, spacingHint() );
  mTextEdit = new Q3TextEdit( plainPage() );
  vlay->addWidget( mTextEdit );
  mTextEdit->setTextFormat( Q3TextEdit::PlainText );
  mTextEdit->setWordWrap( Q3TextEdit::NoWrap );
  mTextEdit->setFont( KGlobalSettings::fixedFont() );

  resize( 3 * sizeHint() );
}

KMail::SieveEditor::~SieveEditor() {}

void KMail::ManageSieveScriptsDialog::slotGetResult( KMail::SieveJob *, bool success, const QString & script, bool isActive ) {
  if ( !success )
    return;

  if ( mSieveEditor )
    return;

  mSieveEditor = new SieveEditor( this );
  mSieveEditor->setScript( script );
  connect( mSieveEditor, SIGNAL(okClicked()), this, SLOT(slotSieveEditorOkClicked()) );
  connect( mSieveEditor, SIGNAL(cancelClicked()), this, SLOT(slotSieveEditorCancelClicked()) );
  mSieveEditor->show();
  mWasActive = isActive;
}

void KMail::ManageSieveScriptsDialog::slotSieveEditorOkClicked() {
  if ( !mSieveEditor )
    return;
  SieveJob * job = SieveJob::put( mCurrentURL,mSieveEditor->script(), mWasActive, mWasActive );
  connect( job, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotPutResult(KMail::SieveJob*,bool)) );
}

void KMail::ManageSieveScriptsDialog::slotSieveEditorCancelClicked() {
  mSieveEditor->deleteLater(); mSieveEditor = 0;
  mCurrentURL = KURL();
}

void KMail::ManageSieveScriptsDialog::slotPutResult( KMail::SieveJob *, bool success ) {
  if ( success ) {
    KMessageBox::information( this, i18n( "The Sieve script was successfully uploaded." ),
                              i18n( "Sieve Script Upload" ) );
    mSieveEditor->deleteLater(); mSieveEditor = 0;
    mCurrentURL = KURL();
  } else {
    mSieveEditor->show();
  }
}

#include "managesievescriptsdialog.moc"
#include "managesievescriptsdialog_p.moc"
