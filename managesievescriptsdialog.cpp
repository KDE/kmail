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
#include <qlistview.h>
#include <qtextedit.h>
#include <qpopupmenu.h>

#include <cassert>

inline QCheckListItem * qcli_cast( QListViewItem * lvi ) {
  return lvi && lvi->rtti() == 1 ? static_cast<QCheckListItem*>( lvi ) : 0 ;
}
inline const QCheckListItem * qcli_cast( const QListViewItem * lvi ) {
  return lvi && lvi->rtti() == 1 ? static_cast<const QCheckListItem*>( lvi ) : 0 ;
}

KMail::ManageSieveScriptsDialog::ManageSieveScriptsDialog( QWidget * parent, const char * name )
  : KDialogBase( Plain, i18n( "Manage Sieve Scripts" ), Close, Close,
    parent, name, false ),
    mSieveEditor( 0 ),
    mContextMenuItem( 0 ),
    mWasActive( false )
{
  setWFlags( WGroupLeader|WDestructiveClose );
  KWin::setIcons( winId(), kapp->icon(), kapp->miniIcon() );

  QVBoxLayout * vlay = new QVBoxLayout( plainPage(), 0, 0 );

  mListView = new QListView( plainPage() );
  mListView->addColumn( i18n( "Available Scripts" ) );
  mListView->setResizeMode( QListView::LastColumn );
  mListView->setRootIsDecorated( true );
  mListView->setSelectionMode( QListView::Single );
  connect( mListView, SIGNAL(contextMenuRequested(QListViewItem*,const QPoint&,int)),
           this, SLOT(slotContextMenuRequested(QListViewItem*, const QPoint&)) );
  connect( mListView, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)),
           this, SLOT(slotDoubleClicked(QListViewItem*)) );
  connect( mListView, SIGNAL(selectionChanged(QListViewItem*)),
           this, SLOT(slotSelectionChanged(QListViewItem*)) );
  vlay->addWidget( mListView );

  resize( 2 * sizeHint().width(), sizeHint().height() );

  slotRefresh();
}

KMail::ManageSieveScriptsDialog::~ManageSieveScriptsDialog() {
  killAllJobs();
}

void KMail::ManageSieveScriptsDialog::killAllJobs() {
  for ( QMap<SieveJob*,QCheckListItem*>::const_iterator it = mJobs.constBegin(), end = mJobs.constEnd() ; it != end ; ++it )
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
    // Translate IMAP LOGIN to PLAIN:
    u.addQueryItem( "x-mech", a->auth() == "*" ? "PLAIN" : a->auth() );
    if ( !a->useSSL() && !a->useTLS() )
        u.addQueryItem( "x-allow-unencrypted", "true" );
    return u;
  } else {
    KURL u = sieve.alternateURL();
    if ( u.protocol().lower() == "sieve" && !a->useSSL() && !a->useTLS() && u.queryItem("x-allow-unencrypted").isEmpty() )
        u.addQueryItem( "x-allow-unencrypted", "true" );
    return u;
  }
}

void KMail::ManageSieveScriptsDialog::slotRefresh() {
  killAllJobs();
  mUrls.clear();
  mListView->clear();

  KMail::AccountManager * am = kmkernel->acctMgr();
  assert( am );
  QCheckListItem * last = 0;
  for ( KMAccount * a = am->first() ; a ; a = am->next() ) {
    last = new QCheckListItem( mListView, last, a->name(), QCheckListItem::Controller );
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
      QListViewItem * item = new QListViewItem( last, i18n( "No Sieve URL configured" ) );
      item->setEnabled( false );
      last->setOpen( true );
    }
  }
}

void KMail::ManageSieveScriptsDialog::slotResult( KMail::SieveJob * job, bool success, const QString &, bool ) {
  QCheckListItem * parent = mJobs[job];
  if ( !parent )
    return;

  mJobs.remove( job );

  parent->setOpen( true );

  if ( success )
    return;

  QListViewItem * item = new QListViewItem( parent, i18n( "Failed to fetch the list of scripts" ) );
  item->setEnabled( false );
}

