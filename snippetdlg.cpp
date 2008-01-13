
#include "snippetdlg.h"

#include <kdialog.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <kpushbutton.h>
#include "kactioncollection.h"
#include "kmessagebox.h"

/*
 *  Constructs a SnippetDlg as a child of 'parent', with the
 *  widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SnippetDlg::SnippetDlg( KActionCollection* ac, QWidget* parent, bool modal,
                        Qt::WindowFlags f )
  : QDialog( parent, f ),
    actionCollection( ac )
{
    setupUi( this );
    setModal( modal );

    connect( keyWidget, SIGNAL( capturedKeySequence( const QKeySequence& ) ),
             this, SLOT( slotCapturedShortcut( const QKeySequence& ) ) );

    //TODO tab order in designer!
    setTabOrder( snippetText, keyWidget );
    setTabOrder( keyWidget, btnAdd );
    setTabOrder( btnAdd, btnCancel );
}

/*
 *  Destroys the object and frees any allocated resources
 */
SnippetDlg::~SnippetDlg()
{
    // no need to delete child widgets, Qt does it all for us
}

static bool shortcutIsValid( const KActionCollection *actionCollection,
                             const QKeySequence &ks )
{
  foreach ( QAction *const a, actionCollection->actions() ) {
    foreach ( const QKeySequence &other, a->shortcuts() ) {
      if ( ks == other )
        return false;
    }
  }
  return true;
}

void SnippetDlg::slotCapturedShortcut( const QKeySequence &ks )
{

  if ( ks == keyWidget->keySequence() )
    return;
  if ( !ks.isEmpty() && !shortcutIsValid( actionCollection, ks ) ) {
    QString msg( i18n( "The selected shortcut is already used, "
                       "please select a different one." ) );
    KMessageBox::sorry( this, msg );
    keyWidget->setKeySequence( QKeySequence() );
  }
}

void SnippetDlg::setGroupMode( bool groupMode )
{
  const bool full = !groupMode;
  textLabelGroup->setShown( full );
  cbGroup->setShown( full );
  textLabel->setShown( full );
  snippetText->setShown( full );
  keyWidgetLabel->setShown( full );
  keyWidget->setShown( full );
  if ( groupMode )
    resize( width(), minimumSizeHint().height() );
}

#include "snippetdlg.moc"
