// This must be first
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "klistboxdialog.h"

#include <qlabel.h>
#include <qlayout.h>

KListBoxDialog::KListBoxDialog( QString& _selectedString,
                                const QString& caption,
                                const QString& labelText,
                                QWidget* parent,
                                const char* name,
                                bool modal )
    : KDialogBase( parent, name, modal, caption, Ok|Cancel, Ok, true ),
      selectedString( _selectedString )

{
    if ( !name )
      setName( "KListBoxDialog" );
    resize( 400, 180 );

    QFrame *page = makeMainWidget();
    QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
    labelAboveLA = new QLabel( page, "labelAboveLA" );
    labelAboveLA->setText( labelText );

    topLayout->addWidget( labelAboveLA );

    entriesLB = new QListBox( page, "entriesLB" );

    topLayout->addWidget( entriesLB );

    commentBelowLA = new QLabel( page, "commentBelowLA" );
    commentBelowLA->setText( "" );
    topLayout->addWidget( commentBelowLA );
    commentBelowLA->hide();

    // signals and slots connections
    connect( entriesLB, SIGNAL( highlighted( const QString& ) ),
             this,      SLOT(   highlighted( const QString& ) ) );
    connect( entriesLB, SIGNAL( selected(int) ),
                        SLOT(   slotOk() ) );
    // buddies
    labelAboveLA->setBuddy( entriesLB );
}

/*
 *  Destroys the object and frees any allocated resources
 */
KListBoxDialog::~KListBoxDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void KListBoxDialog::setLabelAbove(const QString& label)
{
    labelAboveLA->setText( label );
    if( label.isEmpty() )
        labelAboveLA->hide();
    else
        labelAboveLA->show();
}

void KListBoxDialog::setCommentBelow(const QString& comment)
{
    commentBelowLA->setText( comment );
    if( comment.isEmpty() )
        commentBelowLA->hide();
    else
        commentBelowLA->show();
}



void KListBoxDialog::highlighted( const QString& txt )
{
    selectedString = txt;
}

#include "klistboxdialog.moc"