void KMail::ManageSieveScriptsDialog::slotItem( KMail::SieveJob * job, const QString & filename, bool isActive ) {
  QCheckListItem * parent = mJobs[job];
  if ( !parent )
    return;
  QCheckListItem * item = new QCheckListItem( parent, filename, QCheckListItem::RadioButton );
  if ( isActive ) {
    item->setOn( true );
    mSelectedItems[parent] = item;
  }
}

void KMail::ManageSieveScriptsDialog::slotContextMenuRequested( QListViewItem * i, const QPoint & p ) {
  QCheckListItem * item = qcli_cast( i );
  if ( !item )
    return;
  if ( !item->depth() && !mUrls.count( item ) )
    return;
  QPopupMenu menu;
  mContextMenuItem = item;
  if ( item->depth() ) {
    // script items:
    menu.insertItem( i18n( "Delete Script" ), this, SLOT(slotDeleteScript()) );
    menu.insertItem( i18n( "Edit Script..." ), this, SLOT(slotEditScript()) );
    menu.insertItem( i18n( "Desactivate Script" ), this, SLOT(slotDesactivateScript()) );
  } else {
    // top-levels:
    menu.insertItem( i18n( "New Script..." ), this, SLOT(slotNewScript()) );
  }
  menu.exec( p );
  mContextMenuItem = 0;
}


void KMail::ManageSieveScriptsDialog::slotDesactivateScript() {
  if ( !mContextMenuItem )
    return;

  QCheckListItem * parent = qcli_cast( mContextMenuItem->parent() );
  if ( !parent )
    return;
  if ( mContextMenuItem->isOn()) {
    mSelectedItems[parent] = mContextMenuItem;
    changeActiveScript( parent,false );
  }
}

void KMail::ManageSieveScriptsDialog::slotSelectionChanged( QListViewItem * i ) {
  QCheckListItem * item = qcli_cast( i );
  if ( !item )
    return;
  QCheckListItem * parent = qcli_cast( item->parent() );
  if ( !parent )
    return;
  if ( item->isOn() && mSelectedItems[parent] != item ) {
    mSelectedItems[parent] = item;
    changeActiveScript( parent,true );
  }
}

void KMail::ManageSieveScriptsDialog::changeActiveScript( QCheckListItem * item , bool activate) {
  if ( !item )
    return;
  if ( !mUrls.count( item ) )
    return;
  if ( !mSelectedItems.count( item ) )
    return;
  KURL u = mUrls[item];
  if ( u.isEmpty() )
    return;
  QCheckListItem * selected = mSelectedItems[item];
  if ( !selected )
    return;
  u.setFileName( selected->text( 0 ) );
  SieveJob * job;
  if ( activate )
    job = SieveJob::activate( u );
  else
    job = SieveJob::desactivate( u );
  connect( job, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotRefresh()) );
}

void KMail::ManageSieveScriptsDialog::slotDoubleClicked( QListViewItem * i ) {
  QCheckListItem * item = qcli_cast( i );
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

  QCheckListItem * parent = qcli_cast( mContextMenuItem->parent() );
  if ( !parent )
    return;

  if ( !mUrls.count( parent ) )
    return;

  KURL u = mUrls[parent];
  if ( u.isEmpty() )
    return;

  u.setFileName( mContextMenuItem->text( 0 ) );

  if ( KMessageBox::warningContinueCancel( this, i18n( "Really delete script \"%1\" from the server?" ).arg( u.fileName() ),
                                   i18n( "Delete Sieve Script Confirmation" ),
                                   KStdGuiItem::del() )
       != KMessageBox::Continue )
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
  QCheckListItem * parent = qcli_cast( mContextMenuItem->parent() );
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

  (void) new QCheckListItem( mContextMenuItem, name, QCheckListItem::RadioButton );

  mCurrentURL = u;
  slotGetResult( 0, true, QString::null, false );
}

KMail::SieveEditor::SieveEditor( QWidget * parent, const char * name )
  : KDialogBase( Plain, i18n( "Edit Sieve Script" ), Ok|Cancel, Ok, parent, name )
{
  QVBoxLayout * vlay = new QVBoxLayout( plainPage(), 0, spacingHint() );
  mTextEdit = new QTextEdit( plainPage() );
  vlay->addWidget( mTextEdit );
  mTextEdit->setFocus();
  mTextEdit->setTextFormat( QTextEdit::PlainText );
  mTextEdit->setWordWrap( QTextEdit::NoWrap );
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
