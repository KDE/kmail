
#include "snippetdlg.h"

#include <kdialog.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <kpushbutton.h>
#include <ktextedit.h>
#include "kkeybutton.h"
#include "kactioncollection.h"
#include "kmessagebox.h"

/*
 *  Constructs a SnippetDlg as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SnippetDlg::SnippetDlg( KActionCollection* ac, QWidget* parent, const char* name, bool modal, WFlags fl )
    : SnippetDlgBase( parent, name, modal, fl ), actionCollection( ac )
{
    if ( !name )
	setName( "SnippetDlg" );

    textLabel3 = new QLabel( this, "textLabel3" );
    keyButton = new KKeyButton( this );
    connect( keyButton, SIGNAL( capturedShortcut( const KShortcut& ) ),
             this, SLOT( slotCapturedShortcut( const KShortcut& ) ) );

    layout3->addWidget( textLabel3, 7, 0 );
    layout3->addWidget( keyButton, 7, 1 );

    // tab order
    setTabOrder( snippetText, keyButton );
    setTabOrder( keyButton, btnAdd );
    setTabOrder( btnAdd, btnCancel );

    textLabel3->setBuddy( keyButton );
    languageChange();
}

/*
 *  Destroys the object and frees any allocated resources
 */
SnippetDlg::~SnippetDlg()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void SnippetDlg::languageChange()
{
    textLabel3->setText( tr2i18n( "Sh&ortcut:" ) );
}

static bool shortcutIsValid( const KActionCollection* actionCollection, const KShortcut &sc )
{
  KActionPtrList actions = actionCollection->actions();
  KActionPtrList::Iterator it( actions.begin() );
  for ( ; it != actions.end(); it++ ) {
    if ( (*it)->shortcut() == sc ) return false;
  }
  return true;
}

void SnippetDlg::slotCapturedShortcut( const KShortcut& sc )
{

    if ( sc == keyButton->shortcut() ) return;
    if ( sc.toString().isNull() ) {
      // null is fine, that's reset, but sc.іsNull() will be false :/
      keyButton->setShortcut( KShortcut::null(), false );
    } else {
      if( !shortcutIsValid( actionCollection, sc ) ) {
        QString msg( i18n( "The selected shortcut is already used, "
              "please select a different one." ) );
        KMessageBox::sorry( this, msg );
      } else {
        keyButton->setShortcut( sc, false );
      }
    }
}

void SnippetDlg::setShowShortcut( bool show )
{
    textLabel3->setShown( show );
    keyButton->setShown( show );
}

#include "snippetdlg.moc"
