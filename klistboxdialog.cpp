// This must be first
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <klocale.h>
#include "klistboxdialog.h"

#include <qframe.h>
#include <qlistbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qheader.h>

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
    resize( 400, 150 );

    QFrame *page = makeMainWidget();
    QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
    descriptionLA = new QLabel( page, "descriptionLA" );
    descriptionLA->setText( labelText );

    topLayout->addWidget( descriptionLA );

    entriesLB = new QListBox( page, "entriesLB" );

    topLayout->addWidget( entriesLB );

    // signals and slots connections
    connect( entriesLB, SIGNAL( highlighted( const QString& ) ),
             this,      SLOT(   highlighted( const QString& ) ) );
    connect( entriesLB, SIGNAL( selected(int) ),
                        SLOT(   slotOk() ) );
    // buddies
    descriptionLA->setBuddy( entriesLB );
}

/*
 *  Destroys the object and frees any allocated resources
 */
KListBoxDialog::~KListBoxDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void KListBoxDialog::highlighted( const QString& txt )
{
    selectedString = txt;
}

#include "klistboxdialog.moc"
